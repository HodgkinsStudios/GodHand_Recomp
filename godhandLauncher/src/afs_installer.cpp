#include "afs_installer.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <utility>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace godhand {
namespace fs = std::filesystem;
namespace {
struct Entry {
    std::uint32_t offset = 0;
    std::uint32_t length = 0;
    std::string filename;
};
struct Index {
    fs::path source;
    std::uintmax_t archive_size = 0;
    std::vector<Entry> entries;
};
struct Fd {
    int value = -1;
    explicit Fd(int fd) : value(fd) {}
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : value(std::exchange(other.value, -1)) {}
    ~Fd() { if (value >= 0) ::close(value); }
};
void fail(const std::string& reason) { throw std::runtime_error("AFS extraction: " + reason); }
std::uint32_t le32(const unsigned char* bytes) {
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
           (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}
void read_exact(std::ifstream& stream, std::uintmax_t offset, void* target, size_t size) {
    if (offset > std::uintmax_t(std::numeric_limits<std::streamoff>::max()))
        fail("archive offset is too large");
    stream.clear();
    stream.seekg(static_cast<std::streamoff>(offset));
    stream.read(static_cast<char*>(target), static_cast<std::streamsize>(size));
    if (!stream || stream.gcount() != static_cast<std::streamsize>(size))
        fail("unexpected end of _DATA.AFS");
}
fs::path find_source(const fs::path& data_dir) {
    const fs::path disc = data_dir / "Extracted_Iso";
    std::error_code ec;
    if (!fs::is_directory(disc, ec)) fail("extracted disc directory is missing");
    for (const auto& item : fs::directory_iterator(disc)) {
        std::string name = item.path().filename().string();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        if (name == "_DATA.AFS" || name == "_DATA.AFS;1") {
            if (!item.is_regular_file() || item.is_symlink())
                fail("invalid source _DATA.AFS file");
            return item.path();
        }
    }
    fail("disc is missing _DATA.AFS; verify or reinstall the original ISO");
    return {};
}
std::string entry_name(std::string_view original, size_t id) {
    std::string filename;
    for (unsigned char ch : original) {
        if (ch == '/' || ch == '\\' || ch < 0x20 || ch >= 0x7f)
            filename.push_back('_');
        else if (std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
            filename.push_back(char(ch));
        else
            filename.push_back('_');
    }
    // Do not create any path elements from the untrusted on-disc filename.
    while (!filename.empty() && filename.front() == '.') filename.erase(filename.begin());
    if (filename.empty()) filename = "unnamed.bin";
    if (filename.size() > 90) filename.resize(90);
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(6) << id << "_" << filename;
    return stream.str();
}
Index parse(const fs::path& root) {
    Index result;
    result.source = find_source(root);
    result.archive_size = fs::file_size(result.source);
    if (result.archive_size < 16) fail("_DATA.AFS header is truncated");
    std::ifstream in(result.source, std::ios::binary);
    if (!in) fail("cannot open _DATA.AFS");
    std::array<unsigned char, 8> head{};
    read_exact(in, 0, head.data(), head.size());
    if (std::memcmp(head.data(), "AFS\0", 4) != 0)
        fail("_DATA.AFS has the wrong CRI AFS signature");
    const std::uint32_t count = le32(head.data() + 4);
    if (count == 0 || count > 300000 || 16ull + 8ull * count > result.archive_size)
        fail("AFS index count is invalid or truncated");
    const size_t table_size = size_t(count) * 8 + 8;
    std::vector<unsigned char> table(table_size);
    read_exact(in, 8, table.data(), table.size());
    const std::uint32_t toc_at = le32(table.data() + size_t(count) * 8);
    const std::uint32_t toc_bytes = le32(table.data() + size_t(count) * 8 + 4);
    if (toc_at != 0 && (std::uint64_t(toc_at) + toc_bytes > result.archive_size ||
                        std::uint64_t(toc_bytes) < std::uint64_t(count) * 48))
        fail("AFS filename table extends outside original archive");
    std::vector<unsigned char> names;
    if (toc_at != 0) {
        names.resize(size_t(count) * 48);
        read_exact(in, toc_at, names.data(), names.size());
    }
    result.entries.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint32_t offset = le32(table.data() + size_t(i) * 8);
        const std::uint32_t size = le32(table.data() + size_t(i) * 8 + 4);
        if (std::uint64_t(offset) + size > result.archive_size)
            fail("AFS entry " + std::to_string(i) + " extends beyond archive");
        std::string_view title;
        if (!names.empty()) {
            const char* data = reinterpret_cast<const char*>(names.data() + size_t(i) * 48);
            title = std::string_view(data, strnlen(data, 32));
        }
        result.entries.push_back({offset, size, entry_name(title, i)});
    }
    return result;
}
std::string marker(const Index& index) {
    return "GodHand_Recomp AFS extraction v2\narchive_bytes=" +
           std::to_string(index.archive_size) + "\nentries=" +
           std::to_string(index.entries.size()) + "\n";
}
bool existing_ok(int fd, const Entry& entry) {
    struct stat st {};
    if (::fstatat(fd, entry.filename.c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0)
        return false;
    return S_ISREG(st.st_mode) && st.st_size >= 0 &&
           std::uintmax_t(st.st_size) == entry.length;
}
Fd open_folder(const fs::path& path, bool create) {
    std::error_code ec;
    if (create) fs::create_directories(path, ec);
    if (ec) fail("cannot create local Extracted_AFS: " + ec.message());
    Fd dir(::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (dir.value < 0) fail("unsafe or unreadable local Extracted_AFS directory");
    return dir;
}
void write_all(int fd, const char* data, size_t count) {
    while (count) {
        const ssize_t done = ::write(fd, data, count);
        if (done <= 0) fail("writing extracted AFS entry: " + std::string(std::strerror(errno)));
        data += done;
        count -= size_t(done);
    }
}
void write_entry(std::ifstream& source, int directory_fd, const Entry& item) {
    const std::string temporary = item.filename + ".partial";
    // Interrupted work is discarded, never presented as an installed entry.
    ::unlinkat(directory_fd, temporary.c_str(), 0);
    Fd out(::openat(directory_fd, temporary.c_str(),
                    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644));
    if (out.value < 0) fail("cannot create AFS entry " + item.filename);
    try {
        source.clear();
        source.seekg(item.offset);
        if (!source) fail("cannot seek to AFS entry");
        std::array<char, 1024 * 1024> buffer{};
        std::uint64_t remaining = item.length;
        while (remaining) {
            const auto chunk = size_t(std::min<std::uint64_t>(remaining, buffer.size()));
            source.read(buffer.data(), std::streamsize(chunk));
            if (!source || source.gcount() != std::streamsize(chunk))
                fail("AFS source truncated while extracting " + item.filename);
            write_all(out.value, buffer.data(), chunk);
            remaining -= chunk;
        }
        if (::fsync(out.value) != 0) fail("cannot flush extracted AFS data");
        if (::renameat(directory_fd, temporary.c_str(),
                       directory_fd, item.filename.c_str()) != 0)
            fail("cannot commit extracted AFS entry " + item.filename);
    } catch (...) {
        ::unlinkat(directory_fd, temporary.c_str(), 0);
        throw;
    }
}
bool complete(const Index& index, const fs::path& root) {
    const fs::path destination = root / "Extracted_AFS";
    std::error_code ec;
    if (!fs::is_directory(destination, ec) || fs::is_symlink(destination, ec))
        return false;
    std::ifstream done(destination / "afs.complete");
    std::string value((std::istreambuf_iterator<char>(done)), std::istreambuf_iterator<char>());
    if (value != marker(index)) return false;
    Fd dir(::open(destination.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (dir.value < 0) return false;
    for (const auto& entry : index.entries)
        if (!existing_ok(dir.value, entry)) return false;
    return true;
}
} // namespace

bool afs_extraction_complete(const fs::path& root) {
    try { return complete(parse(root), root); }
    catch (...) { return false; }
}

InstallResult ensure_afs_extracted(const fs::path& root, const ProgressCallback& progress) {
    try {
        const Index index = parse(root);
        if (complete(index, root))
            return {true, "All AFS files are already extracted and verified"};
        const fs::path destination = root / "Extracted_AFS";
        Fd dir = open_folder(destination, true);
        std::uint64_t pending_bytes = 0;
        std::uint32_t pending_count = 0;
        for (const auto& entry : index.entries) {
            if (!existing_ok(dir.value, entry)) {
                pending_bytes += entry.length;
                ++pending_count;
            }
        }
        if (fs::space(destination).available < pending_bytes + 32ull * 1024 * 1024)
            fail("not enough free disk space to extract " + std::to_string(pending_bytes) +
                 " AFS bytes (additional 32 MiB safety reserve)");
        std::ifstream source(index.source, std::ios::binary);
        if (!source) fail("cannot reopen _DATA.AFS");
        std::uint64_t processed = 0;
        std::uint32_t repaired = 0;
        for (size_t i = 0; i < index.entries.size(); ++i) {
            const Entry& entry = index.entries[i];
            if (!existing_ok(dir.value, entry)) {
                write_entry(source, dir.value, entry);
                ++repaired;
            }
            processed += entry.length;
            if (progress && (i % 8 == 0 || i + 1 == index.entries.size())) {
                const double fraction = processed == 0 ? 1.0 :
                    double(i + 1) / double(index.entries.size());
                progress("Extracting AFS game assets (" + std::to_string(i + 1) + "/" +
                         std::to_string(index.entries.size()) + ", " +
                         std::to_string(repaired) + " repaired)", fraction);
            }
        }
        // Commit completion state using a separately created atomic file.
        const std::string text = marker(index);
        const std::string temporary = "afs.complete.partial";
        ::unlinkat(dir.value, temporary.c_str(), 0);
        {
            Fd fd(::openat(dir.value, temporary.c_str(),
                           O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC, 0644));
            if (fd.value < 0) fail("cannot write AFS completion marker");
            write_all(fd.value, text.data(), text.size());
            if (::fsync(fd.value) != 0) fail("cannot sync AFS completion marker");
        }
        if (::renameat(dir.value, temporary.c_str(), dir.value, "afs.complete") != 0)
            fail("cannot finalize AFS completion marker");
        if (!complete(index, root)) fail("AFS extraction verification failed");
        return {true, "AFS extraction complete: " +
                      std::to_string(index.entries.size()) + " entries, " +
                      std::to_string(repaired) + " extracted/repaired"};
    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

} // namespace godhand
