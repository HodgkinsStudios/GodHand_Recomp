# Launcher input and display settings (development)

GodHand:Recomp has a GTK3 desktop launcher with an ISO installation flow, Settings dialog, keyboard and SDL2 game controller bindings. On first launch, choose your original ISO using the native file picker; it is copied to `~/.local/share/HodgkinsStudios/GodHand_Recomp/GodHand.iso` and extracted into `Extracted_Iso/`. Later launches display **Play**.

Settings persist in `~/.config/HodgkinsStudios/GodHand_Recomp/settings.ini` (or `$XDG_CONFIG_HOME` when set). Keyboard and gamepad actions have independent remapping buttons; gamepads are enumerated by SDL2 and hotplug events are processed continuously. Mapped inputs are evaluated as keyboard **or** connected controller; unplugging the controller leaves keyboard input available. Controller remapping requires a connected device. The current action names are initial defaults, not verified God Hand hardware mappings.

Display modes: `Windowed` (default), `Fullscreen` (preserve aspect ratio) and `Fullscreen (stretched)`. The window-mode helper exists in the reusable input module. The mode is passed as `GODHAND_SCREEN_MODE` when launching a native game. In-game Esc cycling and actual fullscreen rendering must be wired into the native game loop once one exists; this is **not** evidence of a playable recompilation.

Launcher binary must be built with GTK3, SDL2 and libarchive development libraries. Run `bash godhandLauncher/install_launcher.sh` from the source root, or retrieve a tested GitHub Actions binary from `built/linux-x86_64/`.

No PS2 guest game is currently running. The game execution and graphics wiring remains incomplete; Play reports a missing native executable rather than starting an emulator or a simulated game.
