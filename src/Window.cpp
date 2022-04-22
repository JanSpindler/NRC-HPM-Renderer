#include <engine/graphics/Window.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	GLFWwindow* Window::m_GLFWHandle;
	uint32_t Window::m_Width;
	uint32_t Window::m_Height;

	void Window::Init(uint32_t width, uint32_t height, bool resizable, const std::string& title)
	{
		Log::Info("Initializing Window(GLFW)");

		m_Width = width;
		m_Height = height;

		if (glfwInit() != GLFW_TRUE)
			Log::Error("Failed to initialize GLFW", true);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		m_GLFWHandle = glfwCreateWindow(static_cast<int>(m_Width), static_cast<int>(m_Height), title.c_str(), nullptr, nullptr);
	}

	void Window::Update()
	{
		glfwPollEvents();

		int width;
		int height;
		glfwGetFramebufferSize(m_GLFWHandle, &width, &height);
		m_Width = static_cast<int>(width);
		m_Height = static_cast<int>(height);
	}

	void Window::Shutdown()
	{
		Log::Info("Shutting down Window(GLFW)");

		glfwDestroyWindow(m_GLFWHandle);
		glfwTerminate();
	}

	void Window::WaitForUsableSize()
	{
		do
		{
			int width;
			int height;
			glfwGetFramebufferSize(m_GLFWHandle, &width, &height);
			m_Width = static_cast<uint32_t>(width);
			m_Height = static_cast<uint32_t>(height);

			glfwWaitEvents();
		} while (m_Width <= 0 || m_Height <= 0);
	}

	void Window::EnableCursor(bool enableCursor)
	{
		int cursorMode = enableCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
		glfwSetInputMode(m_GLFWHandle, GLFW_CURSOR, cursorMode);
	}

	GLFWwindow* Window::GetGLFWHandle()
	{
		return m_GLFWHandle;
	}

	uint32_t Window::GetWidth()
	{
		return m_Width;
	}

	uint32_t Window::GetHeight()
	{
		return m_Height;
	}

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose(m_GLFWHandle);
	}

	std::vector<const char*> Window::GetVulkanExtensions()
	{
		uint32_t extCount;
		const char** ext = glfwGetRequiredInstanceExtensions(&extCount);

		std::vector<const char*> extensions(extCount);
		for (uint32_t i = 0; i < extCount; i++)
			extensions[i] = ext[i];

		return extensions;
	}

	VkSurfaceKHR Window::CreateVulkanSurface(VkInstance vulkanInstance)
	{
		VkSurfaceKHR surface;
		VkResult result = glfwCreateWindowSurface(vulkanInstance, m_GLFWHandle, nullptr, &surface);
		ASSERT_VULKAN(result);

		return surface;
	}

	void Window::SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_GLFWHandle, title.c_str());
	}
}
