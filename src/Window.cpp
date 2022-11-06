#include <engine/graphics/Window.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	bool Window::m_Supported = false;
	GLFWwindow* Window::m_GLFWHandle;
	uint32_t Window::m_Width;
	uint32_t Window::m_Height;

	void Window::Init(uint32_t width, uint32_t height, bool resizable, const std::string& title)
	{
		Log::Info("Initializing Window(GLFW)");

		m_Width = width;
		m_Height = height;

		if (glfwInit() != GLFW_TRUE)
		{
			Log::Warn("Failed to initialize GLFW. Starting without window");
			m_Supported = false;
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		m_GLFWHandle = glfwCreateWindow(
			static_cast<int>(m_Width), 
			static_cast<int>(m_Height), 
			title.c_str(), 
			nullptr,
			nullptr);

		if (m_GLFWHandle == nullptr)
		{
			Log::Warn("Failed to create Window. Starting without window");
			m_Supported = false;
			return;
		}

		m_Supported = true;
	}

	void Window::Update()
	{
		if (!m_Supported) { return; }

		glfwPollEvents();

		int width;
		int height;
		glfwGetFramebufferSize(m_GLFWHandle, &width, &height);
		m_Width = static_cast<int>(width);
		m_Height = static_cast<int>(height);
	}

	void Window::Shutdown()
	{
		if (!m_Supported) { return; }

		Log::Info("Shutting down Window(GLFW)");

		glfwDestroyWindow(m_GLFWHandle);
		glfwTerminate();
	}

	void Window::WaitForUsableSize()
	{
		if (!m_Supported) { return; }

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
		if (!m_Supported) { return; }

		int cursorMode = enableCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
		glfwSetInputMode(m_GLFWHandle, GLFW_CURSOR, cursorMode);
	}

	bool Window::IsSupported()
	{
		return m_Supported;
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
		if (!m_Supported) { return false; }
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
		if (!m_Supported) { return VK_NULL_HANDLE; }

		VkSurfaceKHR surface;
		VkResult result = glfwCreateWindowSurface(vulkanInstance, m_GLFWHandle, nullptr, &surface);
		ASSERT_VULKAN(result);

		return surface;
	}

	void Window::SetTitle(const std::string& title)
	{
		if (!m_Supported) { return; }
		glfwSetWindowTitle(m_GLFWHandle, title.c_str());
	}
}
