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
    fs::remove(sibling);
    fs::remove(built);
    std::ofstream(legacy).put('x');
    assert(godhand::find_game_executable(launcher, checkout, data) == fs::absolute(legacy));
    fs::remove_all(root);
}
