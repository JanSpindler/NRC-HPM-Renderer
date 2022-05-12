#include <engine/util/Input.hpp>
#include <engine/graphics/Window.hpp>

namespace en
{
    GLFWwindow* Input::m_WindowHandle = nullptr;
    glm::vec2 Input::m_MousePos = glm::vec2(0.0f);
    glm::vec2 Input::m_MouseMove = glm::vec2(0.0f);

    void Input::Init(GLFWwindow* windowHandle)
    {
        m_WindowHandle = windowHandle;
        Update();
    }

    void Input::Update()
    {
        double xpos;
        double ypos;
        glfwGetCursorPos(m_WindowHandle, &xpos, &ypos);
        glm::vec2 newMousePos = glm::vec2(xpos, ypos);

        m_MouseMove = newMousePos - m_MousePos;
        m_MousePos = newMousePos;
    }

    bool Input::IsKeyPressed(int keycode)
    {
        int state = glfwGetKey(m_WindowHandle, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        int state = glfwGetMouseButton(m_WindowHandle, button);
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePos()
    {
        return m_MousePos;
    }

    glm::vec2 Input::GetMouseMove()
    {
        return m_MouseMove;
    }

    void Input::HandleUserCamInput(Camera* cam, float deltaTime)
    {
        bool cameraChanged = false;

        // Mouse input handling
        bool mouseRightPressed = en::Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        en::Window::EnableCursor(!mouseRightPressed);
        if (mouseRightPressed)
        {
            glm::vec2 mouseMove = 0.005f * -en::Input::GetMouseMove();
            cam->RotateViewDir(mouseMove.x, mouseMove.y);

            if (mouseMove != glm::vec2(0.0f, 0.0f))
                cameraChanged = true;
        }

        // Keyboard input handling
        glm::vec3 camMove(0.0f, 0.0f, 0.0f);
        float camMoveSpeed = 20.0f * deltaTime;
        bool frontPressed = en::Input::IsKeyPressed(KEY_W);
        bool backPressed = en::Input::IsKeyPressed(KEY_S);
        bool leftPressed = en::Input::IsKeyPressed(KEY_A);
        bool rightPressed = en::Input::IsKeyPressed(KEY_D);
        bool upPressed = en::Input::IsKeyPressed(KEY_SPACE);
        bool downPressed = en::Input::IsKeyPressed(KEY_C);
        if (frontPressed && !backPressed)
            camMove.z = camMoveSpeed;
        else if (backPressed && !frontPressed)
            camMove.z = -camMoveSpeed;
        if (rightPressed && !leftPressed)
            camMove.x = camMoveSpeed;
        else if (leftPressed && !rightPressed)
            camMove.x = -camMoveSpeed;
        if (upPressed && !downPressed)
            camMove.y = camMoveSpeed;
        else if (downPressed && !upPressed)
            camMove.y = -camMoveSpeed;

        if (en::Input::IsKeyPressed(KEY_LEFT_SHIFT))
            camMove *= 10.0f;

        if (camMove != glm::vec3(0.f, 0.f, 0.f))
        {
            cam->Move(camMove);
            cameraChanged = true;
        }

        cam->SetChanged(cameraChanged);
    }
}
