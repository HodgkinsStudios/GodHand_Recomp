#include "installer.hpp"
#include "afs_installer.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage:\n"
                  << "  godhand_iso_inspect --inspect-iso <ISO>\n"
                  << "  godhand_iso_inspect --verify-afs <installed data directory>\n"
                  << "  godhand_iso_inspect --ensure-afs <installed data directory>\n";
        return 2;
    }
    const std::string command = argv[1];
    if (command == "--inspect-iso") {
        const auto result = godhand::inspect_iso(argv[2]);
        std::cout << (result.ok ? "OK: " : "ERROR: ") << result.message << '\n';
        return result.ok ? 0 : 1;
    }
    if (command == "--verify-afs") {
        const bool complete = godhand::afs_extraction_complete(argv[2]);
        std::cout << (complete ? "OK: AFS extraction complete and all file sizes verified"
                               : "ERROR: AFS extraction missing or incomplete") << '\n';
        return complete ? 0 : 1;
    }
    if (command == "--ensure-afs") {
        const auto result = godhand::ensure_afs_extracted(argv[2],
            [](const std::string& label, double fraction) {
                const int percent = static_cast<int>(fraction * 100.0);
                if (percent % 10 == 0)
                    std::cout << percent << "% " << label << '\n';
            });
        std::cout << (result.ok ? "OK: " : "ERROR: ") << result.message << '\n';
        return result.ok ? 0 : 1;
    }
    std::cerr << "Unknown command: " << command << '\n';
    return 2;
}
