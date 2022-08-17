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
		float* data = stbi_loadf(fileName.c_str(), &width, &height, &channel, 4);
		
		if (data == nullptr)
		{
			Log::Error("Failed to load Hdr4f from File: " + fileName, true);
		}

		const size_t floatCount = width * height * 4;
		const size_t rawSize = floatCount * sizeof(float);

		std::vector<float> hdrData(floatCount);
		memcpy(hdrData.data(), data, rawSize);

		float max = 0.0f;
		size_t maxX = 0;
		size_t maxY = 0;
		for (size_t x = 0; x < width; x++)
		{
			for (size_t y = 0; y < height; y++)
			{
				for (size_t c = 0; c < 4; c++)
				{
					const size_t linearIndex = (y * width * 4) + (x * 4) + c;
					const float current = hdrData[linearIndex];
					if (current > max)
					{
						max = current;
						maxX = x;
						maxY = y;
					}

					hdrData[linearIndex] = 0.75f;
				}
			}
		}
		
		en::Log::Info(fileName + " at (" + std::to_string(maxX) + "," + std::to_string(maxY) + ") max float: " + std::to_string(max));

		return hdrData;
	}

	std::vector<float> InvertCdf(const std::vector<float>& cdf)
	{
		std::vector<float> invCdf(cdf.size());

		size_t p = 0;
		for (size_t i = 0; i < invCdf.size(); i++)
		{
			const float threshold = static_cast<float>(i) / static_cast<float>(invCdf.size());
			while (cdf[p] < threshold)
			{
				p++;
			}
			invCdf[i] = static_cast<float>(p) / static_cast<float>(cdf.size());
		}

		return invCdf;
	}

	std::array<std::vector<float>, 2> Hdr4fToCdf(const std::vector<float>& hdr4f, size_t width, size_t height)
	{
		std::vector<std::vector<float>> cdfX(height);
		for (std::vector<float>& cdfXgivenY : cdfX)
		{
			cdfXgivenY.resize(width);
		}

		std::vector<float> pdfY(height);

		// Get cdf of x given y
		for (size_t y = 0; y < height; y++)
		{
			float brightnessSum = 0.0f;
			for (size_t x = 0; x < width; x++)
			{
				const float r = hdr4f[(y * width * 4) + (x * 4) + 0];
				const float g = hdr4f[(y * width * 4) + (x * 4) + 1];
				const float b = hdr4f[(y * width * 4) + (x * 4) + 2];
				//const float a = hdr4f[(y * width * 4) + (x * 4) + 3];
			
				const float brightness = r + g + b;
				brightnessSum += brightness;

				cdfX[y][x] = brightnessSum;
			}

			// Norm
			for (size_t x = 0; x < width; x++)
			{
				cdfX[y][x] /= brightnessSum;
			}

			// Store pdf unorm of y
			pdfY[y] = brightnessSum;
		}

		// Get cdf unorm of y
		std::vector<float> cdfY(height);

		float brightnessSum = 0.0f;
		for (size_t y = 0; y < height; y++)
		{
			brightnessSum += pdfY[y];
			cdfY[y] = brightnessSum;
		}

		// Norm cdf of y
		for (size_t y = 0; y < height; y++)
		{
			cdfY[y] /= cdfY[height - 1];
		}

		std::vector<float> invCdfX(width * height);

		for (size_t y = 0; y < height; y++)
		{
			std::vector<float> invCdfXgivenY = InvertCdf(cdfX[y]);
			for (size_t x = 0; x < width; x++)
			{
				invCdfX[(y * width) + x] = invCdfXgivenY[x];
			}
		}

		return { invCdfX, InvertCdf(cdfY) };
	}
}
