#include "ImGuiDx12.h"

void ImGuiDx12::Draw(Renderer &r)
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;

    ImGui::Begin("Overlay", nullptr, flags);
    //ImGui::TextUnformatted("Hello");
    //ImGui::Separator();
    //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    //static bool pressed = false;
    //if (ImGui::Button("Toggle Button")) pressed = !pressed;
    //if (pressed) ImGui::Text("Button Pressed");

    for (auto& item : m_items) {
        item->DrawImGui();
	}
    ImGui::End();
    m_imgui.Render(r.GetCommandList());
}
