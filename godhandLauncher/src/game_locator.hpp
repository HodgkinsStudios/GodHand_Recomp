#pragma once

#include <filesystem>
#include <string>
#include <optional>

namespace godhand {

// Resolved from /proc/self/exe on Ubuntu. Never depend on the shell working
// directory to determine which binary was launched.
std::filesystem::path launcher_executable_directory();

// Prefer the native game binary beside the launcher, then a development checkout
// built/ directory, and lastly the legacy user-data installation location.
std::optional<std::filesystem::path> find_game_executable(
    const std::filesystem::path& launcher_dir,
    const std::filesystem::path& working_dir,
    const std::filesystem::path& data_dir);

// Returns a user-readable error for LFS pointers and invalid Linux game files.
std::string game_binary_problem(const std::filesystem::path& executable);

} // namespace godhand
