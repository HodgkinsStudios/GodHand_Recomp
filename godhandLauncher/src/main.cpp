#include "installer.hpp"
#include "settings.hpp"
#include "input.hpp"

#include <gtk/gtk.h>

#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

struct Launcher {
    GtkWidget* window = nullptr;
    GtkWidget* title = nullptr;
    GtkWidget* status = nullptr;
    GtkWidget* button = nullptr;
    GtkWidget* progress = nullptr;
    GtkWidget* settings_dialog = nullptr;
    GtkWidget* settings_status = nullptr;
    GtkWidget* binding_buttons[godhand::kActionCount][2] = {};
    int capture_action = -1;
    int capture_kind = -1;
    godhand::Settings configuration;
    std::unique_ptr<godhand::InputManager> input;

    bool installing = false;
    bool installed = false;
    fs::path data_dir;
};

struct Update {
    Launcher* launcher;
    std::string label;
    double progress;
    bool finished = false;
    bool success = false;
};

void show_error(Launcher& app, const char* title, const std::string& detail) {
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app.window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", detail.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void set_install_state(Launcher& app) {
    app.installed = godhand::is_installed(app.data_dir);
    gtk_button_set_label(GTK_BUTTON(app.button), app.installed ? "Play" : "Load God Hand ISO");
    if (!app.installing) {
        gtk_label_set_text(GTK_LABEL(app.status), app.installed
            ? "Installation complete. Ready for an AOT game build."
            : "Select your own God Hand PS2 ISO to install locally.");
    }
}

gboolean on_update(gpointer pointer) {
    std::unique_ptr<Update> update(static_cast<Update*>(pointer));
    Launcher& app = *update->launcher;
    gtk_label_set_text(GTK_LABEL(app.status), update->label.c_str());
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app.progress), update->progress);
    if (update->finished) {
        app.installing = false;
        gtk_widget_set_sensitive(app.button, TRUE);
        set_install_state(app);
        gtk_widget_hide(app.progress);
        if (!update->success) show_error(app, "Installation failed", update->label);
    }
    return G_SOURCE_REMOVE;
}

void post_update(Launcher& app, std::string label, double fraction,
                 bool finished = false, bool success = false) {
    g_idle_add(on_update, new Update{&app, std::move(label), fraction, finished, success});
}

void install_selected(Launcher& app, std::string selected) {
    app.installing = true;
    gtk_widget_set_sensitive(app.button, FALSE);
    gtk_widget_show(app.progress);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app.progress), 0);
    gtk_label_set_text(GTK_LABEL(app.status), "Preparing local installation...");

    std::thread([&app, selected = std::move(selected)] {
        const auto result = godhand::install_iso(selected, app.data_dir,
            [&app](const std::string& message, double percentage) {
                post_update(app, message, percentage);
            });
        post_update(app, result.ok ? "Installation complete" : result.message,
                    result.ok ? 1.0 : 0.0, true, result.ok);
    }).detach();
}

void choose_iso(Launcher& app) {
    GtkFileChooserNative* dialog = gtk_file_chooser_native_new("Select God Hand (PS2) ISO",
        GTK_WINDOW(app.window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open ISO", "_Cancel");
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PlayStation 2 ISO (*.iso)");
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_add_pattern(filter, "*.ISO");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            std::string selected(filename);
            g_free(filename);
            g_object_unref(dialog);
            install_selected(app, std::move(selected));
            return;
        }
    }
    g_object_unref(dialog);
}

void play(Launcher& app) {
    // This program is a launcher only. A God Hand AOT native binary is not yet generated.
    const auto executable = app.data_dir / "bin/GodHand_Recomp_Game";
    std::error_code ec;
    if (!fs::is_regular_file(executable, ec)) {
        show_error(app, "AOT game executable not available",
            "The ISO and assets are installed, but God Hand has not yet been recompiled into a playable native executable.\n\n"
            "The future game executable belongs at:\n" + executable.string());
        return;
    }
    if (access(executable.c_str(), X_OK) != 0) {
        show_error(app, "AOT game executable is not executable",
            "Check the permissions of:\n" + executable.string());
        return;
    }

    // Explicit absolute path, no shell interpolation or command-line injection.
    const std::string program = executable.string();
    gchar* argv[] = {const_cast<gchar*>(program.c_str()), nullptr};
    gchar** environment = g_get_environ();
    gchar** with_data = g_environ_setenv(environment, "GODHAND_DATA_DIR", app.data_dir.c_str(), TRUE);
    g_strfreev(environment);
    gchar** with_settings = g_environ_setenv(with_data, "GODHAND_SETTINGS_FILE", godhand::settings_file().c_str(), TRUE);
    g_strfreev(with_data);
    with_data = g_environ_setenv(with_settings, "GODHAND_SCREEN_MODE", godhand::screen_mode_name(app.configuration.mode), TRUE);
    g_strfreev(with_settings);
    GError* error = nullptr;
    gboolean launched = g_spawn_async(app.data_dir.c_str(), argv, with_data,
        G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, &error);
    g_strfreev(with_data);
    if (!launched) {
        std::string message = error ? error->message : "Unknown process launch error";
        if (error) g_error_free(error);
        show_error(app, "Unable to start God Hand", message);
    } else {
        gtk_label_set_text(GTK_LABEL(app.status), "Started native God Hand executable.");
    }
}

void on_primary_clicked(GtkButton*, gpointer user_data) {
    Launcher& app = *static_cast<Launcher*>(user_data);
    if (app.installing) return;
    if (app.installed) play(app);
    else choose_iso(app);
}

void persist_settings(Launcher& app) {
    if (!godhand::save_settings(godhand::settings_file(), app.configuration))
        show_error(app, "Unable to save settings", "Check permissions under ~/.config/HodgkinsStudios/GodHand_Recomp/");
}

void refresh_binding_label(Launcher& app, int action, int kind) {
    auto* widget = app.binding_buttons[action][kind];
    if (!widget) return;
    const auto& value = kind == 0 ? app.configuration.keyboard[action] : app.configuration.gamepad[action];
    gtk_button_set_label(GTK_BUTTON(widget), value.c_str());
}

void select_binding(GtkButton* button, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    int action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "action"));
    int kind = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "kind"));
    app.capture_action = action;
    app.capture_kind = kind;
    gtk_label_set_text(GTK_LABEL(app.settings_status), kind == 0
        ? "Press a keyboard key (Esc cancels)" : "Press a controller button (Esc cancels)");
    gtk_widget_grab_focus(app.settings_dialog);
}

bool apply_binding(Launcher& app, int action, int kind, const std::string& value) {
    if (action < 0 || action >= godhand::kActionCount || value.empty()) return false;
    if (kind == 0) app.configuration.keyboard[action] = value;
    else app.configuration.gamepad[action] = value;
    refresh_binding_label(app, action, kind);
    app.capture_action = -1;
    app.capture_kind = -1;
    gtk_label_set_text(GTK_LABEL(app.settings_status), "Bindings saved automatically.");
    persist_settings(app);
    return true;
}

gboolean settings_key(GtkWidget*, GdkEventKey* event, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    if (app.capture_action < 0) return FALSE;
    if (event->keyval == GDK_KEY_Escape) {
        app.capture_action = -1;
        gtk_label_set_text(GTK_LABEL(app.settings_status), "Remapping cancelled.");
    } else if (app.capture_kind == 0) {
        const char* name = gdk_keyval_name(event->keyval);
        if (name) {
            SDL_Scancode scan = SDL_GetScancodeFromName(name);
            // GDK uses e.g. 'Shift_L', SDL uses 'Left Shift'. Normalize common differences.
            if (event->keyval == GDK_KEY_Shift_L) scan = SDL_SCANCODE_LSHIFT;
            if (event->keyval == GDK_KEY_Shift_R) scan = SDL_SCANCODE_RSHIFT;
            if (event->keyval == GDK_KEY_Return) scan = SDL_SCANCODE_RETURN;
            if (event->keyval == GDK_KEY_space) scan = SDL_SCANCODE_SPACE;
            if (event->keyval == GDK_KEY_Tab) scan = SDL_SCANCODE_TAB;
            if (scan != SDL_SCANCODE_UNKNOWN)
                apply_binding(app, app.capture_action, 0, SDL_GetScancodeName(scan));
        }
    }
    return TRUE;
}

gboolean input_tick(gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    if (!app.input) return G_SOURCE_CONTINUE;
    app.input->pump();
    if (app.settings_dialog) {
        if (app.capture_action >= 0 && app.capture_kind == 1) {
            std::string button;
            if (app.input->poll_capture_button(button))
                apply_binding(app, app.capture_action, 1, button);
        } else {
            // Discard old captured input before initiating another remap.
            std::string discarded;
            app.input->poll_capture_button(discarded);
        }
    }
    return G_SOURCE_CONTINUE;
}

void on_mode_changed(GtkComboBox* combo, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    int index = gtk_combo_box_get_active(combo);
    if (index >= 0 && index < 3) {
        app.configuration.mode = static_cast<godhand::ScreenMode>(index);
        persist_settings(app);
    }
}

void reset_bindings(GtkButton*, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    auto old_mode = app.configuration.mode;
    app.configuration = godhand::Settings{};
    app.configuration.mode = old_mode;
    for (int i = 0; i < godhand::kActionCount; ++i)
        for (int k = 0; k < 2; ++k) refresh_binding_label(app, i, k);
    persist_settings(app);
    gtk_label_set_text(GTK_LABEL(app.settings_status), "Keyboard and controller bindings restored.");
}

void on_settings_destroy(GtkWidget*, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    app.settings_dialog = nullptr;
    app.settings_status = nullptr;
    app.capture_action = -1;
    for (auto& row : app.binding_buttons) row[0] = row[1] = nullptr;
}

void show_settings(GtkButton*, gpointer userdata) {
    auto& app = *static_cast<Launcher*>(userdata);
    if (app.settings_dialog) {gtk_window_present(GTK_WINDOW(app.settings_dialog));return;}
    GtkWidget* dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    app.settings_dialog = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "GodHand:Recomp — Settings");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 660, 580);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app.window));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 16);
    gtk_widget_add_events(dialog, GDK_KEY_PRESS_MASK);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(settings_key), &app);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_settings_destroy), &app);

    GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(dialog), outer);
    GtkWidget* screen_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(outer), screen_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(screen_row), gtk_label_new("Screen mode"), FALSE, FALSE, 0);
    GtkWidget* combo = gtk_combo_box_text_new();
    for (int i = 0; i < 3; ++i)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), godhand::screen_mode_name(static_cast<godhand::ScreenMode>(i)));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), static_cast<int>(app.configuration.mode));
    g_signal_connect(combo, "changed", G_CALLBACK(on_mode_changed), &app);
    gtk_box_pack_start(GTK_BOX(screen_row), combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(outer), gtk_label_new("Fullscreen cycles with Esc when the native game runtime is integrated."), FALSE, FALSE, 0);

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(outer), scroll, TRUE, TRUE, 0);
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
    gtk_container_add(GTK_CONTAINER(scroll), grid);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Action"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Keyboard"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Controller"), 2, 0, 1, 1);
    for (int i = 0; i < godhand::kActionCount; ++i) {
        GtkWidget* action = gtk_label_new(godhand::action_defaults[i].name);
        gtk_widget_set_halign(action, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), action, 0, i + 1, 1, 1);
        for (int kind = 0; kind < 2; ++kind) {
            GtkWidget* btn = gtk_button_new();
            gtk_widget_set_size_request(btn, 155, 38);
            app.binding_buttons[i][kind] = btn;
            refresh_binding_label(app, i, kind);
            g_object_set_data(G_OBJECT(btn), "action", GINT_TO_POINTER(i));
            g_object_set_data(G_OBJECT(btn), "kind", GINT_TO_POINTER(kind));
            g_signal_connect(btn, "clicked", G_CALLBACK(select_binding), &app);
            gtk_grid_attach(GTK_GRID(grid), btn, kind + 1, i + 1, 1, 1);
        }
    }
    app.settings_status = gtk_label_new(app.input && app.input->initialized()
        ? "Controller auto-detection enabled. Select any binding to remap."
        : "Controller unavailable (SDL initialization failed); keyboard remapping works.");
    gtk_label_set_line_wrap(GTK_LABEL(app.settings_status), TRUE);
    gtk_box_pack_start(GTK_BOX(outer), app.settings_status, FALSE, FALSE, 0);
    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(outer), controls, FALSE, FALSE, 0);
    GtkWidget* defaults = gtk_button_new_with_label("Restore default bindings");
    g_signal_connect(defaults, "clicked", G_CALLBACK(reset_bindings), &app);
    gtk_box_pack_start(GTK_BOX(controls), defaults, TRUE, TRUE, 0);
    GtkWidget* close = gtk_button_new_with_label("Back to launcher");
    g_signal_connect_swapped(close, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_box_pack_start(GTK_BOX(controls), close, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
}

void use_stylesheet() {
    static const char* css = R"CSS(
        window { background: #121820; }
        #brand { font-family: Sans; font-size: 31px; font-weight: 800; color: #f3f6f9; }
        #credit { font-size: 15px; font-weight: 600; color: #aebfce; }
        #tagline { font-size: 15px; color: #c9d4de; }
        #status { font-size: 13px; color: #9db1c0; }
        #primary { background-image: none; background: #ce4039;
                   color: #ffffff; border-radius: 8px; border: 0;
                   font-size: 18px; font-weight: 700; padding: 14px 30px;
                   min-height: 32px; }
        #primary:hover { background: #e6574c; }
        #secondary { color: #f3f6f9; font-size: 16px; min-height: 38px; padding: 8px 30px; }

        #primary:disabled { background: #6d747c; color: #d7dde4; }
        progressbar trough { min-height: 9px; background: #303d49; border-radius: 4px; }
        progressbar progress { background: #ce4039; border-radius: 4px; }
    )CSS";
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, nullptr);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

GtkWidget* label(const char* text, const char* id) {
    GtkWidget* widget = gtk_label_new(text);
    gtk_widget_set_name(widget, id);
    gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
    return widget;
}

} // namespace

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);
    Launcher app;
    try {
        app.data_dir = godhand::data_directory();
    } catch (const std::exception& e) {
        g_printerr("GodHand:Recomp launcher error: %s\n", e.what());
        return 1;
    }
    godhand::load_settings(godhand::settings_file(), app.configuration);
    app.input = std::make_unique<godhand::InputManager>();
    use_stylesheet();

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "GodHand:Recomp | HodgkinsStudios");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 720, 450);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(app.window), 40);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GtkWidget* vertical = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app.window), vertical);
    GtkWidget* spacer_top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer_top, TRUE);
    gtk_box_pack_start(GTK_BOX(vertical), spacer_top, TRUE, TRUE, 0);

    app.title = label("GodHand:Recomp", "brand");
    gtk_box_pack_start(GTK_BOX(vertical), app.title, FALSE, FALSE, 9);
    GtkWidget* credit = label("HodgkinsStudios", "credit");
    gtk_box_pack_start(GTK_BOX(vertical), credit, FALSE, FALSE, 8);
    GtkWidget* tagline = label("An experimental PlayStation 2 static recompilation project", "tagline");
    gtk_box_pack_start(GTK_BOX(vertical), tagline, FALSE, FALSE, 30);

    app.button = gtk_button_new_with_label("Load God Hand ISO");
    gtk_widget_set_name(app.button, "primary");
    gtk_widget_set_halign(app.button, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(app.button, 290, 64);
    g_signal_connect(app.button, "clicked", G_CALLBACK(on_primary_clicked), &app);
    gtk_box_pack_start(GTK_BOX(vertical), app.button, FALSE, FALSE, 12);

    GtkWidget* settings_btn = gtk_button_new_with_label("Settings");
    gtk_widget_set_name(settings_btn, "secondary");
    gtk_widget_set_size_request(settings_btn, 210, 48);
    gtk_widget_set_halign(settings_btn, GTK_ALIGN_CENTER);
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(show_settings), &app);
    gtk_box_pack_start(GTK_BOX(vertical), settings_btn, FALSE, FALSE, 6);

    app.progress = gtk_progress_bar_new();
    gtk_widget_set_size_request(app.progress, 420, 12);
    gtk_widget_set_halign(app.progress, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vertical), app.progress, FALSE, FALSE, 10);

    app.status = label("", "status");
    gtk_box_pack_start(GTK_BOX(vertical), app.status, FALSE, FALSE, 16);
    GtkWidget* spacer_bottom = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer_bottom, TRUE);
    gtk_box_pack_start(GTK_BOX(vertical), spacer_bottom, TRUE, TRUE, 0);

    set_install_state(app);
    gtk_widget_show_all(app.window);
    gtk_widget_hide(app.progress);
    g_timeout_add(50, input_tick, &app);
    gtk_main();
    return 0;
}
