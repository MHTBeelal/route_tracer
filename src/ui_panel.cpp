#include "imgui.h"
#include "ui_panel.hpp"


void UIPanel::ShowUIPanel()
{
    ImGui::Begin("Karachi Route Tracer");

    ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Pathfinding Controls");
    ImGui::Separator();
    ImGui::Spacing();

    static int mode = 0;  // 0 = Node IDs, 1 = Coordinates
    ImGui::Text("Select Pathfinding Mode:");
    ImGui::RadioButton("Node IDs", &mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Coordinates", &mode, 1);
    ImGui::Spacing();

    if (mode == 0) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Node Search");
        ImGui::InputScalar("Start Node", ImGuiDataType_S64, &m_startNode, nullptr, nullptr, "%lld", ImGuiInputTextFlags_None);
        ImGui::InputScalar("End Node", ImGuiDataType_S64, &m_endNode, nullptr, nullptr, "%lld", ImGuiInputTextFlags_None);
        if (ImGui::Button("Run A* (Node IDs)")) m_runAStarWithNodes = true;
        ImGui::Spacing();
    }

    if (mode == 1) {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Coordinate Search");

        ImGui::Text("Start Location:");
        ImGui::InputFloat("Start Latitude", &m_startLat, 0.0f, 0.0f, "%.6f");
        ImGui::InputFloat("Start Longitude", &m_startLon, 0.0f, 0.0f, "%.6f");
        ImGui::Spacing();

        ImGui::Text("End Location:");
        ImGui::InputFloat("End Latitude", &m_endLat, 0.0f, 0.0f, "%.6f");
        ImGui::InputFloat("End Longitude", &m_endLon, 0.0f, 0.0f, "%.6f");

        ImGui::Spacing();
        if (ImGui::Button("Run A* (Coordinates)")) m_runAStarWithCoords = true;
        ImGui::Spacing();
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0, 0.8, 0.05, 1.0), "Results: ");
    ImGui::Spacing();

    ImGui::Text("Distance: %.3f km", m_distance / 1000.0);
    ImGui::Text("Straight Line Distance: %.3f km", m_straightLineDistance / 1000.0);

    

    ImGui::End();
}