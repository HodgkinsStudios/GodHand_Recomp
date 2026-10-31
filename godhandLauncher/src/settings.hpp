#pragma once
#include <array>
#include <filesystem>
#include <string>

namespace godhand {
enum class ScreenMode { Windowed = 0, Fullscreen = 1, Stretched = 2 };
constexpr int kActionCount = 11;
struct Binding {
    const char* name;
    const char* default_key;
    const char* default_pad;
};
extern const std::array<Binding, kActionCount> action_defaults;
struct Settings {
    ScreenMode mode = ScreenMode::Windowed;
    std::array<std::string, kActionCount> keyboard{};
    std::array<std::string, kActionCount> gamepad{};
    Settings();
};
std::filesystem::path settings_file();
bool load_settings(const std::filesystem::path& path, Settings& settings);
bool save_settings(const std::filesystem::path& path, const Settings& settings);
const char* screen_mode_name(ScreenMode mode);
ScreenMode next_screen_mode(ScreenMode mode);
}
