#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Cloth.hpp"
#include "Camera.hpp"

class ClothGUI
{
public:
    ClothGUI() = default;
    
    void init(GLFWwindow* window, const char* glsl_version = "#version 330")
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();
        
        ImGui_ImplGlfw_InitForOpenGL(window, false);
        ImGui_ImplOpenGL3_Init(glsl_version);
        
        std::cout << "=== ImGui initialized (manual callbacks mode) ===" << std::endl;
        std::cout << "Display size: " << io.DisplaySize.x << "x" << io.DisplaySize.y << std::endl;
    }
    
    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    
    void beginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    
    void render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    void drawClothControls(Cloth* cloth, Camera* camera)
    {
        ImGui::Begin("Cloth Simulation Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Mouse Over GUI: %s", io.WantCaptureMouse ? "YES" : "NO");
        ImGui::Text("Mouse Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
        
        static int clickCount = 0;
        if (ImGui::Button("TEST CLICK ME!"))
        {
            clickCount++;
            std::cout << "Button clicked! Count: " << clickCount << std::endl;
        }
        ImGui::SameLine();
        ImGui::Text("Clicks: %d", clickCount);
        
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Simulation Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Masses: %zu", cloth->getMasses().size());
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Camera Blocked: %s", camera->getCameraBlocked() ? "YES (FPS Mode)" : "NO (GUI Mode)");
            
            if (camera->getCameraBlocked())
            {
                ImGui::TextColored(ImVec4(1,1,0,1), "Press C to unlock camera for GUI!");
            }
        }
        
        if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Toggle Texture (B)"))
                cloth->changeTextureVisible();
            ImGui::SameLine();
            if (ImGui::Button("Toggle Springs (N)"))
                cloth->changeSpringsVisible();
            ImGui::SameLine();
            if (ImGui::Button("Toggle Masses (M)"))
                cloth->changeMassesVisible();
        }
        
        if (ImGui::CollapsingHeader("Forces", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ForceManager& fm = cloth->getForceManager();
            
            if (GravityForce* gravity = fm.getForce<GravityForce>())
            {
                ImGui::Separator();
                ImGui::Text("Gravity");
                bool enabled = gravity->isEnabled();
                if (ImGui::Checkbox("Enable Gravity (G)", &enabled))
                    gravity->setEnabled(enabled);
                
                float g = gravity->getGravity();
                if (ImGui::SliderFloat("Gravity Strength", &g, -20.0f, 0.0f))
                    gravity->setGravity(g);
            }
            
            if (WindForce* wind = fm.getForce<WindForce>())
            {
                ImGui::Separator();
                ImGui::Text("Wind");
                bool enabled = wind->isEnabled();
                if (ImGui::Checkbox("Enable Wind (Shift+W)", &enabled))
                    wind->setEnabled(enabled);
                
                float strength = wind->getStrength();
                if (ImGui::SliderFloat("Wind Strength (+/-)", &strength, 0.0f, 20.0f))
                    wind->setStrength(strength);
                
                glm::vec3 dir = wind->getDirection();
                if (ImGui::SliderFloat3("Wind Direction", &dir.x, -1.0f, 1.0f))
                {
                    if (glm::length(dir) > 0.001f)
                        wind->setDirection(glm::normalize(dir));
                }
                
                if (ImGui::Button("Right (1)")) wind->setDirection(glm::vec3(1, 0, 0));
                ImGui::SameLine();
                if (ImGui::Button("Left (2)")) wind->setDirection(glm::vec3(-1, 0, 0));
                ImGui::SameLine();
                if (ImGui::Button("Forward (3)")) wind->setDirection(glm::vec3(0, 0, 1));
                if (ImGui::Button("Back (4)")) wind->setDirection(glm::vec3(0, 0, -1));
                ImGui::SameLine();
                if (ImGui::Button("Up (5)")) wind->setDirection(glm::vec3(0, 1, 0));
            }
            
            ImGui::Separator();
            ImGui::Text("Explosion Force");
            auto explosions = fm.getForces<RepulsionForce>();
            if (!explosions.empty())
            {
                RepulsionForce* exp = explosions[0];
                bool enabled = exp->isEnabled();
                if (ImGui::Checkbox("Enable Explosion (E)", &enabled))
                    exp->setEnabled(enabled);
                
                glm::vec3 center = exp->getCenter();
                if (ImGui::SliderFloat3("Explosion Center", &center.x, -10.0f, 10.0f))
                    exp->setCenter(center);
                
                float strength = exp->getStrength();
                if (ImGui::SliderFloat("Explosion Strength", &strength, 0.0f, 100.0f))
                    exp->setStrength(strength);
                
                float radius = exp->getRadius();
                if (ImGui::SliderFloat("Explosion Radius", &radius, 1.0f, 15.0f))
                    exp->setRadius(radius);
            }
            else
            {
                if (ImGui::Button("Add Explosion Force (E)"))
                {
                    RepulsionForce* explosion = fm.addForce<RepulsionForce>(
                        glm::vec3(0.0f, 2.5f, 0.0f), 50.0f, 5.0f);
                    explosion->setEnabled(true);
                }
            }
            
            ImGui::Separator();
            ImGui::Text("Attraction Force");
            auto attractions = fm.getForces<AttractionForce>();
            if (!attractions.empty())
            {
                AttractionForce* att = attractions[0];
                bool enabled = att->isEnabled();
                if (ImGui::Checkbox("Enable Attraction (T)", &enabled))
                    att->setEnabled(enabled);
                
                glm::vec3 center = att->getCenter();
                if (ImGui::SliderFloat3("Attraction Center", &center.x, -10.0f, 10.0f))
                    att->setCenter(center);
                
                float strength = att->getStrength();
                if (ImGui::SliderFloat("Attraction Strength", &strength, 0.0f, 50.0f))
                    att->setStrength(strength);
            }
            else
            {
                if (ImGui::Button("Add Attraction Force (T)"))
                {
                    AttractionForce* attraction = fm.addForce<AttractionForce>(
                        glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);
                    attraction->setEnabled(true);
                }
            }
            
            ImGui::Separator();
            ImGui::Text("Oscillating Force");
            auto oscillations = fm.getForces<OscillatingForce>();
            if (!oscillations.empty())
            {
                OscillatingForce* osc = oscillations[0];
                bool enabled = osc->isEnabled();
                if (ImGui::Checkbox("Enable Oscillation (O)", &enabled))
                    osc->setEnabled(enabled);
                
                glm::vec3 dir = osc->getDirection();
                if (ImGui::SliderFloat3("Oscillation Direction", &dir.x, -1.0f, 1.0f))
                {
                    if (glm::length(dir) > 0.001f)
                        osc->setDirection(glm::normalize(dir));
                }
                
                float amplitude = osc->getAmplitude();
                if (ImGui::SliderFloat("Oscillation Amplitude", &amplitude, 0.0f, 20.0f))
                    osc->setAmplitude(amplitude);
                
                float frequency = osc->getFrequency();
                if (ImGui::SliderFloat("Oscillation Frequency", &frequency, 0.1f, 10.0f))
                    osc->setFrequency(frequency);
            }
            else
            {
                if (ImGui::Button("Add Oscillating Force (O)"))
                {
                    OscillatingForce* oscillating = fm.addForce<OscillatingForce>(
                        glm::vec3(1.0f, 0.0f, 0.0f), 3.0f, 2.0f);
                    oscillating->setEnabled(true);
                }
            }
        }
        
        if (ImGui::CollapsingHeader("Actions"))
        {
            if (ImGui::Button("Reset Cloth (R)"))
            {
                cloth->reset();
            }
            
            if (ImGui::Button("Toggle Camera Lock (C)"))
            {
                GLFWwindow* win = glfwGetCurrentContext();
                if (camera->getCameraBlocked())
                {
                    camera->unLockCamera(win);
                    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                else
                {
                    camera->setLockCamera(true);
                    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            
            ImGui::Text("Camera: %s", camera->getCameraBlocked() ? "LOCKED (FPS)" : "UNLOCKED (GUI)");
        }
        
        if (ImGui::CollapsingHeader("Help"))
        {
            ImGui::TextWrapped("Left Click: Grab/Cut cloth");
            ImGui::TextWrapped("W/A/S/D: Move camera");
            ImGui::TextWrapped("Space/Shift: Up/Down");
            ImGui::TextWrapped("Mouse: Look around");
            ImGui::TextWrapped("Scroll: Zoom");
            ImGui::TextWrapped("H: Show full controls in console");
        }
        
        ImGui::End();
    }
};