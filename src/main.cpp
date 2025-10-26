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
#include "Ray.hpp"

// global settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cPressedLastFrame = false;

// time
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool mousePressed = false;
bool massSelected = false;
int selectedMassIndex = -1;

bool drawDebugRay = false;
glm::vec3 debugRayStart, debugRayEnd;

glm::vec3 lastMouseWorldPos(0.0f);
float interactionDistance = 10.0f;
bool wasMousePressed = false;

// cutting visualization
std::vector<glm::vec3> cuttingPath;
const int MAX_PATH_POINTS = 100;

// callbacks
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void cameraLock(GLFWwindow *window);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void updateGrabbedMass(GLFWwindow *window);
void drawDebugLine(Shader &shader, const glm::vec3 &start, const glm::vec3 &end);
Ray createRayFromMouse(GLFWwindow *window);
glm::vec3 getWorldPosFromRay(const Ray &ray, float distance);
bool rayPlaneIntersection(const Ray &ray, const glm::vec3 &planePoint, const glm::vec3 &planeNormal,
                          glm::vec3 &hitPoint);

int main()
{
    glfwInit();

    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Learning", nullptr, nullptr);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW Window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader("../shaders/shader.vs", "../shaders/shader.fs");
    unsigned int VAO, VBO;
    shader.use();
    camera.setLockCamera(true);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Cloth cloth(5.0f, 5.0f, 50, 50, -10.0f);
    glfwSetWindowUserPointer(window, &cloth);

    glm::vec4 clear_color = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        processInput(window);
        cameraLock(window);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cloth.update(deltaTime);
        updateGrabbedMass(window);

        shader.use();

        glm::mat4 projection =
            glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        cloth.draw(shader);

        if (!cuttingPath.empty() && cuttingPath.size() >= 2)
        {
            shader.setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
            glLineWidth(3.0f);

            std::vector<float> pathVertices;
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

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

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

    static bool rPressedLastFrame = false;
    bool rPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (rPressed && !rPressedLastFrame)
    {
        Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
        cloth->reset();

        mousePressed = false;
        massSelected = false;
        selectedMassIndex = -1;
        cuttingPath.clear();

        std::cout << "Simulation reset!" << std::endl;
    }
    rPressedLastFrame = rPressed;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
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

void cameraLock(GLFWwindow *window)
{
    static bool cPressedLastFrame = false;
    bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    if (cPressed && !cPressedLastFrame)
    {
        camera.unLockCamera(window);
        firstMouse = true;

        if (!camera.getCameraBlocked())
        {
            std::cout << "Camera UNLOCKED - can pick points, cursor visible" << std::endl;
        }
        else
        {
            std::cout << "Camera LOCKED - cannot pick points, cursor hidden" << std::endl;
            if (massSelected)
            {
                Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
                cloth->releaseMassPoint(selectedMassIndex);
                massSelected = false;
                selectedMassIndex = -1;
            }
        }
    }
    cPressedLastFrame = cPressed;
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

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (camera.getCameraBlocked())
        {
            std::cout << "Camera locked - cannot pick points" << std::endl;
            return;
        }

        Ray ray = createRayFromMouse(window);
        mousePressed = true;

        Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
        selectedMassIndex = cloth->pickMassPoint(ray);
        massSelected = (selectedMassIndex != -1);

        if (massSelected)
        {
            Mass &mass = cloth->getMass(selectedMassIndex);
            interactionDistance = glm::length(mass.position - camera.Position);
            lastMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);

            cuttingPath.clear();

            std::cout << "Picked mass point: " << selectedMassIndex << " at distance: " << interactionDistance
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
                std::cout << "Cutting at plane intersection: (" << lastMouseWorldPos.x << ", " << lastMouseWorldPos.y
                          << ", " << lastMouseWorldPos.z << ")" << std::endl;
            }
            else
            {
                interactionDistance = 10.0f;
                lastMouseWorldPos = getWorldPosFromRay(ray, interactionDistance);
                cuttingPath.clear();
                cuttingPath.push_back(lastMouseWorldPos);
                std::cout << "Cutting (fallback) at distance: " << interactionDistance << std::endl;
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        mousePressed = false;

        if (massSelected)
        {
            Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
            cloth->releaseMassPoint(selectedMassIndex);
            massSelected = false;
            selectedMassIndex = -1;
            std::cout << "Mass point released" << std::endl;
        }

        cuttingPath.clear();
    }
}

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