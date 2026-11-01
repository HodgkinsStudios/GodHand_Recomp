#include "game_locator.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace godhand {

fs::path launcher_executable_directory() {
#ifdef __linux__
    std::error_code ec;
    const auto exe = fs::read_symlink("/proc/self/exe", ec);
    if (!ec && exe.is_absolute()) return exe.parent_path();
#endif
    std::error_code fallback_ec;
    return fs::current_path(fallback_ec);
}

std::optional<fs::path> find_game_executable(
    const fs::path& launcher_dir,
    const fs::path& working_dir,
    const fs::path& data_dir) {
    const char* const name = "GodHand_Recomp_Game";
    // The two executables are siblings in built/linux-x86_64 or ~/.local/bin.
    // Fall back to the project-root built/ folder when launching via desktop
    // shortcuts, or to the legacy layout for existing user installations.
    const std::array<fs::path, 5> candidates = {
        launcher_dir / name,
        launcher_dir / "built" / "linux-x86_64" / name,
        working_dir / name,
        working_dir / "built" / "linux-x86_64" / name,
        data_dir / "bin" / name,
    };
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::is_regular_file(candidate, ec)) {
            auto resolved = fs::absolute(candidate, ec);
            if (!ec) return resolved.lexically_normal();
            return candidate;
        }
    }
    return std::nullopt;
}

std::string game_binary_problem(const fs::path& executable) {
    std::error_code ec;
    if (!fs::is_regular_file(executable, ec))
        return "Native game executable is missing or is not a regular file.";
    std::ifstream input(executable, std::ios::binary);
    if (!input) return "Unable to open the native game executable.";
    std::array<char, 64> header{};
    input.read(header.data(), static_cast<std::streamsize>(header.size()));
    const auto bytes = input.gcount();
    constexpr char lfs[] = "version https://git-lfs.github.com/spec/v1";
    if (bytes >= static_cast<std::streamsize>(sizeof(lfs) - 1) &&
        std::memcmp(header.data(), lfs, sizeof(lfs) - 1) == 0)
        return "This file is only a Git LFS pointer, not the compiled game. "
               "Run 'git lfs pull' in the repository and reinstall using "
               "'bash godhandLauncher/install_prebuilt.sh'.";
    if (bytes < 4 || std::memcmp(header.data(), "\x7f" "ELF", 4) != 0)
        return "The selected native game is not a Linux ELF executable. "
               "Rebuild the native AOT or restore the complete Git LFS binary.";
    return {};
}

} // namespace godhand
