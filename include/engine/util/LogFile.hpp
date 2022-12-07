#pragma once

#include <string>
#include <fstream>

namespace en
{
	class LogFile
	{
	public:
		LogFile(const std::string& filePath);
		~LogFile();

		void WriteLine(const std::string& line);

	private:
		std::ofstream m_File;
	};
}
