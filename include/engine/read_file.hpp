#pragma once

#include <vector>
#include <string>

namespace en
{
	std::vector<char> ReadFileBinary(const std::string& fileName);
	std::vector<std::vector<float>> ReadFileImageR(const std::string& fileName);
	std::vector<std::vector<std::vector<float>>> ReadFileDensity3D(const std::string& fileName, size_t xSize, size_t ySize, size_t zSize);
}
