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

glm::vec3 lastMouseWorldPos(0.0f);
bool wasMousePressed = false;

// callbacks
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void cameraLock(GLFWwindow *window);
Ray createRayFromMouse(double xpos, double ypos, const glm::mat4 &view, const glm::mat4 &projection);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void updateGrabbedMass(GLFWwindow *window);

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
            std::cout << "Camera LOCKED - can pick points, cursor hidden" << std::endl;
        }
        else
        {
            std::cout << "Camera UNLOCKED - cannot pick points, cursor visible" << std::endl;
            // relase mass if was selected
            if (massSelected)
            {
                Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
                cloth->releaseMassPoint(selectedMassIndex);
                massSelected = false;
                selectedMassIndex = -1;
                std::cout << "Auto-released mass point (camera unlocked)" << std::endl;
            }
        }
    }
    cPressedLastFrame = cPressed;
}

Ray createRayFromMouse(double xpos, double ypos, const glm::mat4 &view, const glm::mat4 &projection)
{
    // mouse position conversion to NDC
    float x = (2.0f * xpos) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * ypos) / SCR_HEIGHT;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    // converstion to eye
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // convertsion to world
    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    return Ray(camera.Position, rayWorld);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (camera.getCameraBlocked())
        {
            std::cout << "Camera locked - cannot pick points or cut springs" << std::endl;
            return;
        }

        mousePressed = true;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection =
            glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        Ray ray = createRayFromMouse(xpos, ypos, view, projection);

        Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));
        selectedMassIndex = cloth->pickMassPoint(ray);
        massSelected = (selectedMassIndex != -1);

        if (massSelected)
        {
            std::cout << "Picked mass point: " << selectedMassIndex << std::endl;
            Mass &mass = cloth->getMass(selectedMassIndex);
            float distanceToCloth = glm::length(mass.position - camera.Position);
            lastMouseWorldPos = ray.Origin() + ray.Direction() * distanceToCloth;
        }
        else
        {
            std::cout << "Cutting mode activated" << std::endl;
            float cutDistance = 8.0f;
            lastMouseWorldPos = ray.Origin() + ray.Direction() * cutDistance;
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
    }
}

void updateGrabbedMass(GLFWwindow *window)
{
    if (camera.getCameraBlocked() || !mousePressed)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    Ray ray = createRayFromMouse(xpos, ypos, view, projection);
    Cloth *cloth = static_cast<Cloth *>(glfwGetWindowUserPointer(window));

    if (massSelected)
    {
        Mass &mass = cloth->getMass(selectedMassIndex);
        float targetDistance = glm::length(mass.position - camera.Position);
        glm::vec3 newPos = camera.Position + ray.Direction() * targetDistance;
        cloth->setMassPosition(selectedMassIndex, newPos);
    }
    else
    {
        float cutDistance = 8.0f;
        glm::vec3 currentMouseWorldPos = ray.Origin() + ray.Direction() * cutDistance;
        cloth->cutSpringsWithRay(ray, lastMouseWorldPos);
        lastMouseWorldPos = currentMouseWorldPos;
    }
}