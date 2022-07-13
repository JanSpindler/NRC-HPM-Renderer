#include <engine/util/read_file.hpp>
#include <stb_image.h>
#include <engine/util/Log.hpp>
#include <fstream>
#include <thread>
#include <array>

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

	std::vector<std::vector<std::vector<float>>> ReadFileDensity3D(const std::string& fileName, size_t xSize, size_t ySize, size_t zSize)
	{
		size_t totalSize = xSize * ySize * zSize;

		// Load raw data
		std::vector<char> binaryData = ReadFileBinary(fileName);
		float* rawData = reinterpret_cast<float*>(binaryData.data());

		// Create and fill 3d array
		std::vector<std::vector<std::vector<float>>> density3D(xSize);

		for (size_t x = 0; x < xSize; x++)
		{
			density3D[x].resize(ySize);
			for (size_t y = 0; y < ySize; y++)
			{
				density3D[x][y].resize(zSize);
				for (size_t z = 0; z < zSize; z++)
				{
					size_t index = x * ySize * zSize + y * zSize + z;
					float value = rawData[index];
					//en::Log::Info(std::to_string(value));
					density3D[x][y][z] = value;
				}
			}
		}

		return density3D;
	}

	std::vector<float> ReadFileHdr4f(const std::string& fileName, int& width, int& height)
	{
		int channel;
		stbi_set_flip_vertically_on_load(true);
		float* data = stbi_loadf(fileName.c_str(), &width, &height, &channel, 0);
		
		if (data == nullptr)
		{
			Log::Error("Failed to load Hdr4f from File: " + fileName, true);
		}

		const size_t floatCount = width * height * channel;
		const size_t rawSize = floatCount * sizeof(float);

		std::vector<float> hdrData(floatCount);
		memcpy(hdrData.data(), data, rawSize);

		if (channel == 3)
		{
			size_t newSize = width * height * 4;
			if (newSize % 16 != 0)
			{
				Log::Error("Hdr4f pixel count not dividable by 16", true);
			}
			std::vector<float> hdr4fData(newSize);

			const size_t pixelCount = width * height;
			for (size_t i = 0; i < pixelCount; i++)
			{
				hdr4fData[(i * 4) + 0] = hdrData[(i * 3) + 0];
				hdr4fData[(i * 4) + 1] = hdrData[(i * 3) + 1];
				hdr4fData[(i * 4) + 2] = hdrData[(i * 3) + 2];
				hdr4fData[(i * 4) + 3] = 1.0f;
			}

			return hdr4fData;
		}

		return hdrData;
	}
}
