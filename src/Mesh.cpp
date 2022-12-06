#include <engine/objects/Mesh.hpp>
#include <engine/graphics/VulkanAPI.hpp>

namespace en
{
	Mesh::Mesh(const std::vector<PNTVertex>& vertices, const std::vector<uint32_t>& indices, const Material* material) :
		m_Vertices(vertices),
		m_Indices(indices),
		m_Material(material)
	{
		// Vertex Buffer
		VkDeviceSize vertexDataSize = static_cast<VkDeviceSize>(sizeof(PNTVertex) * m_Vertices.size());

		vk::Buffer vertexStagingBuffer(
			vertexDataSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		vertexStagingBuffer.SetData(vertexDataSize, m_Vertices.data(), 0, 0);

		m_VertexBuffer = new vk::Buffer(
			vertexDataSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		vk::Buffer::Copy(&vertexStagingBuffer, m_VertexBuffer, vertexDataSize);

		vertexStagingBuffer.Destroy();

		// Index Buffer
		VkDeviceSize indexDataSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * m_Indices.size());

		vk::Buffer indexStagingBuffer(
			indexDataSize,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			{});
		indexStagingBuffer.SetData(indexDataSize, m_Indices.data(), 0, 0);

		m_IndexBuffer = new vk::Buffer(
			indexDataSize,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{});

		vk::Buffer::Copy(&indexStagingBuffer, m_IndexBuffer, indexDataSize);

		indexStagingBuffer.Destroy();
	}

	void Mesh::DestroyVulkanBuffers()
	{
		m_VertexBuffer->Destroy();
		delete m_VertexBuffer;

		m_IndexBuffer->Destroy();
		delete m_IndexBuffer;
	}

	VkBuffer Mesh::GetVertexBufferVulkanHandle() const
	{
		return m_VertexBuffer->GetVulkanHandle();
	}

	VkBuffer Mesh::GetIndexBufferVulkanHandle() const
	{
		return m_IndexBuffer->GetVulkanHandle();
	}

	uint32_t Mesh::GetIndexCount() const
	{
		return m_Indices.size();
	}

	const Material* Mesh::GetMaterial() const
	{
		return m_Material;
	}

	void Mesh::SetMaterial(const Material* material)
	{
		m_Material = material;
	}

	VkDescriptorSetLayout MeshInstance::m_DescriptorSetLayout;
	VkDescriptorPool MeshInstance::m_DescriptorPool;

	void MeshInstance::Init()
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create Descriptor Set Layout
		VkDescriptorSetLayoutBinding uboLayoutBinding;
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
		descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCreateInfo.pNext = nullptr;
		descSetLayoutCreateInfo.flags = 0;
		descSetLayoutCreateInfo.bindingCount = 1;
		descSetLayoutCreateInfo.pBindings = &uboLayoutBinding;

		VkResult result = vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create Descriptor Pool
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descPoolCreateInfo;
		descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCreateInfo.pNext = nullptr;
		descPoolCreateInfo.flags = 0;
		descPoolCreateInfo.maxSets = MAX_MESH_INSTANCE_COUNT;
		descPoolCreateInfo.poolSizeCount = 1;
		descPoolCreateInfo.pPoolSizes = &poolSize;

		result = vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void MeshInstance::Shutdown()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout MeshInstance::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	MeshInstance::MeshInstance(const Mesh* mesh, const glm::mat4& modelMat) :
		m_Mesh(mesh),
		m_ModelMat(modelMat),
		m_UniformBuffer(new vk::Buffer(
			sizeof(glm::mat4),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{}))
	{
		m_UniformBuffer->SetData(sizeof(glm::mat4), &m_ModelMat, 0, 0);

		VkDevice device = VulkanAPI::GetDevice();

		// Allocate descriptor set
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		ASSERT_VULKAN(result);

		// Write descriptor set
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_UniformBuffer->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4);

		VkWriteDescriptorSet writeDescSet;
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.pNext = nullptr;
		writeDescSet.dstSet = m_DescriptorSet;
		writeDescSet.dstBinding = 0;
		writeDescSet.dstArrayElement = 0;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescSet.pImageInfo = nullptr;
		writeDescSet.pBufferInfo = &bufferInfo;
		writeDescSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
	}

	void MeshInstance::Destroy()
	{
		m_UniformBuffer->Destroy();
		delete m_UniformBuffer;
	}

	const Mesh* MeshInstance::GetMesh() const
	{
		return m_Mesh;
	}

	void MeshInstance::SetMesh(const Mesh* mesh)
	{
		m_Mesh = mesh;
	}

	const glm::mat4& MeshInstance::GetModelMat() const
	{
		return m_ModelMat;
	}

	void MeshInstance::SetModelMat(const glm::mat4& modelMat)
	{
		m_ModelMat = modelMat;
		m_UniformBuffer->SetData(sizeof(glm::mat4), &m_ModelMat, 0, 0);
	}

	VkDescriptorSet MeshInstance::GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}
}
