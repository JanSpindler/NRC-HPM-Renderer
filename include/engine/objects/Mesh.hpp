#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <engine/objects/Vertex.hpp>
#include <engine/graphics/vulkan/Buffer.hpp>
#include <glm/glm.hpp>
#include <engine/objects/Material.hpp>

namespace en
{
	class Mesh
	{
	public:
		Mesh(const std::vector<PNTVertex>& vertices, const std::vector<uint32_t>& indices, const Material* material);

		void DestroyVulkanBuffers();

		VkBuffer GetVertexBufferVulkanHandle() const;
		VkBuffer GetIndexBufferVulkanHandle() const;

		uint32_t GetIndexCount() const;

		const Material* GetMaterial() const;
		void SetMaterial(const Material* material);

	private:
		std::vector<PNTVertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
		const Material* m_Material;

		vk::Buffer* m_VertexBuffer;
		vk::Buffer* m_IndexBuffer;
	};

	const uint32_t MAX_MESH_INSTANCE_COUNT = 8192;

	class MeshInstance
	{
	public:
		static void Init();
		static void Shutdown();

		static VkDescriptorSetLayout GetDescriptorSetLayout();

		MeshInstance(const Mesh* mesh, const glm::mat4& modelMat);

		void Destroy();

		const Mesh* GetMesh() const;
		void SetMesh(const Mesh* mesh);

		const glm::mat4& GetModelMat() const;
		void SetModelMat(const glm::mat4& modelMat);

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		const Mesh* m_Mesh;
		glm::mat4 m_ModelMat;

		VkDescriptorSet m_DescriptorSet;
		vk::Buffer* m_UniformBuffer;
	};
}
