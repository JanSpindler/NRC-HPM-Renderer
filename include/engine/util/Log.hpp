#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include <unordered_map>
#include <fstream>

namespace en
{
	class Log
	{
	public:
		static void Info(const std::string& msg);
		static void Warn(const std::string& msg);
		static void Error(const std::string& msg, bool exit);
		static void LocationError(const std::string& msg, VkResult res, const std::string& file, const int line, bool exit);
	};
}
