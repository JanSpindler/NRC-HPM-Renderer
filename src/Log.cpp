#include <engine/util/Log.hpp>
#include <iostream>

namespace en
{
	std::unordered_map<std::string, std::ofstream> Log::s_LogFiles = {};

	void Log::Info(const std::string& msg)
	{
		std::cout << "INFO: \t" << msg << std::endl;
	}

	void Log::Warn(const std::string& msg)
	{
		std::cout << "WARN: \t" << msg << std::endl;
	}

	void Log::Error(const std::string& msg, bool exit) {
		std::cout << "ERROR:\t" << msg << std::endl;
		if (exit)
			throw std::runtime_error("SkyRenderer ERROR: " + msg);
	}

	void Log::LocationError(const std::string& msg, VkResult res, const std::string& file, const int line, bool exit)
	{
		std::cout << "Error\t" << msg << ", errno " << res << "\n\t in " << file << ":" << line << std::endl;
		if (exit)
			throw std::runtime_error("SkyRenderer ERROR: " + msg);
	}

	void Log::InfoFile(const std::string& msg, const std::string& fileName)
	{
		if (s_LogFiles.find(fileName) == s_LogFiles.end()) { s_LogFiles.insert_or_assign(fileName, std::ofstream(fileName)); }
		s_LogFiles[fileName] << msg << "\n";
	}

	void Log::CloseFile(const std::string& fileName)
	{
		if (s_LogFiles.find(fileName) != s_LogFiles.end()) { s_LogFiles[fileName].close(); }
	}
}
