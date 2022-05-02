#pragma once

#include <vector>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <cstdarg>

namespace en
{

	class Tensor
	{
	public:
		Tensor(const std::vector<size_t>& shape, float defaultVal = 0.0f);

		Tensor operator+(const Tensor& other) const;
		Tensor operator*(const Tensor& other) const;

		void Destroy();

		float GetValue(size_t linearIndex) const;
		size_t GetLinearIndex(const std::vector<size_t>& indices) const;

		void SetValue(size_t linearIndex, float value);

	private:
		std::vector<size_t> m_Shape;
		float* m_HostData;
		size_t m_FloatCount;
		size_t m_MemorySize;

		vk::Buffer m_VkBuffer;
	};
}
