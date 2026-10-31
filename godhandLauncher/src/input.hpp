#pragma once
#include "settings.hpp"
#include <SDL.h>
#include <array>
#include <string>
namespace godhand {
class InputManager {
public:
    InputManager();
    ~InputManager();
    bool initialized() const { return initialized_; }
    const std::string& error() const { return error_; }
    void pump();
    bool is_gamepad_active() const;
    std::string connected_pad_name() const;
    bool action_down(int index, const Settings& cfg) const;
    bool consume_escape_toggle();
    bool poll_capture_key(std::string& key);
    bool poll_capture_button(std::string& button);
private:
    SDL_GameController* controller_ = nullptr;
    bool initialized_ = false;
    bool last_was_pad_ = false;
    bool escape_requested_ = false;
    std::string error_;
    std::string key_capture_, pad_capture_;
    void refresh_controller();
};
SDL_GameControllerButton button_from_name(const std::string& name);
const char* button_name(SDL_GameControllerButton button);
void apply_window_mode(SDL_Window* window, ScreenMode mode, int logical_width = 960, int logical_height = 540);
void configure_renderer(SDL_Renderer* renderer, ScreenMode mode,
                        int logical_width = 960, int logical_height = 540);
}
