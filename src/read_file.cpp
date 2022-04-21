#include <engine/read_file.hpp>
#include <stb_image.h>
#include <engine/Log.hpp>
#include <fstream>

namespace en
{
	std::vector<char> ReadFileBinary(const std::string& fileName)
	{
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			Log::Error("Failed to open file " + fileName, true);

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	std::vector<std::vector<float>> ReadFileImageR(const std::string& fileName)
	{
		int width;
		int height;
		int channelCount;

		stbi_uc* data = stbi_load(fileName.c_str(), &width, &height, &channelCount, 1);
		if (data == nullptr)
			Log::Error("Failed to load ImageR " + fileName, true);

		std::vector<std::vector<float>> values(width);
		for (int x = 0; x < width; x++)
		{
			values[x].resize(height);
			for (int y = 0; y < height; y++)
			{
				uint8_t imageVal = data[height * x + y];
				values[x][y] = static_cast<float>(imageVal) / 255.0f;
			}
		}

		stbi_image_free(data);
		return values;
	}
}
