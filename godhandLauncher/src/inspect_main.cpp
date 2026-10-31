#include "installer.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "--inspect-iso") {
        std::cerr << "Usage: godhand_iso_inspect --inspect-iso <local ISO path>\n";
        return 2;
    }
    const auto result = godhand::inspect_iso(argv[2]);
    std::cout << (result.ok ? "OK: " : "ERROR: ") << result.message << '\n';
    return result.ok ? 0 : 1;
}
