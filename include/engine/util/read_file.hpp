#pragma once

#include <vector>
#include <string>
#include <array>

namespace en
{
	std::vector<char> ReadFileBinary(const std::string& fileName);
	std::vector<std::vector<float>> ReadFileImageR(const std::string& fileName);
	std::vector<std::vector<std::vector<float>>> ReadFileDensity3D(const std::string& fileName, size_t xSize, size_t ySize, size_t zSize);
	std::vector<float> ReadFileHdr4f(const std::string& fileName, int& width, int& height, float max);
	std::array<std::vector<float>, 2> Hdr4fToCdf(const std::vector<float>& hdr4f, size_t width, size_t height);
}
