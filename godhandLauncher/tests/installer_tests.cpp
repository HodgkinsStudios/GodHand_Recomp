#include "installer.hpp"
#include "afs_installer.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <unistd.h>

namespace fs = std::filesystem;

// Write a tiny test ISO with libarchive: no original game data is used.
void make_iso(const fs::path& destination) {
    archive* writer = archive_write_new();
    assert(writer);
    assert(archive_write_set_format_iso9660(writer) == ARCHIVE_OK);
    assert(archive_write_open_filename(writer, destination.c_str()) == ARCHIVE_OK);
    archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, "GODHAND/READ_ME.TXT");
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, 5);
    assert(archive_write_header(writer, entry) == ARCHIVE_OK);
    assert(archive_write_data(writer, "hello", 5) == 5);
    archive_entry_free(entry);

    // A tiny, valid CRI AFS nested inside this synthetic ISO9660 filesystem.
    // Only test data, never real game bytes or proprietary assets.
    std::vector<char> afs(4096, 0);
    afs[0] = 'A'; afs[1] = 'F'; afs[2] = 'S';
    auto u32 = [&](size_t pos, std::uint32_t value) {
        for (int j = 0; j < 4; ++j) afs[pos + size_t(j)] = char(value >> (8 * j));
    };
    u32(4, 2); // count
    u32(8, 2048); u32(12, 5); // record 0
    u32(16, 2304); u32(20, 4); // record 1
    u32(24, 2560); u32(28, 96); // optional CRI filename table
    const std::string name0 = "core.bin";
    const std::string name1 = "title.tim2";
    std::copy(name0.begin(), name0.end(), afs.begin() + 2560);
    std::copy(name1.begin(), name1.end(), afs.begin() + 2608);
    std::copy_n("hello", 5, afs.begin() + 2048);
    std::copy_n("TIM2", 4, afs.begin() + 2304);
    entry = archive_entry_new();
    archive_entry_set_pathname(entry, "_DATA.AFS");
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, static_cast<la_int64_t>(afs.size()));
    assert(archive_write_header(writer, entry) == ARCHIVE_OK);
    assert(archive_write_data(writer, afs.data(), afs.size()) == static_cast<la_ssize_t>(afs.size()));
    archive_entry_free(entry);
    assert(archive_write_close(writer) == ARCHIVE_OK);
    archive_write_free(writer);
}

int main() {
    fs::path relative;
    assert(godhand::sanitized_iso_path("GAME/EXEC.ELF", relative));
    assert(relative == fs::path("GAME/EXEC.ELF"));
    assert(godhand::sanitized_iso_path("./FOO/BAR", relative));
    assert(relative == fs::path("FOO/BAR"));
    for (auto bad : {"", "..", "../outside", "foo/../outside", "/etc/passwd", "A\\B"})
        assert(!godhand::sanitized_iso_path(bad, relative));

    const fs::path root = fs::temp_directory_path() / ("godhand_launcher_tests_" + std::to_string(getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path iso = root / "test.iso";
    make_iso(iso);
    assert(godhand::inspect_iso(iso).ok);
    assert(!godhand::is_installed(root / "install"));
    // ISO sample is small: the free-space test still applies normally.
    const auto result = godhand::install_iso(iso, root / "install");
    if (!result.ok) std::cerr << result.message << '\n';
    assert(result.ok);
    assert(godhand::is_installed(root / "install"));
    assert(godhand::afs_extraction_complete(root / "install"));
    assert(fs::file_size(root / "install/GodHand.iso") == fs::file_size(iso));
    const fs::path data = root / "install/Extracted_AFS";
    const fs::path first = data / "000000_core.bin";
    const fs::path second = data / "000001_title.tim2";
    assert(fs::file_size(first) == 5);
    assert(fs::file_size(second) == 4);
    // Rechecking must preserve all already-extracted asset bytes.
    assert(godhand::ensure_afs_extracted(root / "install").ok);
    assert(godhand::afs_extraction_complete(root / "install"));
    fs::remove(first);
    assert(!godhand::afs_extraction_complete(root / "install"));
    assert(godhand::ensure_afs_extracted(root / "install").ok);
    assert(fs::file_size(first) == 5);
    // Incomplete or corrupt-sized assets should be repaired automatically.
    { std::ofstream truncate(second, std::ios::binary | std::ios::trunc); truncate << "T"; }
    assert(!godhand::afs_extraction_complete(root / "install"));
    assert(godhand::ensure_afs_extracted(root / "install").ok);
    assert(fs::file_size(second) == 4);
    { std::ifstream input(second, std::ios::binary); std::string signature(4, '\0'); input.read(&signature[0], 4); assert(signature == "TIM2"); }
    // libarchive ISO9660 can change case or suffixes; verify payload exists.
    bool found = false;
    for (const auto& p : fs::recursive_directory_iterator(root / "install/Extracted_Iso")) {
        if (!p.is_regular_file()) continue;
        std::ifstream stream(p.path());
        std::string content;
        std::getline(stream, content);
        if (content == "hello") found = true;
    }
    assert(found);
    assert(godhand::install_iso(iso, root / "install").ok); // repeat is idempotent

    fs::create_directories(root / "bad");
    std::ofstream(root / "bad/not_an_iso.iso") << "not an iso";
    const auto bad = godhand::install_iso(root / "bad/not_an_iso.iso", root / "bad/install");
    assert(!bad.ok);
    assert(!godhand::inspect_iso(root / "bad/not_an_iso.iso").ok);
    assert(!godhand::is_installed(root / "bad/install"));
    fs::remove_all(root);
    std::cout << "installer tests passed\n";
}
