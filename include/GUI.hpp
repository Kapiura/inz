#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Cloth.hpp"
#include "Camera.hpp"
#include "Force.hpp"
#include "AnalysisData.hpp"
#include <glm/glm.hpp>
#include <algorithm>  
#include <sstream>
#include <iomanip>
#include <fstream>

class ClothGUI
{
public:
    ClothGUI() 
    : showAnalysisWindow(false), 
      showPlotsWindow(false), 
      showEventLog(false), 
      autoScrollEvents(true),
      velocityColor(ImVec4(0.2f, 0.8f, 0.3f, 1.0f)),
      energyColor(ImVec4(1.0f, 0.7f, 0.0f, 1.0f)),
      tensionColor(ImVec4(1.0f, 0.2f, 0.2f, 1.0f)),
      forceColor(ImVec4(0.3f, 0.5f, 1.0f, 1.0f))
    {
    }
    
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

        if (ImGui::CollapsingHeader("Analysis Windows", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Data Analysis & Monitoring");
            ImGui::Separator();
            
            if (ImGui::Button("Point Analysis", ImVec2(150, 0)))
                showAnalysisWindow = !showAnalysisWindow;
            ImGui::SameLine();
            if (showAnalysisWindow)
                ImGui::TextColored(ImVec4(0,1,0,1), "[ON]");
            else
                ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "[OFF]");
            
            if (ImGui::Button("Real-Time Plots", ImVec2(150, 0)))
                showPlotsWindow = !showPlotsWindow;
            ImGui::SameLine();
            if (showPlotsWindow)
                ImGui::TextColored(ImVec4(0,1,0,1), "[ON]");
            else
                ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "[OFF]");
            
            if (ImGui::Button("Event Log", ImVec2(150, 0)))
                showEventLog = !showEventLog;
            ImGui::SameLine();
            if (showEventLog)
                ImGui::TextColored(ImVec4(0,1,0,1), "[ON]");
            else
                ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "[OFF]");
            
            ImGui::Separator();
            
            ClothAnalysis& analysis = cloth->getAnalysis();
            bool recording = analysis.isRecordingEnabled();
            if (ImGui::Checkbox("Record Data", &recording))
                analysis.setRecordingEnabled(recording);
            
            if (recording)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "● REC");
            }
            
            ImGui::TextWrapped("Select a mass point to record its data");
            
            if (ImGui::Button("Clear History", ImVec2(150, 0)))
            {
                analysis.clearHistory();
                std::cout << "Analysis history cleared\n";
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Export CSV", ImVec2(150, 0)))
            {
                std::string csvData = analysis.exportToCSV();
                std::ofstream file("cloth_analysis.csv");
                if (file.is_open())
                {
                    file << csvData;
                    file.close();
                    std::cout << "Data exported to cloth_analysis.csv\n";
                }
                else
                {
                    std::cout << "Failed to export CSV\n";
                }
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
            static float correctionFactor = 0.2f;
            static float maxStretchRatio = 1.15f;
            
            ImGui::Text("Solver Settings:");
            ImGui::SliderInt("Iterations", &solverIterations, 1, 15);
            ImGui::TextWrapped("Lower = less jitter, Higher = more stable");
            
            ImGui::SliderFloat("Correction Factor", &correctionFactor, 0.1f, 0.5f, "%.2f");
            ImGui::TextWrapped("Lower = softer cloth, less jitter");
            
            ImGui::SliderFloat("Max Stretch", &maxStretchRatio, 1.05f, 1.5f, "%.2f");
            ImGui::TextWrapped("How much springs can stretch");
            
            cloth->setSolverParameters(solverIterations, correctionFactor, maxStretchRatio);
            
            ImGui::Separator();
            
            if (ImGui::Button("Stable (5 iter, soft)"))
            {
                solverIterations = 5;
                correctionFactor = 0.2f;
                maxStretchRatio = 1.15f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stiff (10 iter)"))
            {
                solverIterations = 10;
                correctionFactor = 0.3f;
                maxStretchRatio = 1.08f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Very Soft"))
            {
                solverIterations = 3;
                correctionFactor = 0.15f;
                maxStretchRatio = 1.3f;
            }
            
            ImGui::Separator();
            ImGui::TextWrapped("Tip: Lower values = less jitter but softer cloth");
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
    
        if (ImGui::CollapsingHeader("Collision Settings"))
        {
            ImGui::Text("Self-Collision (Mass Spheres)");
            ImGui::Separator();
            
            const auto& masses = cloth->getMasses();
            int massCount = masses.size();
            
            bool selfCollision = cloth->getEnableSelfCollision();
            
            if (massCount > 2000)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                                  "Too many masses (%d > 2000)", massCount);
                if (selfCollision)
                {
                    cloth->setEnableSelfCollision(false);
                }
            }
            else
            {
                if (ImGui::Checkbox("Enable Self-Collision", &selfCollision))
                {
                    cloth->setEnableSelfCollision(selfCollision);
                }
                
                if (selfCollision)
                {
                    float collisionRadius = cloth->getSelfCollisionRadius();
                    if (ImGui::SliderFloat("Sphere Radius", &collisionRadius, 0.02f, 0.1f, "%.3f"))
                    {
                        cloth->setSelfCollisionRadius(collisionRadius);
                    }
                    ImGui::TextWrapped("Each mass has a sphere of this radius");
                    ImGui::TextWrapped("Neighboring spheres don't collide");
                    ImGui::Text("Recommended: 0.03 - 0.05");
                    
                    if (massCount > 1500)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                                          "Warning: May be slow with %d masses", massCount);
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::Text("Sphere Collision");
            
            if (ImGui::Button("Add Collision Sphere"))
            {
                // Dodaj kulę nad tkaniną
                auto sphere = std::make_unique<Sphere>(glm::vec3(0.0f, 2.0f, 0.0f), 1.0f);
                cloth->addCollisionObject(std::move(sphere));
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Clear All"))
            {
                cloth->clearCollisionObjects();
            }
            
            auto objects = cloth->getCollisionObjects();
            
            if (!objects.empty())
            {
                ImGui::Separator();
                ImGui::Text("Active Spheres: %zu", objects.size());
                
                static int sphereToDelete = -1;
                
                for (size_t i = 0; i < objects.size(); i++)
                {
                    if (Sphere* sphere = dynamic_cast<Sphere*>(objects[i]))
                    {
                        ImGui::PushID(i);
                        
                        std::string label = "Sphere #" + std::to_string(i);
                        if (ImGui::TreeNode(label.c_str()))
                        {
                            glm::vec3 pos = sphere->getPosition();
                            if (ImGui::SliderFloat3("Position", &pos.x, -8.0f, 8.0f))
                            {
                                sphere->setPosition(pos);
                            }
                            
                            float radius = sphere->getRadius();
                            if (ImGui::SliderFloat("Radius", &radius, 0.3f, 4.0f))
                            {
                                sphere->setRadius(radius);
                            }
                            
                            ImGui::Text("Visual = Collision radius");
                            ImGui::Separator();
                            
                            if (ImGui::Button("Delete", ImVec2(-1, 0)))
                            {
                                sphereToDelete = i;
                            }
                            
                            ImGui::TreePop();
                        }
                        
                        ImGui::PopID();
                    }
                }
                
                if (sphereToDelete >= 0)
                {
                    cloth->removeCollisionObject(sphereToDelete);
                    sphereToDelete = -1;
                }
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

        AnalysisDisplayData analysisData = cloth->getAnalysisDisplayData();
        
        if (showAnalysisWindow)
            drawAnalysisWindow(analysisData);
        
        if (showPlotsWindow)
            drawRealTimePlots(analysisData);
        
        if (showEventLog)
            drawEventLog(cloth);
    }


    void drawAnalysisWindow(const AnalysisDisplayData& data)
{
    ImGui::Begin("Point Analysis", &showAnalysisWindow, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (data.selectedMassIndex < 0)
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                          "No point selected");
        ImGui::TextWrapped("Click on a mass point in GUI mode to analyze it.");
        ImGui::Text("Press C to unlock camera for point selection.");
        ImGui::End();
        return;
    }
    
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                      "Selected Mass: #%d", data.selectedMassIndex);
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("X: %8.3f", data.position.x);
        ImGui::Text("Y: %8.3f", data.position.y);
        ImGui::Text("Z: %8.3f", data.position.z);
    }
    
    if (ImGui::CollapsingHeader("Motion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(velocityColor, "Velocity");
        ImGui::Text("  VX: %7.3f m/s", data.velocity.x);
        ImGui::Text("  VY: %7.3f m/s", data.velocity.y);
        ImGui::Text("  VZ: %7.3f m/s", data.velocity.z);
        ImGui::TextColored(velocityColor, "Speed: %.3f m/s", data.speed);
        
        ImGui::Separator();
        
        ImGui::TextColored(forceColor, "Acceleration");
        ImGui::Text("  AX: %7.3f m/s²", data.acceleration.x);
        ImGui::Text("  AY: %7.3f m/s²", data.acceleration.y);
        ImGui::Text("  AZ: %7.3f m/s²", data.acceleration.z);
        
        float accMag = glm::length(data.acceleration);
        ImGui::TextColored(forceColor, "Magnitude: %.3f m/s²", accMag);
    }
    
    if (ImGui::CollapsingHeader("Energy", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(energyColor, "Kinetic Energy: %.4f J", data.kineticEnergy);
        
        float energyNormalized = std::min(data.kineticEnergy / 10.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
        ImGui::ProgressBar(energyNormalized, ImVec2(-1, 0), "");
        ImGui::PopStyleColor();
    }
    
    if (ImGui::CollapsingHeader("Structure", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Connected Springs: %d", data.connectedSprings);
        
        ImGui::Separator();
        
        ImGui::TextColored(tensionColor, "Avg Tension: %.3f", data.averageTension);
        
        float tensionNormalized = std::min(data.averageTension / 2.0f, 1.0f);
        
        ImVec4 tensionBarColor;
        if (tensionNormalized > 0.7f)
            tensionBarColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        else if (tensionNormalized > 0.5f)
            tensionBarColor = ImVec4(1.0f, 0.7f, 0.0f, 1.0f);
        else
            tensionBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, tensionBarColor);
        ImGui::ProgressBar(tensionNormalized, ImVec2(-1, 0), "");
        ImGui::PopStyleColor();
        
        if (tensionNormalized > 0.7f)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠ High Tension!");
    }
    
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("System Statistics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Total Energy: %.3f J", data.totalEnergy);
        ImGui::Text("Avg System Tension: %.3f", data.averageSystemTension);
        ImGui::Text("Max Tension: %.3f", data.maxTension);
        
        ImGui::Separator();
        
        ImGui::Text("Total Springs: %d", data.totalSprings);
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                          "Broken Springs: %d", data.brokenSprings);
        
        if (data.brokenSprings > 0)
        {
            float breakPercentage = (data.brokenSprings * 100.0f) / 
                                   (data.totalSprings + data.brokenSprings);
            ImGui::Text("Break Rate: %.1f%%", breakPercentage);
        }
    }
    
    ImGui::End();
}

void drawRealTimePlots(const AnalysisDisplayData& data)
{
    ImGui::Begin("Real-Time Plots", &showPlotsWindow, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (data.timeHistory.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                          "No data recorded");
        ImGui::TextWrapped("Enable 'Record Data' and select a mass point to see plots.");
        ImGui::End();
        return;
    }
    
    ImGui::Text("History: %zu data points", data.timeHistory.size());
    ImGui::Text("Time Range: %.2f - %.2f s", 
               data.timeHistory.front(), data.timeHistory.back());
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Velocity over Time", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_PlotLines, velocityColor);
        ImGui::PlotLines("##Velocity", 
                        data.velocityHistory.data(), 
                        static_cast<int>(data.velocityHistory.size()),
                        0, "Speed (m/s)", 
                        0.0f, FLT_MAX, 
                        ImVec2(400, 120));
        ImGui::PopStyleColor();
        
        if (!data.velocityHistory.empty())
        {
            float current = data.velocityHistory.back();
            float max = *std::max_element(data.velocityHistory.begin(), 
                                         data.velocityHistory.end());
            float avg = 0.0f;
            for (float v : data.velocityHistory) avg += v;
            avg /= data.velocityHistory.size();
            
            ImGui::Text("Current: %.3f m/s | Max: %.3f m/s | Avg: %.3f m/s", 
                       current, max, avg);
        }
    }
    
    if (ImGui::CollapsingHeader("Kinetic Energy over Time", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_PlotLines, energyColor);
        ImGui::PlotLines("##Energy", 
                        data.energyHistory.data(), 
                        static_cast<int>(data.energyHistory.size()),
                        0, "Energy (J)", 
                        0.0f, FLT_MAX, 
                        ImVec2(400, 120));
        ImGui::PopStyleColor();
        
        if (!data.energyHistory.empty())
        {
            float current = data.energyHistory.back();
            float max = *std::max_element(data.energyHistory.begin(), 
                                         data.energyHistory.end());
            float avg = 0.0f;
            for (float e : data.energyHistory) avg += e;
            avg /= data.energyHistory.size();
            
            ImGui::Text("Current: %.4f J | Max: %.4f J | Avg: %.4f J", 
                       current, max, avg);
        }
    }
    
    if (ImGui::CollapsingHeader("Spring Tension over Time", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_PlotLines, tensionColor);
        ImGui::PlotLines("##Tension", 
                        data.tensionHistory.data(), 
                        static_cast<int>(data.tensionHistory.size()),
                        0, "Tension", 
                        0.0f, FLT_MAX, 
                        ImVec2(400, 120));
        ImGui::PopStyleColor();
        
        if (!data.tensionHistory.empty())
        {
            float current = data.tensionHistory.back();
            float max = *std::max_element(data.tensionHistory.begin(), 
                                         data.tensionHistory.end());
            float avg = 0.0f;
            for (float t : data.tensionHistory) avg += t;
            avg /= data.tensionHistory.size();
            
            ImGui::Text("Current: %.3f | Max: %.3f | Avg: %.3f", 
                       current, max, avg);
            
            if (max > 2.0f)
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                                  "⚠ High tension detected in history!");
        }
    }
    
    ImGui::End();
}

void drawEventLog(Cloth* cloth)
{
    ImGui::Begin("Event Log", &showEventLog, ImGuiWindowFlags_AlwaysAutoResize);
    
    const ClothAnalysis& analysis = cloth->getAnalysis();
    const auto& events = analysis.getBreakEvents();
    
    ImGui::Text("Spring Break Events: %zu", events.size());
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScrollEvents);
    ImGui::Separator();
    
    ImGui::BeginChild("EventList", ImVec2(600, 300), true, 
                     ImGuiWindowFlags_HorizontalScrollbar);
    
    if (events.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                          "No spring break events yet...");
    }
    else
    {
        for (size_t i = 0; i < events.size(); i++)
        {
            const auto& event = events[i];
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << "[" << std::setw(7) << event.time << "s] "
               << "Spring #" << std::setw(4) << event.springIndex 
               << " broke at (" 
               << std::setw(6) << event.position.x << ", " 
               << std::setw(6) << event.position.y << ", " 
               << std::setw(6) << event.position.z 
               << ") | Tension: " << event.tension << "x";
            
            ImVec4 color;
            if (event.tension > 7.0f)
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            else if (event.tension > 5.0f)
                color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            else
                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            
            ImGui::TextColored(color, "%s", ss.str().c_str());
        }
    }
    
    if (autoScrollEvents && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    
    ImGui::EndChild();
    
    ImGui::Separator();
    
    if (ImGui::Button("Clear Log", ImVec2(120, 0)))
    {
        cloth->getAnalysis().clearHistory();
        std::cout << "Event log cleared\n";
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Export Log", ImVec2(120, 0)))
    {
        std::ofstream file("spring_break_events.txt");
        if (file.is_open())
        {
            for (const auto& event : events)
            {
                file << std::fixed << std::setprecision(2);
                file << "[" << event.time << "s] Spring #" << event.springIndex 
                     << " broke at (" << event.position.x << ", " 
                     << event.position.y << ", " << event.position.z 
                     << ") | Tension: " << event.tension << "x\n";
            }
            file.close();
            std::cout << "Event log exported to spring_break_events.txt\n";
        }
    }
    
    ImGui::End();
}


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
