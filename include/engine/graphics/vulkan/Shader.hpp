#pragma once

#include <engine/graphics/common.hpp>
#include <vector>
#include <string>

namespace en::vk
{
	class Shader
	{
	public:
		enum Type
		{
			Vertex,
			Geometry,
			Fragment
		};

		Shader();
		Shader(const std::vector<char>& code);
		Shader(const std::string& fileName, bool compiled);

		void Destroy();

		VkShaderModule GetVulkanModule() const;

	private:
		VkShaderModule m_VulkanModule;

		void Create(const std::vector<char>& code);
	};
}
