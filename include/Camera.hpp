#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Movement types
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
  public:
    // Directions vecs
    glm::vec3 Up;
    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    glm::vec3 Position;

    // Rotation angles
    float Yaw;
    float Pitch;

    // Camera options
    float Zoom;
    float cameraBlocked = false;
    float MovementSpeed;
    float MouseSensitivity;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f);

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    // View matrix
    glm::mat4 GetViewMatrix();

    // Locking
    void unLockCamera(GLFWwindow *window);
    void setLockCamera(bool flag)
    {
        cameraBlocked = flag;
    }
    bool getCameraBlocked()
    {
        return cameraBlocked;
    };

    // Handle mouse and keyboard
    void ProcessKeyboard(int direction, float deltaTime);
    void ProcessMouseScroll(float yoffset);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

  private:
    void updateCameraVectors();
};