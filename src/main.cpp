#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <limits>

#include "Camera.hpp"
#include "Cloth.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Ray.hpp"

// GLOBAL STATE
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool mousePressed = false;
bool massSelected = false;
int selectedMassIndex = -1;
glm::vec3 lastMouseWorldPos(0.0f);
float interactionDistance = 10.0f;

std::vector<glm::vec3> cuttingPath;
const int MAX_PATH_POINTS = 100;

// FORWARD DECLARATIONS
void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

void processInput(GLFWwindow *window);
void updateGrabbedMass(GLFWwindow *window);
void renderCuttingPath(Shader &shader);

Ray createRayFromMouse(GLFWwindow *window);
glm::vec3 getWorldPosFromRay(const Ray &ray, float distance);
bool rayPlaneIntersection(const Ray &ray, const glm::vec3 &planePoint, const glm::vec3 &planeNormal,
                          glm::vec3 &hitPoint);

GLFWwindow *initializeWindow();
void setupCallbacks(GLFWwindow *window);

// MAIN
int main()
{
    GLFWwindow *window = initializeWindow();
    if (!window)
        return -1;

    Shader shader("../shaders/shader.vs", "../shaders/shader.fs");
    shader.use();

    Cloth cloth(5.0f, 5.0f, 50, 50, -10.0f);

    try
    {
        Texture *clothTexture = new Texture("../img/textures/rainbowFlag.png");
        cloth.setTexture(clothTexture);
        std::cout << "Texture loaded successfully\n";
    }
    catch (...)
    {
        std::cout << "No texture found, using solid color\n";
    }

    glfwSetWindowUserPointer(window, &cloth);

    setupCallbacks(window);

    camera.setLockCamera(true);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glm::vec4 clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Update timing
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update simulation
        cloth.update(deltaTime);
        updateGrabbedMass(window);

        // Render
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w,
                     clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 projection =
            glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        cloth.draw(shader);
        renderCuttingPath(shader);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// INITIALIZATION
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
}

// INPUT CALLBACKS
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;

    Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;

    case GLFW_KEY_R:
        cloth->reset();
        std::cout << "Cloth reset" << std::endl;
        break;

    case GLFW_KEY_M:
        cloth->changeMassesVisible();
        std::cout << "Toggled mass visibility" << std::endl;
        break;

    case GLFW_KEY_N:
        cloth->changeSpringsVisible();
        std::cout << "Toggled spring visibility" << std::endl;
        break;

    case GLFW_KEY_C: {
        camera.unLockCamera(window);
        firstMouse = true;

        if (!camera.getCameraBlocked())
        {
            std::cout << "Camera UNLOCKED - can interact with cloth" << std::endl;
        }
        else
        {
            std::cout << "Camera LOCKED - free look mode" << std::endl;

            if (massSelected)
            {
                cloth->releaseMassPoint(selectedMassIndex);
                massSelected = false;
                selectedMassIndex = -1;
            }
        }
        break;
    }
    }
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        if (camera.getCameraBlocked())
        {
            std::cout << "Camera locked - cannot interact with cloth" << std::endl;
            return;
        }

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

            std::cout << "Grabbed mass point #" << selectedMassIndex << " at distance: " << interactionDistance
                      << std::endl;
        }
        else
        {
            glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 planePoint = glm::vec3(0.0f, 2.5f, 0.0f);

            if (rayPlaneIntersection(ray, planePoint, planeNormal, lastMouseWorldPos))
            {
                cuttingPath.clear();
                cuttingPath.push_back(lastMouseWorldPos);
                std::cout << "Started cutting at: (" << lastMouseWorldPos.x << ", " << lastMouseWorldPos.y << ", "
                          << lastMouseWorldPos.z << ")" << std::endl;
            }
            else
            {
                interactionDistance = 10.0f;
                lastMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);
                cuttingPath.clear();
                cuttingPath.push_back(lastMouseWorldPos);
                std::cout << "Started cutting (fallback)" << std::endl;
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
            std::cout << "Released mass point" << std::endl;
        }

        cuttingPath.clear();
    }
}

void mouseCallback(GLFWwindow *window, double xposIn, double yposIn)
{
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
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// INPUT PROCESSING
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

// INTERACTION UPDATE
void updateGrabbedMass(GLFWwindow *window)
{
    if (camera.getCameraBlocked() || !mousePressed)
        return;

    Ray ray = createRayFromMouse(window);
    Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));

    if (massSelected)
    {
        glm::vec3 currentMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);
        cloth->setMassPosition(selectedMassIndex, currentMouseWorldPos);
        lastMouseWorldPos = currentMouseWorldPos;
    }
    else
    {
        glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 planePoint = glm::vec3(0.0f, 2.5f, 0.0f);
        glm::vec3 currentMouseWorldPos;

        if (rayPlaneIntersection(ray, planePoint, planeNormal, currentMouseWorldPos))
        {
            cloth->cutSpringsWithRay(ray, lastMouseWorldPos);
            lastMouseWorldPos = currentMouseWorldPos;

            cuttingPath.push_back(lastMouseWorldPos);
            if (cuttingPath.size() > MAX_PATH_POINTS)
            {
                cuttingPath.erase(cuttingPath.begin());
            }
        }
    }
}

// RENDERING
void renderCuttingPath(Shader &shader)
{
    if (cuttingPath.empty() || cuttingPath.size() < 2)
        return;

    shader.setBool("useTexture", false);
    shader.setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
    glLineWidth(3.0f);

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
    glBufferData(GL_ARRAY_BUFFER, pathVertices.size() * sizeof(float), pathVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, pathVertices.size() / 3);

    glDeleteBuffers(1, &pathVBO);
    glDeleteVertexArrays(1, &pathVAO);
    glLineWidth(1.0f);
}

// RAY UTILITIES
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