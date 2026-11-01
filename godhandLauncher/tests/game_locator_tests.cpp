#include "game_locator.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

int main() {
    const auto marker = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path root = fs::temp_directory_path() / ("godhand-locator-" + marker);
    const auto launcher = root / "install";
    const auto checkout = root / "source";
    const auto data = root / "user-data";
    const auto sibling = launcher / "GodHand_Recomp_Game";
    const auto built = checkout / "built/linux-x86_64/GodHand_Recomp_Game";
    const auto legacy = data / "bin/GodHand_Recomp_Game";
    fs::create_directories(launcher);
    fs::create_directories(built.parent_path());
    fs::create_directories(legacy.parent_path());
    assert(!godhand::find_game_executable(launcher, checkout, data));

    // A checked-out build is usable even if launcher is started from elsewhere.
    std::ofstream(built).put('x');
    assert(godhand::find_game_executable(launcher, checkout, data) == fs::absolute(built));
    std::ofstream(sibling).put('x');
    assert(godhand::find_game_executable(launcher, checkout, data) == fs::absolute(sibling));
    // A 134-byte LFS pointer can have the executable bit without being an ELF.
    assert(godhand::game_binary_problem(sibling).find("not a Linux ELF") != std::string::npos);
    {
        std::ofstream out(sibling, std::ios::binary | std::ios::trunc);
        out << "version https://git-lfs.github.com/spec/v1\n"
               "oid sha256:0000000000000000000000000000000000000000000000000000000000000000\n"
               "size 209109192\n";
    }
    assert(godhand::game_binary_problem(sibling).find("Git LFS pointer") != std::string::npos);
    {
        std::ofstream out(sibling, std::ios::binary | std::ios::trunc);
        out.write("\x7f" "ELF", 4);
    }
    assert(godhand::game_binary_problem(sibling).empty());
    fs::remove(sibling);
    fs::remove(built);
    std::ofstream(legacy).put('x');
    assert(godhand::find_game_executable(launcher, checkout, data) == fs::absolute(legacy));
    fs::remove_all(root);
}
