#pragma once

#include <vector>
#include <string>

namespace en
{
	std::vector<char> ReadFileBinary(const std::string& fileName);
	std::vector<std::vector<float>> ReadFileImageR(const std::string& fileName);
}
