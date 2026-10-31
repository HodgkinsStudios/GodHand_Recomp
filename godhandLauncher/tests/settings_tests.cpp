#include "settings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
int main() {
    godhand::Settings cfg;
    assert(cfg.keyboard[0] == "W");
    assert(cfg.gamepad[4] == "A");
    assert(godhand::next_screen_mode(godhand::ScreenMode::Windowed) == godhand::ScreenMode::Fullscreen);
    assert(godhand::next_screen_mode(godhand::ScreenMode::Fullscreen) == godhand::ScreenMode::Stretched);
    assert(godhand::next_screen_mode(godhand::ScreenMode::Stretched) == godhand::ScreenMode::Windowed);
    auto dir=fs::temp_directory_path()/("godhand_settings_"+std::to_string(getpid()));
    auto p=dir/"nested/settings.ini";
    cfg.mode=godhand::ScreenMode::Stretched;
    cfg.keyboard[0]="Up";
    cfg.gamepad[3]="X";
    assert(godhand::save_settings(p,cfg));
    godhand::Settings loaded;
    assert(godhand::load_settings(p,loaded));
    assert(loaded.mode==godhand::ScreenMode::Stretched);
    assert(loaded.keyboard[0]=="Up");
    assert(loaded.gamepad[3]=="X");
    std::ofstream(p, std::ios::app)<<"\nkeyboard_0=garbage\\nname\nkeyboard_1=Right\n";
    assert(godhand::load_settings(p,loaded));
    assert(loaded.keyboard[1]=="Right");
    fs::remove_all(dir);
    std::cout<<"settings tests passed\n";
}
