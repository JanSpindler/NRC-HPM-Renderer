#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>

namespace en::vk
{
	class Vector
	{
	public:
		Vector(size_t dimension);

		Vector operator+(const Vector& other) const;
		void operator+=(const Vector& other);

		float operator*(const Vector& other) const; // Dot product not element wise multiplication

		void Destroy();

		size_t GetDimension() const;
		size_t GetMemorySize() const;
		float GetValue(size_t index) const;
		const float* GetData() const;

		void SetValue(size_t index, float value);

	private:
		size_t m_Dimension;
		size_t m_MemorySize;
		float* m_HostMemory;
		vk::Buffer m_Buffer;
	};
}
