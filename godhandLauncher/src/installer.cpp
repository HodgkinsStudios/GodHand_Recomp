#include "installer.hpp"
#include "afs_installer.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace godhand {
namespace fs = std::filesystem;
namespace {

struct UniqueFd {
    int fd = -1;
    explicit UniqueFd(int value = -1) : fd(value) {}
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    UniqueFd(UniqueFd&& other) noexcept : fd(std::exchange(other.fd, -1)) {}
    ~UniqueFd() { if (fd >= 0) ::close(fd); }
};

struct ArchiveReader {
    archive* ptr = archive_read_new();
    ArchiveReader() {
        if (!ptr) throw std::runtime_error("Cannot create ISO archive reader");
        archive_read_support_format_iso9660(ptr);
        archive_read_support_filter_none(ptr);
    }
    ~ArchiveReader() { if (ptr) archive_read_free(ptr); }
};

void report(const ProgressCallback& progress, const std::string& label, double fraction) {
    if (progress) progress(label, std::clamp(fraction, 0.0, 1.0));
}

void verify_write(ssize_t result, const char* action) {
    if (result < 0) throw std::runtime_error(std::string(action) + ": " + std::strerror(errno));
}

void write_all(int fd, const char* data, size_t count) {
    while (count) {
        const auto sent = ::write(fd, data, count);
        verify_write(sent, "Writing extracted ISO file");
        if (sent == 0) throw std::runtime_error("Short write extracting ISO");
        data += sent;
        count -= static_cast<size_t>(sent);
    }
}

// Walk path components using dirfds. O_NOFOLLOW prevents archive-created symlink traversal.
UniqueFd parent_dir(int root_fd, const fs::path& path) {
    UniqueFd parent(::dup(root_fd));
    if (parent.fd < 0) throw std::runtime_error("Cannot access extraction root");
    auto it = path.begin();
    const auto last = std::prev(path.end());
    for (; it != last; ++it) {
        const auto part = it->string();
        if (::mkdirat(parent.fd, part.c_str(), 0755) != 0 && errno != EEXIST)
            throw std::runtime_error("Creating directory in ISO: " + part + ": " + std::strerror(errno));
        int next = ::openat(parent.fd, part.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (next < 0) throw std::runtime_error("Unsafe ISO path component: " + part);
        ::close(parent.fd);
        parent.fd = next;
    }
    return parent;
}

void extract_iso(const fs::path& source, const fs::path& destination,
                 const ProgressCallback& progress) {
    ArchiveReader reader;
    if (archive_read_open_filename(reader.ptr, source.c_str(), 1024 * 1024) != ARCHIVE_OK)
        throw std::runtime_error(std::string("Cannot open ISO9660 disc image: ") + archive_error_string(reader.ptr));

    UniqueFd root(::open(destination.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (root.fd < 0) throw std::runtime_error("Cannot open extraction directory");

    archive_entry* entry = nullptr;
    std::uint64_t count = 0;
    while (true) {
        const int status = archive_read_next_header(reader.ptr, &entry);
        if (status == ARCHIVE_EOF) break;
        if (status != ARCHIVE_OK)
            throw std::runtime_error(std::string("ISO directory read failed: ") + archive_error_string(reader.ptr));

        const char* name = archive_entry_pathname(entry);
        fs::path rel;
        if (name && (std::string_view(name) == "." || std::string_view(name) == "./")
            && archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(reader.ptr);
            continue;
        }
        if (!name || !sanitized_iso_path(name, rel))
            throw std::runtime_error("Refusing an unsafe ISO entry path");

        const auto type = archive_entry_filetype(entry);
        if (type != AE_IFDIR && type != AE_IFREG) {
            // Do not materialize links, devices, FIFOs or other special files.
            archive_read_data_skip(reader.ptr);
            continue;
        }
        UniqueFd parent = parent_dir(root.fd, rel);
        const auto filename = rel.filename().string();
        if (type == AE_IFDIR) {
            if (::mkdirat(parent.fd, filename.c_str(), 0755) != 0 && errno != EEXIST)
                throw std::runtime_error("Cannot create ISO directory: " + filename);
            UniqueFd verify(::openat(parent.fd, filename.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
            if (verify.fd < 0) throw std::runtime_error("Unsafe ISO directory: " + filename);
            archive_read_data_skip(reader.ptr);
        } else {
            UniqueFd out(::openat(parent.fd, filename.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644));
            if (out.fd < 0) throw std::runtime_error("Cannot create extracted file: " + filename + ": " + std::strerror(errno));
            char buf[256 * 1024];
            while (true) {
                const la_ssize_t got = archive_read_data(reader.ptr, buf, sizeof(buf));
                if (got < 0) throw std::runtime_error(std::string("ISO extraction error: ") + archive_error_string(reader.ptr));
                if (got == 0) break;
                write_all(out.fd, buf, static_cast<size_t>(got));
            }
        }
        ++count;
        if (count % 25 == 0) report(progress, "Extracting game assets (" + std::to_string(count) + " entries)", 0.55);
    }
    if (count == 0) throw std::runtime_error("ISO contained no extractable files");
}

void copy_iso(const fs::path& source, const fs::path& target,
              std::uintmax_t size, const ProgressCallback& progress) {
    std::ifstream in(source, std::ios::binary);
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    if (!in || !out) throw std::runtime_error("Cannot open ISO copy source or destination");
    std::vector<char> buffer(1024 * 1024);
    std::uintmax_t copied = 0;
    std::uintmax_t last_report = 0;
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto n = in.gcount();
        if (n <= 0) break;
        out.write(buffer.data(), n);
        if (!out) throw std::runtime_error("Disk write failed copying ISO (possibly out of space)");
        copied += static_cast<std::uintmax_t>(n);
        if (copied - last_report >= 16 * 1024 * 1024 || copied == size) {
            last_report = copied;
            report(progress, "Copying original ISO", 0.05 + 0.45 * static_cast<double>(copied) / static_cast<double>(size));
        }
    }
    out.flush();
    if (!out || in.bad() || copied != size) throw std::runtime_error("ISO copy incomplete or write failed");
}

} // namespace

fs::path data_directory() {
    const char* home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME environment variable not set");
    return fs::path(home) / ".local/share/HodgkinsStudios/GodHand_Recomp";
}

bool is_installed(const fs::path& root) {
    std::error_code ec;
    return fs::is_regular_file(root / "GodHand.iso", ec)
        && fs::is_directory(root / "Extracted_Iso", ec)
        && fs::is_regular_file(root / "install.complete", ec);
}

bool sanitized_iso_path(const std::string& raw, fs::path& relative) {
    relative.clear();
    if (raw.empty() || raw.front() == '/' || raw.find('\\') != std::string::npos ||
        raw.find('\0') != std::string::npos) return false;
    fs::path path(raw);
    for (const auto& part : path) {
        const auto s = part.string();
        if (s == ".." || s.empty()) return false;
        if (s != ".") relative /= part;
    }
    return !relative.empty() && !relative.is_absolute();
}

InstallResult inspect_iso(const fs::path& source) {
    try {
        if (!fs::is_regular_file(source) || fs::file_size(source) == 0)
            throw std::runtime_error("ISO file not found or empty");
        ArchiveReader reader;
        if (archive_read_open_filename(reader.ptr, source.c_str(), 1024 * 1024) != ARCHIVE_OK)
            throw std::runtime_error("Unable to read selected ISO as ISO9660");
        archive_entry* entry = nullptr;
        std::uint64_t regular = 0;
        while (true) {
            const auto code = archive_read_next_header(reader.ptr, &entry);
            if (code == ARCHIVE_EOF) break;
            if (code != ARCHIVE_OK) throw std::runtime_error("Error scanning ISO9660 directory");
            const char* name = archive_entry_pathname(entry);
            if (!name) throw std::runtime_error("ISO contains unnamed archive entry");
            if ((std::string_view(name) != "." && std::string_view(name) != "./")) {
                fs::path rel;
                if (!sanitized_iso_path(name, rel)) throw std::runtime_error("Unsafe path in ISO");
                if (archive_entry_filetype(entry) == AE_IFREG) ++regular;
            }
            archive_read_data_skip(reader.ptr);
        }
        if (!regular) throw std::runtime_error("ISO contains no regular files");
        return {true, "Readable ISO9660 disc image (" + std::to_string(regular) + " files)"};
    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

InstallResult install_iso(const fs::path& source, const fs::path& root,
                          const ProgressCallback& progress) {
    const auto copy_temp = root / "GodHand.iso.partial";
    const auto extract_temp = root / "Extracted_Iso.partial";
    try {
        if (is_installed(root)) return {true, "God Hand is already installed"};
        std::error_code ec;
        if (!fs::is_regular_file(source) || !fs::exists(source))
            throw std::runtime_error("Selected ISO does not exist or is not a regular file");
        const auto file_size = fs::file_size(source);
        if (file_size == 0) throw std::runtime_error("Selected ISO is empty");
        fs::create_directories(root);
        if (fs::is_symlink(root)) throw std::runtime_error("Install directory cannot be a symlink");
        const auto free_bytes = fs::space(root).available;
        // First-run installation retains the ISO, extracted ISO9660 files,
        // and an additional extraction of all top-level CRI AFS entries.
        // Budget three complete disc images plus a small safety reserve.
        constexpr std::uintmax_t reserve = 64ull * 1024 * 1024;
        if (file_size > (std::numeric_limits<std::uintmax_t>::max() - reserve) / 3 ||
            free_bytes < file_size * 3 + reserve)
            throw std::runtime_error("Not enough free space: allow at least three times the ISO size, plus 64 MiB, for ISO + AFS extraction");

        fs::remove_all(copy_temp, ec);
        ec.clear();
        fs::remove_all(extract_temp, ec);
        ec.clear();
        fs::remove(root / "install.complete", ec);
        report(progress, "Copying original ISO", 0.05);
        copy_iso(source, copy_temp, file_size, progress);
        fs::create_directory(extract_temp);
        report(progress, "Reading ISO9660 file system", 0.51);
        extract_iso(copy_temp, extract_temp, progress);
        report(progress, "Finishing installation", 0.90);

        // Replace incomplete prior state, but never follow links inside the archive.
        fs::remove_all(root / "Extracted_Iso");
        fs::remove(root / "GodHand.iso", ec);
        fs::rename(copy_temp, root / "GodHand.iso");
        fs::rename(extract_temp, root / "Extracted_Iso");
        // The ISO9660 extraction only yields the large CRI _DATA.AFS container.
        // Unpack its authentic contents as part of first-run installation.
        const InstallResult afs = ensure_afs_extracted(root, [&](const std::string& label, double f) {
            report(progress, label, 0.90 + 0.09 * f);
        });
        if (!afs.ok) throw std::runtime_error(afs.message);
        std::ofstream marker(root / "install.complete", std::ios::trunc);
        marker << "GodHand_Recomp installation v1\nISO bytes=" << file_size << "\n";
        marker.close();
        if (!marker) throw std::runtime_error("Could not write installation completion marker");
        report(progress, "Installation complete", 1.0);
        return {true, "God Hand files installed successfully"};
    } catch (const std::exception& ex) {
        std::error_code ec;
        fs::remove(root / "install.complete", ec);
        fs::remove_all(copy_temp, ec);
        fs::remove_all(extract_temp, ec);
        return {false, ex.what()};
    }
}

} // namespace godhand
