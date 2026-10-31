// Standalone input/display diagnostic. This is NOT God Hand gameplay.
#include "input.hpp"
#include "settings.hpp"
#include <SDL.h>
#include <iostream>
#include <string>
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        std::cerr << SDL_GetError() << '\n';return 1;
    }
    SDL_Window* win = SDL_CreateWindow("GodHand:Recomp — Input/Display Diagnostic",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540, SDL_WINDOW_RESIZABLE);
    if (!win) {std::cerr << SDL_GetError() << '\n';SDL_Quit();return 1;}
    SDL_Renderer* renderer = SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);
    if (!renderer) {std::cerr << SDL_GetError() << '\n';SDL_DestroyWindow(win);SDL_Quit();return 1;}
    godhand::Settings cfg;
    godhand::load_settings(godhand::settings_file(), cfg);
    godhand::InputManager input;
    godhand::apply_window_mode(win,cfg.mode);
    bool running=true;
    while(running) {
        input.pump();
        if (input.consume_escape_toggle()) {
            cfg.mode=godhand::next_screen_mode(cfg.mode);
            godhand::save_settings(godhand::settings_file(),cfg);
            godhand::apply_window_mode(win,cfg.mode);
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F10]) running=false;
        godhand::configure_renderer(renderer,cfg.mode);
        std::string title="GodHand:Recomp DIAGNOSTIC (NOT GAME) | ";
        title+=godhand::screen_mode_name(cfg.mode);
        title+=" | "+input.connected_pad_name()+" | F10 exits, Esc changes screen mode";
        SDL_SetWindowTitle(win,title.c_str());
        SDL_SetRenderDrawColor(renderer,20,27,35,255);
        SDL_RenderClear(renderer);
        // Blocks light up for the eleven remappable actions.
        for(int i=0;i<godhand::kActionCount;++i) {
            bool down=input.action_down(i,cfg);
            SDL_Rect r{60+(i%4)*210,80+(i/4)*120,170,80};
            SDL_SetRenderDrawColor(renderer,down?205:70,down?64:90,down?57:110,255);
            SDL_RenderFillRect(renderer,&r);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
