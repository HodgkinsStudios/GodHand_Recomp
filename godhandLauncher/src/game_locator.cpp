#include "game_locator.hpp"

#include <array>
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

} // namespace godhand
