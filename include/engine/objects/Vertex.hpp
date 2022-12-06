#pragma once

#include <engine/graphics/Common.hpp>
#include <glm/glm.hpp>
#include <array>

namespace en
{
	struct PVertex
	{
		glm::vec3 pos;

		PVertex(glm::vec3 pPos);

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescription();
	};

	struct PCVertex
	{
		glm::vec3 pos;
		glm::vec4 color;

		PCVertex(glm::vec3 pPos, glm::vec4 pColor);

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescription();
	};

	struct PNTVertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 tex;

		PNTVertex(glm::vec3 pPos, glm::vec3 pNormal = glm::vec3(0.0f), glm::vec2 pTex = glm::vec2(0.0f));

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription();
	};
}
