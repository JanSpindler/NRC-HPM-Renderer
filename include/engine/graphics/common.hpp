#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <engine/util/Log.hpp>

#define ASSERT_VULKAN(result) if (result != VK_SUCCESS) { en::Log::LocationError("ASSERT_VULKAN triggered", result, __FILE__, __LINE__, true); }
