#include <engine/util/LogFile.hpp>
#include <filesystem>
#include <engine/util/Log.hpp>

namespace en
{
	LogFile::LogFile(const std::string& filePath)
	{
		if (std::filesystem::exists(filePath))
		{
			Log::Info("Log file (" + filePath + ") already exists -> deleting it");
			std::filesystem::remove(filePath);
		}

		m_File = std::ofstream(filePath);
	}

	LogFile::~LogFile()
	{
		m_File.close();
	}

	void LogFile::WriteLine(const std::string& line)
	{
		m_File << line << std::endl;
	}
}
