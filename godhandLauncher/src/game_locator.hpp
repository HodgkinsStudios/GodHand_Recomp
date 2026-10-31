#pragma once

#include <filesystem>
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

} // namespace godhand
