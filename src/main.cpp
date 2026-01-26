#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <iostream>
#include <string>
#include <limits>
#include <memory>
#include <array>

#include "Camera.hpp"
#include "Cloth.hpp"
#include "Shader.hpp"
#include "Ray.hpp"
#include "Skybox.hpp"
#include "GUI.hpp"
#include "ExperimentSystem.hpp"
#include "AppData.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
std::vector<float> sphereVertices;
std::vector<unsigned int> sphereIndices;

// GLOBAL STATE
unsigned int SCR_WIDTH = 1600;
unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool trackingMode = false;
int trackedMassIndex = -1;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool mousePressed = false;
bool massSelected = false;
int selectedMassIndex = -1;
glm::vec3 lastMouseWorldPos(0.0f);
float interactionDistance = 10.0f;
bool cubeEnabled = true;

std::vector<glm::vec3> cuttingPath;
const int MAX_PATH_POINTS = 100;

glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
unsigned int depthMapFBO;
unsigned int depthMap;

// FORWARD DECLARATIONS
void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void charCallback(GLFWwindow* window, unsigned int c);
void initSpheres();
void renderForceVisualizations(Shader& shader, Cloth& cloth, const glm::vec3& lightPos);

void processInput(GLFWwindow *window);
void updateGrabbedMass(GLFWwindow *window);
void renderCuttingPath(Shader &shader);
void renderScene(Shader &shader, Cloth &cloth);
void renderFloor(Shader &shader);

Ray createRayFromMouse(GLFWwindow *window);
glm::vec3 getWorldPosFromRay(const Ray &ray, float distance);
bool rayPlaneIntersection(const Ray &ray, const glm::vec3 &planePoint, const glm::vec3 &planeNormal,
                          glm::vec3 &hitPoint);

GLFWwindow *initializeWindow();
void setupCallbacks(GLFWwindow *window);
void setupShadowMap();

unsigned int floorVAO = 0, floorVBO = 0;

// MAIN
int main(int argc, char** argv)
{
    bool experimentMode = false;
    std::string experimentName = "";

    if(argc > 1)
    {
        std::string arg = argv[1];
        if(arg == "--experiment" or arg == "-e")
        {
            experimentMode = true;
            if(argc > 2)
            {
                experimentName = argv[2];
            }
        }
        else if(arg == "--help" or arg == "-h")
        {
            std::cout << "help\n";
            return 0;
        }
    }

    GLFWwindow *window = initializeWindow();
    if (!window)
        return -1;

    Shader shader("../shaders/shader.vs", "../shaders/shader.fs");
    Shader skyboxShader("../shaders/skybox.vs", "../shaders/skybox.fs");
    Shader shadowShader("../shaders/shadow.vs", "../shaders/shadow.fs");

    setupShadowMap();

    std::vector<std::string> faces = {
        "../img/skybox/right.jpg",
        "../img/skybox/left.jpg",
        "../img/skybox/top.jpg",
        "../img/skybox/bottom.jpg",
        "../img/skybox/front.jpg",
        "../img/skybox/back.jpg"
    };
    Skybox skybox(faces);

    Cloth cloth(4.0f, 4.0f, 30, 30, -10.0f);

    Cube* cube = new Cube (glm::vec3(0, -2, 0), glm::vec3(2, 2, 2), "../img/textures/krem.png");
    cloth.addCollisionObject(cube);

    if(experimentMode)
    {
        std::cout << "Welcome to experiment mode\n";

        ExperimentSystem experimentSystem(&cloth);

        if(experimentName == "all")
        {
            experimentSystem.runAllExp();
        }
        else if(experimentName == "exp1")
        {
            experimentSystem.exp1_thresholdImpact();
        }
        else if(experimentName == "exp2")
        {
            experimentSystem.exp2_windStrength();
        }
        else if(experimentName == "exp3")
        {
            experimentSystem.exp3_windDirection();
        }
        else if(experimentName == "exp4")
        {
            experimentSystem.exp4_gravityImpact();
        }
        else if(experimentName == "exp5")
        {
            experimentSystem.exp5_cascadeBreaking();
        }
        else if(experimentName == "exp6")
        {
            experimentSystem.exp6_solverStability();
        }
        else if(experimentName == "exp7")
        {
            experimentSystem.exp7_meshSizePerf();
        }
        else if(experimentName == "exp8")
        {
            experimentSystem.exp8_springTypes();
        }
        else
        {
            std::cout << "unknown experiment " << experimentName << "\n";
            return 1;
        }

        glfwTerminate();
        return 0;
    }

    ClothGUI gui;
    gui.init(window, "#version 330");

    AppData appData;
    appData.cloth = &cloth;
    appData.skybox = &skybox;
    appData.gui = &gui;
    appData.camera = &camera;
    appData.lightPos = &lightPos;
    appData.cube = cube;
    appData.cubeEnabled = &cubeEnabled;
    appData.fps = 0.0;

    glfwSetWindowUserPointer(window, &appData);
    setupCallbacks(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    camera.setLockCamera(false);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    float floorVertices[] = {
        // positions          // normals           // texture coords
        -50.0f, -10.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         50.0f, -10.0f, -50.0f,  0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
         50.0f, -10.0f,  50.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
         50.0f, -10.0f,  50.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
        -50.0f, -10.0f,  50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 10.0f,
        -50.0f, -10.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    initSpheres();

   while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        float clampedDt = glm::min(deltaTime, 0.016f);
        cloth.update(clampedDt);
        updateGrabbedMass(window);

        gui.beginFrame();

        glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        renderScene(shadowShader, cloth);
        
        if (cubeEnabled)
{
    cube->render(shadowShader);
}
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("lightColor", lightColor);
        shader.setInt("useShadows", 1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader.setInt("shadowMap", 1);

        glActiveTexture(GL_TEXTURE0);
        shader.setInt("clothTexture", 0);

        renderScene(shader, cloth);
        
        if (cubeEnabled)
{
    cube->render(shader);
}
        
        renderForceVisualizations(shader, cloth, lightPos);
        renderCuttingPath(shader);

        skyboxShader.use();
        skybox.draw(skyboxShader, view, projection);

        gui.drawClothControls(&appData);
        gui.render();

        glfwSwapBuffers(window);
    }

    delete cube;
    cloth.clearCollisionObjects();
    gui.shutdown();

    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glfwTerminate();
    return 0;
}

void setupShadowMap()
{
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderScene(Shader &shader, Cloth &cloth)
{
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);

    renderFloor(shader);
    cloth.draw(shader);
}

void renderFloor(Shader &shader)
{
    shader.setVec3("color", glm::vec3(0.5f, 0.5f, 0.5f));
    shader.setInt("useTexture", 0);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

GLFWwindow *initializeWindow()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Cloth Simulation", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW Window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);

    return window;
}

void setupCallbacks(GLFWwindow *window)
{
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if (action != GLFW_PRESS)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard && key != GLFW_KEY_ESCAPE && key != GLFW_KEY_C)
        return;

    AppData *appData = static_cast<AppData *>(glfwGetWindowUserPointer(window));
    Cloth *cloth = appData->cloth;
    Skybox *skybox = appData->skybox;
    ForceManager& forceManager = cloth->getForceManager();

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (trackingMode)
        {
            trackingMode = false;
            trackedMassIndex = -1;
            cloth->getAnalysis().setRecordingEnabled(false);
            std::cout << "Tracking mode disabled\n";
        }
        else
        {
            glfwSetWindowShouldClose(window, true);
        }
        break;

    case GLFW_KEY_X:
        if (trackingMode)
        {
            bool recording = cloth->getAnalysis().isRecordingEnabled();
            cloth->getAnalysis().setRecordingEnabled(!recording);
            std::cout << "Recording: " << (!recording ? "ON" : "OFF") << "\n";
        }
        else
        {
            std::cout << "No point tracked. Right-click on a point first.\n";
        }
        break;

    case GLFW_KEY_L:
        if (trackingMode)
        {
            trackingMode = false;
            cloth->getAnalysis().setRecordingEnabled(false);
            std::cout << "Point #" << trackedMassIndex << " deselected\n";
            trackedMassIndex = -1;
        }
        break;

    case GLFW_KEY_R:
        cloth->reset();
        std::cout << "Cloth reset" << std::endl;
        break;
    case GLFW_KEY_V:
        cloth->setOrientation(Cloth::ClothOrientation::VERTICAL);
        break;
    case GLFW_KEY_H:
        cloth->setOrientation(Cloth::ClothOrientation::HORIZONTAL);
        break;
    case GLFW_KEY_M:
        cloth->changeMassesVisible();
        std::cout << "Toggled mass visibility" << std::endl;
        break;

    case GLFW_KEY_F:
        cloth->freeCloth();
        break;

    case GLFW_KEY_N:
        cloth->changeSpringsVisible();
        std::cout << "Toggled spring visibility" << std::endl;
        break;
    case GLFW_KEY_U:
        cubeEnabled = !cubeEnabled;
        cloth->setEnableCollisions(cubeEnabled);
        std::cout << "Cube " << (cubeEnabled ? "enabled" : "disabled") << std::endl;
        break;

    case GLFW_KEY_B:
        cloth->changeTextureVisible();
        std::cout << "Toggled texture visibility" << std::endl;
        break;

    case GLFW_KEY_Z:
        skybox->toggleVisibility();
        std::cout << "Toggled skybox visibility: " << (skybox->isVisible() ? "ON" : "OFF") << std::endl;
        break;

    case GLFW_KEY_C:
        camera.unLockCamera(window);
        firstMouse = true;
        if (massSelected)
        {
            cloth->releaseMassPoint(selectedMassIndex);
            massSelected = false;
            selectedMassIndex = -1;
        }

        if (camera.getCameraBlocked())
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        break;

    case GLFW_KEY_P:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setEnabled(!wind->isEnabled());
            std::cout << "Wind: " << (wind->isEnabled() ? "ON" : "OFF") << std::endl;
        }
        break;

    case GLFW_KEY_1:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setDirection(glm::vec3(1.0f, 0.0f, 0.0f));
            std::cout << "Wind direction: RIGHT" << std::endl;
        }
        break;

    case GLFW_KEY_2:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setDirection(glm::vec3(-1.0f, 0.0f, 0.0f));
            std::cout << "Wind direction: LEFT" << std::endl;
        }
        break;

    case GLFW_KEY_3:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setDirection(glm::vec3(0.0f, 0.0f, 1.0f));
            std::cout << "Wind direction: FORWARD" << std::endl;
        }
        break;

    case GLFW_KEY_4:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setDirection(glm::vec3(0.0f, 0.0f, -1.0f));
            std::cout << "Wind direction: BACKWARD" << std::endl;
        }
        break;

    case GLFW_KEY_5:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            wind->setDirection(glm::vec3(0.0f, 1.0f, 0.0f));
            std::cout << "Wind direction: UP" << std::endl;
        }
        break;

    case GLFW_KEY_EQUAL:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            float newStrength = glm::min(wind->getStrength() + 1.0f, 20.0f);
            wind->setStrength(newStrength);
            std::cout << "Wind strength: " << newStrength << std::endl;
        }
        break;

    case GLFW_KEY_MINUS:
        if (WindForce* wind = forceManager.getForce<WindForce>())
        {
            float newStrength = glm::max(wind->getStrength() - 1.0f, 0.0f);
            wind->setStrength(newStrength);
            std::cout << "Wind strength: " << newStrength << std::endl;
        }
        break;

    case GLFW_KEY_G:
        if (GravityForce* gravity = forceManager.getForce<GravityForce>())
        {
            gravity->setEnabled(!gravity->isEnabled());
            std::cout << "Gravity: " << (gravity->isEnabled() ? "ON" : "OFF") << std::endl;
        }
        break;

    case GLFW_KEY_O:
        {
            if (forceManager.getForce<OscillatingForce>() == nullptr)
            {
                OscillatingForce* oscillating = forceManager.addForce<OscillatingForce>(
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    3.0f,
                    2.0f
                );
                oscillating->setEnabled(true);
                std::cout << "Oscillating force ADDED" << std::endl;
            }
            else
            {
                auto oscillations = forceManager.getForces<OscillatingForce>();
                for (auto* osc : oscillations)
                {
                    osc->setEnabled(!osc->isEnabled());
                    std::cout << "Oscillating force: " << (osc->isEnabled() ? "ON" : "OFF") << std::endl;
                }
            }
        }
        break;

    case GLFW_KEY_T:
        {
            bool tensionEnabled = cloth->getEnableTensionBreaking();
            tensionEnabled = !tensionEnabled;
            cloth->setEnableTensionBreaking(tensionEnabled);
            std::cout << "Tension breaking: " << (tensionEnabled ? "ON" : "OFF") << std::endl;
        }
        break;

    case GLFW_KEY_LEFT_BRACKET:
        {
            float threshold = cloth->getTensionBreakThreshold();
            threshold = glm::max(threshold - 0.2f, 2.0f);
            cloth->setTensionBreakThreshold(threshold);
            std::cout << "Break threshold: " << threshold << "x" << std::endl;
        }
        break;

    case GLFW_KEY_RIGHT_BRACKET:
        {
            float threshold = cloth->getTensionBreakThreshold();
            threshold = glm::min(threshold + 0.2f, 10.0f);
            cloth->setTensionBreakThreshold(threshold);
            std::cout << "Break threshold: " << threshold << "x" << std::endl;
        }
        break;

    case GLFW_KEY_SLASH:
        std::cout << "\n========== CONTROLS ==========\n";
        std::cout << "\n--- Basic ---\n";
        std::cout << "R - Reset cloth\n";
        std::cout << "M - Toggle mass points\n";
        std::cout << "N - Toggle springs\n";
        std::cout << "B - Toggle texture\n";
        std::cout << "V - Toggle skybox\n";
        std::cout << "C - Toggle camera lock\n";
        std::cout << "ESC - Exit\n";

        std::cout << "\n--- Wind ---\n";
        std::cout << "P - Toggle wind\n";
        std::cout << "1/2/3/4/5 - Wind direction\n";
        std::cout << "+/- - Wind strength\n";

        std::cout << "\n--- Forces ---\n";
        std::cout << "G - Toggle gravity\n";
        std::cout << "O - Toggle oscillation\n";

        std::cout << "\n--- Tension Breaking ---\n";
        std::cout << "T - Toggle tension breaking\n";
        std::cout << "[ - Decrease break threshold\n";
        std::cout << "] - Increase break threshold\n";

        std::cout << "\n--- Camera ---\n";
        std::cout << "W/A/S/D - Move\n";
        std::cout << "Space/Shift - Up/Down\n";
        std::cout << "Mouse - Look around\n";
        std::cout << "Scroll - Zoom\n";

        std::cout << "\n--- Point Tracking ---\n";
        std::cout << "Left Click - Select/Track point\n";
        std::cout << "Click again - Deselect point\n";
        std::cout << "X - Toggle tracking mode\n";
        std::cout << "ESC - Cancel tracking\n";

        std::cout << "\n--- Info ---\n";
        std::cout << "H - Show this help\n";
        std::cout << "==============================\n\n";

        break;
    }
}


void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    AppData *appData = static_cast<AppData *>(glfwGetWindowUserPointer(window));
    Cloth *cloth = appData->cloth;

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            if (camera.getCameraBlocked())
                return;

            Ray ray = createRayFromMouse(window);
            mousePressed = true;

            selectedMassIndex = cloth->pickMassPoint(ray);
            massSelected = (selectedMassIndex != -1);

            if (massSelected)
            {
                Mass &mass = cloth->getMass(selectedMassIndex);
                interactionDistance = glm::length(mass.position - camera.Position);
                lastMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);
                cuttingPath.clear();
                std::cout << "Grabbed mass point #" << selectedMassIndex << " (drag mode)\n";
            }
            else
            {
                glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                glm::vec3 planePoint = glm::vec3(0.0f, 2.5f, 0.0f);

                if (rayPlaneIntersection(ray, planePoint, planeNormal, lastMouseWorldPos))
                {
                    cuttingPath.clear();
                    cuttingPath.push_back(lastMouseWorldPos);
                }
            }
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
            if (massSelected)
            {
                cloth->releaseMassPoint(selectedMassIndex);
                massSelected = false;
                selectedMassIndex = -1;
            }
            cuttingPath.clear();
        }
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            if (camera.getCameraBlocked())
                return;

            Ray ray = createRayFromMouse(window);

            int clickedIndex = cloth->pickMassPoint(ray);

            if (clickedIndex != -1)
            {
                if (trackedMassIndex == clickedIndex && trackingMode)
                {
                    trackingMode = false;
                    trackedMassIndex = -1;
                    cloth->getAnalysis().setRecordingEnabled(false);
                    std::cout << "Tracking disabled for point #" << clickedIndex << "\n";
                }
                else
                {
                    trackingMode = true;
                    trackedMassIndex = clickedIndex;

                    cloth->getAnalysis().setRecordingEnabled(true);
                    std::cout << "Tracking enabled for point #" << trackedMassIndex << " (analysis mode)\n";
                    std::cout << "Right-click again on the same point to deselect\n";
                }
            }
            else
            {
                std::cout << "No point found. Right-click on a mass point to track it.\n";
            }
        }
    }
}

void mouseCallback(GLFWwindow *window, double xposIn, double yposIn)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (!camera.getCameraBlocked())
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (camera.getCameraBlocked())
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

void updateGrabbedMass(GLFWwindow *window)
{
    if (camera.getCameraBlocked() || !mousePressed)
        return;

    Ray ray = createRayFromMouse(window);
    AppData *appData = static_cast<AppData *>(glfwGetWindowUserPointer(window));
    Cloth *cloth = appData->cloth;

    if (massSelected)
    {
        glm::vec3 currentMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);
        cloth->setMassPosition(selectedMassIndex, currentMouseWorldPos);
        lastMouseWorldPos = currentMouseWorldPos;
    }
    else
    {
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        cloth->cutSpringsWithRay(ray, lastMouseWorldPos, view, projection, SCR_WIDTH, SCR_HEIGHT);

        glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 planePoint = glm::vec3(0.0f, 2.5f, 0.0f);
        glm::vec3 currentMouseWorldPos;

        if (rayPlaneIntersection(ray, planePoint, planeNormal, currentMouseWorldPos))
        {
            if (cuttingPath.empty() ||
                glm::length(currentMouseWorldPos - cuttingPath.back()) > 0.1f)
            {
                cuttingPath.push_back(currentMouseWorldPos);

                if (cuttingPath.size() > MAX_PATH_POINTS)
                    cuttingPath.erase(cuttingPath.begin());
            }

            lastMouseWorldPos = currentMouseWorldPos;
        }
    }
}

void renderCuttingPath(Shader &shader)
{
    if (cuttingPath.empty())
        return;

    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);

    shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setInt("useTexture", 0);
    shader.setInt("useShadows", 0);

    if (cuttingPath.size() == 1)
    {
        glPointSize(20.0f);

        std::vector<float> pointVertex = {
            cuttingPath[0].x,
            cuttingPath[0].y,
            cuttingPath[0].z
        };

        GLuint pointVAO, pointVBO;
        glGenVertexArrays(1, &pointVAO);
        glGenBuffers(1, &pointVBO);

        glBindVertexArray(pointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        glBufferData(GL_ARRAY_BUFFER, pointVertex.size() * sizeof(float),
                     pointVertex.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_POINTS, 0, 1);

        glDeleteBuffers(1, &pointVBO);
        glDeleteVertexArrays(1, &pointVAO);
        glPointSize(1.0f);

        return;
    }

    glLineWidth(8.0f);

    std::vector<float> pathVertices;
    pathVertices.reserve(cuttingPath.size() * 3);

    for (const auto &point : cuttingPath)
    {
        pathVertices.push_back(point.x);
        pathVertices.push_back(point.y);
        pathVertices.push_back(point.z);
    }

    GLuint pathVAO, pathVBO;
    glGenVertexArrays(1, &pathVAO);
    glGenBuffers(1, &pathVBO);

    glBindVertexArray(pathVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
    glBufferData(GL_ARRAY_BUFFER, pathVertices.size() * sizeof(float),
                 pathVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, pathVertices.size() / 3);

    glDeleteBuffers(1, &pathVBO);
    glDeleteVertexArrays(1, &pathVAO);
    glLineWidth(1.0f);
}

Ray createRayFromMouse(GLFWwindow *window)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float x = (2.0f * xpos / SCR_WIDTH) - 1.0f;
    float y = 1.0f - (2.0f * ypos / SCR_HEIGHT);

    float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    float tanFOV = tan(glm::radians(camera.Zoom * 0.5f));

    glm::vec3 rayDirection =
        glm::normalize(camera.Front + camera.Right * x * aspectRatio * tanFOV + camera.Up * y * tanFOV);

    return Ray(camera.Position, rayDirection);
}

glm::vec3 getWorldPosFromRay(const Ray &ray, float distance)
{
    return ray.Origin() + ray.Direction() * distance;
}

bool rayPlaneIntersection(const Ray &ray, const glm::vec3 &planePoint, const glm::vec3 &planeNormal,
                          glm::vec3 &hitPoint)
{
    float denom = glm::dot(planeNormal, ray.Direction());

    if (std::abs(denom) < 0.0001f)
        return false;

    float t = glm::dot(planePoint - ray.Origin(), planeNormal) / denom;

    if (t < 0)
        return false;

    hitPoint = ray.At(t);
    return true;
}

void charCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

void generateSphere(float radius, unsigned int rings, unsigned int sectors)
{
    sphereVertices.clear();
    sphereIndices.clear();

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for(unsigned int r = 0; r < rings; r++)
    {
        for(unsigned int s = 0; s < sectors; s++)
        {
            float const y = sin(-M_PI_2 + M_PI * r * R);
            float const x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
            float const z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);

            sphereVertices.push_back(x * radius);
            sphereVertices.push_back(y * radius);
            sphereVertices.push_back(z * radius);
        }
    }

    for(unsigned int r = 0; r < rings - 1; r++)
    {
        for(unsigned int s = 0; s < sectors - 1; s++)
        {
            sphereIndices.push_back(r * sectors + s);
            sphereIndices.push_back((r + 1) * sectors + s);
            sphereIndices.push_back((r + 1) * sectors + (s + 1));

            sphereIndices.push_back(r * sectors + s);
            sphereIndices.push_back((r + 1) * sectors + (s + 1));
            sphereIndices.push_back(r * sectors + (s + 1));
        }
    }
}

void initSpheres()
{
    generateSphere(0.3f, 20, 20);

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float),
                 sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int),
                 sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void renderSphere(Shader& shader, const glm::vec3& position, const glm::vec3& color, float scale = 1.0f)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(scale));

    shader.setMat4("model", model);
    shader.setVec3("color", color);
    shader.setInt("useTexture", 0);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderForceVisualizations(Shader& shader, Cloth& cloth, const glm::vec3& lightPos)
{
    ForceManager& fm = cloth.getForceManager();

    renderSphere(shader, lightPos, glm::vec3(1.0f, 1.0f, 0.0f), 0.3f);

    if (WindForce* wind = fm.getForce<WindForce>())
    {
        if (wind->isEnabled())
        {
            glm::vec3 windPos = glm::vec3(0.0f, 5.0f, 0.0f);
            glm::vec3 windDir = wind->getDirection();
            float strength = wind->getStrength();

            renderSphere(shader, windPos, glm::vec3(0.3f, 0.7f, 1.0f), 0.3f + strength * 0.02f);

            std::vector<float> arrowVertices = {
                windPos.x, windPos.y, windPos.z,
                windPos.x + windDir.x * 2.0f,
                windPos.y + windDir.y * 2.0f,
                windPos.z + windDir.z * 2.0f
            };

            GLuint arrowVAO, arrowVBO;
            glGenVertexArrays(1, &arrowVAO);
            glGenBuffers(1, &arrowVBO);
            glBindVertexArray(arrowVAO);
            glBindBuffer(GL_ARRAY_BUFFER, arrowVBO);
            glBufferData(GL_ARRAY_BUFFER, arrowVertices.size() * sizeof(float),
                        arrowVertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            shader.setVec3("color", glm::vec3(0.3f, 0.7f, 1.0f));
            glLineWidth(3.0f);
            glDrawArrays(GL_LINES, 0, 2);
            glLineWidth(1.0f);

            glDeleteBuffers(1, &arrowVBO);
            glDeleteVertexArrays(1, &arrowVAO);
        }
    }
}
