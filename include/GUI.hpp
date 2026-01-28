#pragma once

#include <imgui.h> // ImVec4

struct GLFWwindow;
struct AppData;
struct AnalysisDisplayData;

class ClothGUI
{
  public:
    ClothGUI();

    void init(GLFWwindow *window, const char *glsl_version);
    void shutdown();

    void beginFrame();
    void render();

    void drawClothControls(AppData *appData);
    void drawAnalysisWindow(const AnalysisDisplayData &data);
    void drawRealTimePlots(const AnalysisDisplayData &data);

  private:
    bool showAnalysisWindow;
    bool showPlotsWindow;
    bool showEventLog;
    bool autoScrollEvents;

    ImVec4 velocityColor;
    ImVec4 energyColor;
    ImVec4 tensionColor;
    ImVec4 forceColor;
};
