#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Cloth.hpp"
#include "Camera.hpp"
#include <glm/glm.hpp>

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
    
    void drawClothControls(Cloth* cloth, Camera* camera, glm::vec3* lightPos)
    {
        if (camera->getCameraBlocked())
            return;
        
        ImGui::Begin("Cloth Simulation Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
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

        if (ImGui::CollapsingHeader("Tension Breaking", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Spring Breaking System");
            ImGui::Separator();
            
            bool tensionEnabled = cloth->getEnableTensionBreaking();
            if (ImGui::Checkbox("Enable Tension Breaking (T)", &tensionEnabled))
            {
                cloth->setEnableTensionBreaking(tensionEnabled);
            }
            
            if (tensionEnabled)
            {
                float threshold = cloth->getTensionBreakThreshold();
                if (ImGui::SliderFloat("Break Threshold ([/])", &threshold, 2.0f, 10.0f, "%.2fx"))
                {
                    cloth->setTensionBreakThreshold(threshold);
                }
                ImGui::TextWrapped("Threshold: How much a spring can stretch before breaking (e.g., 3.5x = 350%% of rest length)");
                ImGui::Text("Current: %.2fx (springs break after 3 frames at this stretch)", threshold);
            }
        }

        if (ImGui::CollapsingHeader("Physical Properties"))
        {
            ImGui::Text("These settings require cloth reset to apply");
            ImGui::Separator();
            
            static float massValue = 0.5f;
            static float structuralStiff = 100.0f;
            static float structuralDamping = 1.5f;
            static float shearStiff = 200.0f;
            static float shearDamping = 1.0f;
            static float bendingStiff = 100.0f;
            static float bendingDamping = 0.8f;
            
            ImGui::Text("Mass Properties:");
            ImGui::SliderFloat("Point Mass", &massValue, 0.1f, 5.0f, "%.2f");
            ImGui::TextWrapped("Mass of each point in the cloth");
            
            ImGui::Separator();
            ImGui::Text("Structural Springs (Main fabric):");
            ImGui::SliderFloat("Structural Stiffness", &structuralStiff, 10.0f, 500.0f, "%.1f");
            ImGui::SliderFloat("Structural Damping", &structuralDamping, 0.1f, 5.0f, "%.2f");
            
            ImGui::Separator();
            ImGui::Text("Shear Springs (Diagonal resistance):");
            ImGui::SliderFloat("Shear Stiffness", &shearStiff, 10.0f, 500.0f, "%.1f");
            ImGui::SliderFloat("Shear Damping", &shearDamping, 0.1f, 5.0f, "%.2f");
            
            ImGui::Separator();
            ImGui::Text("Bending Springs (Folding resistance):");
            ImGui::SliderFloat("Bending Stiffness", &bendingStiff, 10.0f, 500.0f, "%.1f");
            ImGui::SliderFloat("Bending Damping", &bendingDamping, 0.1f, 5.0f, "%.2f");
            
            ImGui::Separator();
            
            if (ImGui::Button("Soft Silk"))
            {
                massValue = 0.3f;
                structuralStiff = 50.0f;
                shearStiff = 80.0f;
                bendingStiff = 30.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Normal Fabric"))
            {
                massValue = 0.5f;
                structuralStiff = 100.0f;
                shearStiff = 200.0f;
                bendingStiff = 100.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Heavy Canvas"))
            {
                massValue = 1.5f;
                structuralStiff = 300.0f;
                shearStiff = 400.0f;
                bendingStiff = 250.0f;
            }
            
            if (ImGui::Button("Rubber Sheet"))
            {
                massValue = 0.8f;
                structuralStiff = 150.0f;
                shearStiff = 150.0f;
                bendingStiff = 80.0f;
                structuralDamping = 3.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Steel Mesh"))
            {
                massValue = 2.0f;
                structuralStiff = 500.0f;
                shearStiff = 500.0f;
                bendingStiff = 400.0f;
            }
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "Note: Click 'Apply & Reset' to use new values");
            
            if (ImGui::Button("Apply & Reset Cloth", ImVec2(-1, 40)))
            {
                cloth->setPhysicalProperties(massValue, 
                                            structuralStiff, structuralDamping,
                                            shearStiff, shearDamping,
                                            bendingStiff, bendingDamping);
                cloth->reset();
                std::cout << "Applied new physical properties and reset cloth\n";
            }
        }

        // Cutting Parameters
        if (ImGui::CollapsingHeader("Cutting Parameters"))
        {
            static float cutThreshold = 10.0f;
            
            ImGui::Text("Cut Precision:");
            ImGui::SliderFloat("Cut Radius (pixels)", &cutThreshold, 5.0f, 50.0f, "%.1f");
            ImGui::TextWrapped("How precise the cutting is. Lower = more precise, higher = easier to cut");
            
            cloth->setCutThreshold(cutThreshold);
            
            ImGui::Separator();
            
            if (ImGui::Button("Very Precise (5px)"))
                cutThreshold = 5.0f;
            ImGui::SameLine();
            if (ImGui::Button("Normal (10px)"))
                cutThreshold = 10.0f;
            ImGui::SameLine();
            if (ImGui::Button("Easy (20px)"))
                cutThreshold = 20.0f;
        }

        if (ImGui::CollapsingHeader("Constraint Solver"))
        {
            static int solverIterations = 5;
            static float correctionFactor = 0.15f;
            static float maxStretchRatio = 1.2f;
            
            ImGui::Text("Solver Settings:");
            ImGui::SliderInt("Iterations", &solverIterations, 1, 20);
            ImGui::TextWrapped("More iterations = more stable but slower");
            
            ImGui::SliderFloat("Correction Factor", &correctionFactor, 0.01f, 0.5f, "%.3f");
            ImGui::TextWrapped("How quickly springs correct their length");
            
            ImGui::SliderFloat("Max Stretch Ratio", &maxStretchRatio, 1.1f, 2.0f, "%.2f");
            ImGui::TextWrapped("Maximum allowed stretch before hard constraint");
            
            cloth->setSolverParameters(solverIterations, correctionFactor, maxStretchRatio);
            
            ImGui::Separator();
            
            if (ImGui::Button("Stable (10 iter)"))
            {
                solverIterations = 10;
                correctionFactor = 0.15f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Fast (3 iter)"))
            {
                solverIterations = 3;
                correctionFactor = 0.25f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Very Stable (15 iter)"))
            {
                solverIterations = 15;
                correctionFactor = 0.1f;
            }
        }

        if (ImGui::CollapsingHeader("Lighting"))
        {
            ImGui::Text("Light Source Position");
            ImGui::SliderFloat("Light X", &lightPos->x, -20.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Light Y", &lightPos->y, -5.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Light Z", &lightPos->z, -20.0f, 20.0f, "%.1f");
            
            ImGui::Separator();
            
            if (ImGui::Button("Top Center"))
            {
                *lightPos = glm::vec3(0.0f, 15.0f, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Front"))
            {
                *lightPos = glm::vec3(0.0f, 5.0f, 10.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Back"))
            {
                *lightPos = glm::vec3(0.0f, 5.0f, -10.0f);
            }
            
            if (ImGui::Button("Left"))
            {
                *lightPos = glm::vec3(-10.0f, 5.0f, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Right"))
            {
                *lightPos = glm::vec3(10.0f, 5.0f, 5.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Default"))
            {
                *lightPos = glm::vec3(5.0f, 10.0f, 5.0f);
            }
            
            ImGui::Separator();
            
            ImGui::Text("Artistic Presets:");
            if (ImGui::Button("Dramatic (Low Side)"))
            {
                *lightPos = glm::vec3(8.0f, 1.0f, 5.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Sunset"))
            {
                *lightPos = glm::vec3(-15.0f, 3.0f, -10.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Overhead"))
            {
                *lightPos = glm::vec3(0.0f, 18.0f, 2.0f);
            }
            
            ImGui::Separator();
            ImGui::Text("Current Position: (%.1f, %.1f, %.1f)", 
                       lightPos->x, lightPos->y, lightPos->z);
        }

        if (ImGui::CollapsingHeader("Cloth Dimensions"))
        {
            static float newWidth = cloth->getClothWidth();
            static float newHeight = cloth->getClothHeight();
            static int newResX = 50;
            static int newResY = 50;
            
            ImGui::Text("Current Size: %.1fx%.1f", cloth->getClothWidth(), cloth->getClothHeight());
            ImGui::Separator();
            
            ImGui::SliderFloat("Width", &newWidth, 1.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Height", &newHeight, 1.0f, 20.0f, "%.1f");
            
            ImGui::Separator();
            ImGui::Text("Resolution (Number of Masses)");
            ImGui::SliderInt("Horizontal (X)", &newResX, 5, 100);
            ImGui::SliderInt("Vertical (Y)", &newResY, 5, 100);
            
            ImGui::Text("Total masses: %d", newResX * newResY);
            
            if (newResX * newResY > 5000)
            {
                ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "Warning: High mass count may affect performance!");
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("Small (25x25)"))
            {
                newWidth = 3.0f;
                newHeight = 3.0f;
                newResX = 25;
                newResY = 25;
            }
            ImGui::SameLine();
            if (ImGui::Button("Medium (50x50)"))
            {
                newWidth = 5.0f;
                newHeight = 5.0f;
                newResX = 50;
                newResY = 50;
            }
            ImGui::SameLine();
            if (ImGui::Button("Large (75x75)"))
            {
                newWidth = 8.0f;
                newHeight = 8.0f;
                newResX = 75;
                newResY = 75;
            }
            
            if (ImGui::Button("Fine Detail (100x100)"))
            {
                newWidth = 5.0f;
                newHeight = 5.0f;
                newResX = 100;
                newResY = 100;
            }
            ImGui::SameLine();
            if (ImGui::Button("Flag (80x40)"))
            {
                newWidth = 8.0f;
                newHeight = 4.0f;
                newResX = 80;
                newResY = 40;
            }
            ImGui::SameLine();
            if (ImGui::Button("Curtain (30x80)"))
            {
                newWidth = 3.0f;
                newHeight = 8.0f;
                newResX = 30;
                newResY = 80;
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("Apply New Size", ImVec2(-1, 40)))
            {
                cloth->resize(newWidth, newHeight, newResX, newResY);
                std::cout << "Cloth resized to " << newWidth << "x" << newHeight 
                          << " with " << newResX << "x" << newResY << " masses" << std::endl;
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
                if (ImGui::Checkbox("Enable Wind (P)", &enabled))
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
            if (ImGui::Button("Reset Cloth (R)", ImVec2(200, 30)))
            {
                cloth->reset();
            }
            
            if (ImGui::Button("Lock Camera for FPS (C)", ImVec2(200, 30)))
            {
                GLFWwindow* win = glfwGetCurrentContext();
                camera->setLockCamera(true);
                glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        
        if (ImGui::CollapsingHeader("Help"))
        {
            ImGui::TextWrapped("C - Toggle camera (FPS/GUI mode)");
            ImGui::TextWrapped("Left Click - Grab/Cut cloth (GUI mode)");
            ImGui::TextWrapped("W/A/S/D - Move camera (FPS mode)");
            ImGui::TextWrapped("Mouse - Look around (FPS mode)");
            ImGui::TextWrapped("Scroll - Zoom");
            ImGui::TextWrapped("T - Toggle tension breaking");
            ImGui::TextWrapped("[ / ] - Adjust break threshold");
            ImGui::TextWrapped("H - Show full controls in console");
        }
        
        ImGui::End();
    }
};
