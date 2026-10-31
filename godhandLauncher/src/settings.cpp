#include "settings.hpp"
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

namespace godhand {
const std::array<Binding, kActionCount> action_defaults = {{{"Move Up", "W", "DPad Up"},
    {"Move Down", "S", "DPad Down"}, {"Move Left", "A", "DPad Left"},
    {"Move Right", "D", "DPad Right"}, {"Light Attack", "J", "A"},
    {"Heavy Attack", "K", "X"}, {"Guard", "LShift", "LB"},
    {"Evade", "Space", "B"}, {"Lock On", "Tab", "RB"},
    {"Start", "Return", "Start"}, {"Pause", "P", "Back"}}};
Settings::Settings() {
    for (int i = 0; i < kActionCount; ++i) {
        keyboard[i] = action_defaults[i].default_key;
        gamepad[i] = action_defaults[i].default_pad;
    }
}
std::filesystem::path settings_file() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return std::filesystem::path(xdg) / "HodgkinsStudios/GodHand_Recomp/settings.ini";
    const char* home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is not set");
    return std::filesystem::path(home) / ".config/HodgkinsStudios/GodHand_Recomp/settings.ini";
}
const char* screen_mode_name(ScreenMode mode) {
    switch(mode) { case ScreenMode::Windowed: return "Windowed";
        case ScreenMode::Fullscreen: return "Fullscreen";
        case ScreenMode::Stretched: return "Fullscreen (stretched)"; }
    return "Windowed";
}
ScreenMode next_screen_mode(ScreenMode mode) {
    return static_cast<ScreenMode>((static_cast<int>(mode) + 1) % 3);
}
static bool valid(const std::string& s) {
    return !s.empty() && s.size() <= 64 &&
        s.find_first_of("\r\n=\0", 0, 3) == std::string::npos;
}
bool load_settings(const std::filesystem::path& path, Settings& dst) {
    std::ifstream in(path);
    if (!in) return false;
    Settings loaded;
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = line.substr(0, eq), value = line.substr(eq + 1);
        if (key == "screen_mode") {
            if (value == "windowed") loaded.mode = ScreenMode::Windowed;
            else if (value == "fullscreen") loaded.mode = ScreenMode::Fullscreen;
            else if (value == "stretched") loaded.mode = ScreenMode::Stretched;
        }
        if (!valid(value)) continue;
        for (int i = 0; i < kActionCount; ++i) {
            if (key == "keyboard_" + std::to_string(i)) loaded.keyboard[i] = value;
            if (key == "gamepad_" + std::to_string(i)) loaded.gamepad[i] = value;
        }
    }
    if (in.bad()) return false;
    dst = loaded;
    return true;
}
bool save_settings(const std::filesystem::path& path, const Settings& settings) {
    try {
        std::filesystem::create_directories(path.parent_path());
        const auto temp = std::filesystem::path(path.string() + ".partial");
        std::ofstream out(temp, std::ios::trunc);
        if (!out) return false;
        out << "# GodHand:Recomp settings, version 1\n";
        out << "screen_mode=" << (settings.mode == ScreenMode::Windowed ? "windowed" :
            settings.mode == ScreenMode::Fullscreen ? "fullscreen" : "stretched") << '\n';
        for (int i = 0; i < kActionCount; ++i) {
            if (!valid(settings.keyboard[i]) || !valid(settings.gamepad[i])) return false;
            out << "keyboard_" << i << '=' << settings.keyboard[i] << '\n';
            out << "gamepad_" << i << '=' << settings.gamepad[i] << '\n';
        }
        out.close();
        if (!out) return false;
        std::filesystem::rename(temp, path);
        return true;
    } catch (...) { return false; }
}
}
