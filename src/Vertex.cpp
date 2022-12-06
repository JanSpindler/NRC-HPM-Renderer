#include <engine/objects/Vertex.hpp>

namespace en
{
	PVertex::PVertex(glm::vec3 pPos) :
		pos(pPos)
	{
	}

	VkVertexInputBindingDescription PVertex::GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDesc;
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(PVertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	std::array<VkVertexInputAttributeDescription, 1> PVertex::GetAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 1> attrDescs;
		attrDescs[0].location = 0;
		attrDescs[0].binding = 0;
		attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDescs[0].offset = offsetof(PVertex, pos);

		return attrDescs;
	}

	PCVertex::PCVertex(glm::vec3 pPos, glm::vec4 pColor) :
		pos(pPos),
		color(pColor)
	{
	}

	VkVertexInputBindingDescription PCVertex::GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDesc;
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(PCVertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	std::array<VkVertexInputAttributeDescription, 2> PCVertex::GetAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 2> attrDescs;

		attrDescs[0].location = 0;
		attrDescs[0].binding = 0;
		attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDescs[0].offset = offsetof(PCVertex, pos);

		attrDescs[1].location = 1;
		attrDescs[1].binding = 0;
		attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDescs[1].offset = offsetof(PCVertex, color);

		return attrDescs;
	}

	PNTVertex::PNTVertex(glm::vec3 pPos, glm::vec3 pNormal, glm::vec2 pTex) :
		pos(pPos),
		normal(pNormal),
		tex(pTex)
	{
	}

	VkVertexInputBindingDescription PNTVertex::GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDesc;
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(PNTVertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	std::array<VkVertexInputAttributeDescription, 3> PNTVertex::GetAttributeDescription()
	{

		std::array<VkVertexInputAttributeDescription, 3> attrDescs;

		attrDescs[0].location = 0;
		attrDescs[0].binding = 0;
		attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDescs[0].offset = offsetof(PNTVertex, pos);

		attrDescs[1].location = 1;
		attrDescs[1].binding = 0;
		attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDescs[1].offset = offsetof(PNTVertex, normal);

		attrDescs[2].location = 2;
		attrDescs[2].binding = 0;
		attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attrDescs[2].offset = offsetof(PNTVertex, tex);

		return attrDescs;
	}
}
