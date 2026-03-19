#include "debug/debug_ui.hpp"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "imgui.h"

namespace z1m {

namespace {

void apply_debug_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0F;
    style.FrameRounding = 3.0F;
    style.GrabRounding = 3.0F;
    style.WindowBorderSize = 1.0F;
    style.FrameBorderSize = 0.0F;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.07F, 0.09F, 0.12F, 0.94F);
    colors[ImGuiCol_Header] = ImVec4(0.18F, 0.28F, 0.22F, 1.0F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22F, 0.36F, 0.28F, 1.0F);
    colors[ImGuiCol_Button] = ImVec4(0.18F, 0.28F, 0.22F, 1.0F);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24F, 0.38F, 0.30F, 1.0F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.14F, 0.23F, 0.18F, 1.0F);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12F, 0.15F, 0.18F, 1.0F);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10F, 0.13F, 0.16F, 1.0F);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10F, 0.13F, 0.16F, 1.0F);
}

void render_current_room_warps(const AppState* app) {
    if (app->session.area_kind != AreaKind::Overworld) {
        ImGui::TextUnformatted("cave room active");
        return;
    }

    std::array<OverworldWarp, kMaxRoomWarps> warps = {};
    const int warp_count =
        gather_overworld_warps(&app->world.overworld, app->session.current_room_id, &warps);
    if (warp_count <= 0) {
        ImGui::TextUnformatted("no room warp metadata");
        return;
    }

    for (int index = 0; index < warp_count; ++index) {
        const OverworldWarp& warp = warps[static_cast<std::size_t>(index)];
        ImGui::PushID(index);
        ImGui::SeparatorText(overworld_warp_type_name(warp.type));
        ImGui::Text("cave=%02X tile=%02X visible=%s stairs=%s", warp.cave_id, warp.tile,
                    warp.visible ? "yes" : "no", warp.uses_stairs ? "yes" : "no");
        ImGui::Text("trigger=(%.2f, %.2f) size=(%.2f, %.2f)", warp.trigger_position.x,
                    warp.trigger_position.y, warp.trigger_half_size.x, warp.trigger_half_size.y);
        ImGui::Text("return=(%.2f, %.2f)", warp.return_position.x, warp.return_position.y);
        ImGui::PopID();
    }
}

void render_travel_buttons(AppState* app) {
    if (ImGui::Button("Overworld")) {
        set_area_kind(&app->session, &app->player, AreaKind::Overworld, -1,
                      get_opening_start_position());
    }
    ImGui::SameLine();
    if (ImGui::Button("Enemy Zoo")) {
        set_area_kind(&app->session, &app->player, AreaKind::EnemyZoo, -1, glm::vec2(10.0F, 10.0F));
    }
    ImGui::SameLine();
    if (ImGui::Button("Item Zoo")) {
        set_area_kind(&app->session, &app->player, AreaKind::ItemZoo, -1, glm::vec2(10.0F, 10.0F));
    }
}

} // namespace

bool init_debug_ui(AppState* app) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    apply_debug_style();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(app->window, app->renderer)) {
        return false;
    }

    if (!ImGui_ImplSDLRenderer3_Init(app->renderer)) {
        ImGui_ImplSDL3_Shutdown();
        return false;
    }

    return true;
}

void shutdown_debug_ui() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void debug_ui_process_event(const SDL_Event* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGui_ImplSDL3_ProcessEvent(event);
}

bool debug_ui_wants_keyboard_capture() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return false;
    }

    return ImGui::GetIO().WantCaptureKeyboard;
}

void begin_debug_ui_frame() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void render_debug_ui(AppState* app) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    if (app->debug_view.show_ui) {
        ImGui::SetNextWindowPos(ImVec2(8.0F, 8.0F), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340.0F, 440.0F), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Debug Toggles", &app->debug_view.show_ui)) {
            ImGui::Checkbox("Show Debug UI Window", &app->debug_view.show_ui);
            ImGui::Checkbox("Show Debug Overlay", &app->debug_view.enabled);
            ImGui::Checkbox("Show Hitboxes", &app->debug_view.show_hitboxes);
            ImGui::Checkbox("Show Collision Tiles", &app->debug_view.show_collision_tiles);
            ImGui::Checkbox("Show Interactables", &app->debug_view.show_interactables);
            ImGui::Checkbox("Show Labels", &app->debug_view.show_labels);

            ImGui::SeparatorText("Travel");
            render_travel_buttons(app);

            ImGui::SeparatorText("Session");
            ImGui::Text("area=%s room=%d cave=%d", area_name(&app->session),
                        app->session.current_room_id, app->session.current_cave_id);
            ImGui::Text("player=(%.2f, %.2f) hp=%d/%d", app->player.position.x,
                        app->player.position.y, app->player.health, app->player.max_health);
            ImGui::Text("rupees=%d bombs=%d keys=%d", app->player.rupees, app->player.bombs,
                        app->player.keys);
            ImGui::Text("sword=%s item=%s", app->player.has_sword ? "yes" : "no",
                        use_item_name(app->player.selected_item));
            ImGui::Text("boomerang=%s bow=%s candle=%s potion=%s",
                        app->player.has_boomerang ? "yes" : "no",
                        app->player.has_bow ? "yes" : "no", app->player.has_candle ? "yes" : "no",
                        app->player.has_potion ? "yes" : "no");

            ImGui::SeparatorText("Current Room Warps");
            render_current_room_warps(app);
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), app->renderer);
}

} // namespace z1m
