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

    for (auto& item : m_items) {
        item->DrawImGui();
	}
    ImGui::End();
    m_imgui.Render(r.GetCommandList());
}
