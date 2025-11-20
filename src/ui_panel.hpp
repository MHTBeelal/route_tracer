#ifndef IMGUI_PANEL_HPP
#define IMGUI_PANEL_HPP

#include <imgui.h>
#include <cstdint>

struct UIPanel {

    int64_t m_startNode = 0, m_endNode = 0;
    bool m_runAStarWithNodes = false;
    bool m_runAStarWithCoords = false;

    float m_startLat = 24.8600f, m_startLon = 67.0100f;
    float m_endLat = 24.8700f, m_endLon = 67.0200f;

    char m_searchBuffer[128] = "";
    bool m_searchRequested = false;

    float m_distance = 0, m_straightLineDistance = 0;


    void ShowUIPanel();
};

#endif
