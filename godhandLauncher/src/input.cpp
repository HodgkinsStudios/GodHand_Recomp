#include "input.hpp"
#include <algorithm>
#include <cmath>

namespace godhand {
struct PadName { const char* text; SDL_GameControllerButton button; };
static constexpr PadName names[] = {
    {"A", SDL_CONTROLLER_BUTTON_A}, {"B", SDL_CONTROLLER_BUTTON_B},
    {"X", SDL_CONTROLLER_BUTTON_X}, {"Y", SDL_CONTROLLER_BUTTON_Y},
    {"LB", SDL_CONTROLLER_BUTTON_LEFTSHOULDER}, {"RB", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
    {"Back", SDL_CONTROLLER_BUTTON_BACK}, {"Start", SDL_CONTROLLER_BUTTON_START},
    {"LStick", SDL_CONTROLLER_BUTTON_LEFTSTICK}, {"RStick", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
    {"DPad Up", SDL_CONTROLLER_BUTTON_DPAD_UP}, {"DPad Down", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
    {"DPad Left", SDL_CONTROLLER_BUTTON_DPAD_LEFT}, {"DPad Right", SDL_CONTROLLER_BUTTON_DPAD_RIGHT}
};
SDL_GameControllerButton button_from_name(const std::string& s) {
    for (const auto& e : names) if (s == e.text) return e.button;
    return SDL_CONTROLLER_BUTTON_INVALID;
}
const char* button_name(SDL_GameControllerButton b) {
    for (const auto& e : names) if (b == e.button) return e.text;
    return "Unknown";
}
InputManager::InputManager() {
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        error_ = SDL_GetError(); return;
    }
    initialized_ = true; refresh_controller();
}
InputManager::~InputManager() {
    if (controller_) SDL_GameControllerClose(controller_);
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
}
void InputManager::refresh_controller() {
    if (controller_ && SDL_GameControllerGetAttached(controller_)) return;
    if (controller_) { SDL_GameControllerClose(controller_); controller_ = nullptr; }
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_) break;
        }
    }
    if (!controller_) last_was_pad_ = false;
}
void InputManager::pump() {
    if (!initialized_) return;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED) refresh_controller();
        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            pad_capture_ = button_name(static_cast<SDL_GameControllerButton>(e.cbutton.button));
            last_was_pad_ = true;
        }
        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            key_capture_ = SDL_GetScancodeName(e.key.keysym.scancode);
            last_was_pad_ = false;
        }
    }
    refresh_controller();
    if (controller_) {
        for (const auto& p : names) {
            if (SDL_GameControllerGetButton(controller_, p.button)) {last_was_pad_ = true;break;}
        }
        if (std::abs(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX)) > 16000 ||
            std::abs(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY)) > 16000) last_was_pad_ = true;
    }
}
bool InputManager::is_gamepad_active() const { return controller_ && last_was_pad_; }
std::string InputManager::connected_pad_name() const {
    return controller_ ? SDL_GameControllerName(controller_) : "No gamepad connected";
}
bool InputManager::action_down(int index, const Settings& cfg) const {
    if (index < 0 || index >= kActionCount || !initialized_) return false;
    SDL_Scancode scan = SDL_GetScancodeFromName(cfg.keyboard[index].c_str());
    if (cfg.keyboard[index] == "LShift") scan = SDL_SCANCODE_LSHIFT;
    bool keyboard_down = scan != SDL_SCANCODE_UNKNOWN && SDL_GetKeyboardState(nullptr)[scan];
    SDL_GameControllerButton button = button_from_name(cfg.gamepad[index]);
    bool pad_down = controller_ && button != SDL_CONTROLLER_BUTTON_INVALID &&
        SDL_GameControllerGetButton(controller_, button);
    // Accept either input, with automatic controller availability detection.
    if (controller_ && index < 4) {
        const int axis = (index < 2 ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_LEFTX);
        int value = SDL_GameControllerGetAxis(controller_, static_cast<SDL_GameControllerAxis>(axis));
        if ((index == 0 || index == 2) ? value < -16000 : value > 16000) pad_down = true;
    }
    return keyboard_down || pad_down;
}
bool InputManager::poll_capture_key(std::string& key) {
    if (key_capture_.empty()) return false;
    key.swap(key_capture_); key_capture_.clear(); return true;
}
bool InputManager::poll_capture_button(std::string& button) {
    if (pad_capture_.empty()) return false;
    button.swap(pad_capture_); pad_capture_.clear(); return true;
}
void apply_window_mode(SDL_Window* window, ScreenMode mode, int logical_width, int logical_height) {
    if (!window) return;
    if (mode == ScreenMode::Windowed) {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, logical_width, logical_height);
    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    // A game renderer should set SDL_RenderSetLogicalSize(960,540) when letterboxed;
    // stretched mode uses the full display destination. See docs/INPUT_AND_DISPLAY.md.
}
}
