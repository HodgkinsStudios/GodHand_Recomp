#include "installer.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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
    assert(fs::file_size(root / "install/GodHand.iso") == fs::file_size(iso));
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
