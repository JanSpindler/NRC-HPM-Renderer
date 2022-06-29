#include <engine/graphics/Camera.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <glm/gtx/transform.hpp>

namespace en
{
	VkDescriptorSetLayout Camera::m_DescriptorSetLayout;
	VkDescriptorPool Camera::m_DescriptorPool;

	void Camera::Init()
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Create Descriptor Set Layout
		VkDescriptorSetLayoutBinding matrixBinding;
		matrixBinding.binding = 0;
		matrixBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		matrixBinding.descriptorCount = 1;
		matrixBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		matrixBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding posBinding;
		posBinding.binding = 1;
		posBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		posBinding.descriptorCount = 1;
		posBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		posBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { matrixBinding, posBinding };

		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
		descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCreateInfo.pNext = nullptr;
		descSetLayoutCreateInfo.flags = 0;
		descSetLayoutCreateInfo.bindingCount = bindings.size();
		descSetLayoutCreateInfo.pBindings = bindings.data();

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
		descPoolCreateInfo.maxSets = MAX_CAMERA_COUNT;
		descPoolCreateInfo.poolSizeCount = 1;
		descPoolCreateInfo.pPoolSizes = &poolSize;

		result = vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void Camera::Shutdown()
	{
		VkDevice device = VulkanAPI::GetDevice();

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout Camera::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout;
	}

	Camera::Camera(
		const glm::vec3& pos,
		const glm::vec3& viewDir,
		const glm::vec3& up,
		float aspectRatio,
		float fov,
		float nearPlane,
		float farPlane)
		:
		m_Pos(pos),
		m_ViewDir(glm::normalize(viewDir)),
		m_Up(glm::normalize(up)),
		m_Changed(false),
		m_AspectRatio(aspectRatio),
		m_Fov(fov),
		m_NearPlane(nearPlane),
		m_FarPlane(farPlane),
		m_MatrixUniformBuffer(new vk::Buffer(
			sizeof(CameraMatrices),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{})),
		m_PosUniformBuffer(new vk::Buffer(
			sizeof(glm::vec3),
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			{}))
	{
		VkDevice device = VulkanAPI::GetDevice();

		// Allocate Descriptor Set
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		ASSERT_VULKAN(result);

		// Write Descriptor Set
		VkDescriptorBufferInfo matrixBufferInfo;
		matrixBufferInfo.buffer = m_MatrixUniformBuffer->GetVulkanHandle();
		matrixBufferInfo.offset = 0;
		matrixBufferInfo.range = sizeof(CameraMatrices);

		VkWriteDescriptorSet matrixWrite;
		matrixWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		matrixWrite.pNext = nullptr;
		matrixWrite.dstSet = m_DescriptorSet;
		matrixWrite.dstBinding = 0;
		matrixWrite.dstArrayElement = 0;
		matrixWrite.descriptorCount = 1;
		matrixWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		matrixWrite.pImageInfo = nullptr;
		matrixWrite.pBufferInfo = &matrixBufferInfo;
		matrixWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo posBufferInfo;
		posBufferInfo.buffer = m_PosUniformBuffer->GetVulkanHandle();
		posBufferInfo.offset = 0;
		posBufferInfo.range = sizeof(glm::vec3);

		VkWriteDescriptorSet posWrite;
		posWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		posWrite.pNext = nullptr;
		posWrite.dstSet = m_DescriptorSet;
		posWrite.dstBinding = 1;
		posWrite.dstArrayElement = 0;
		posWrite.descriptorCount = 1;
		posWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		posWrite.pImageInfo = nullptr;
		posWrite.pBufferInfo = &posBufferInfo;
		posWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> descWrites = { matrixWrite, posWrite };

		vkUpdateDescriptorSets(device, descWrites.size(), descWrites.data(), 0, nullptr);

		// Update
		UpdateUniformBuffer();
	}

	void Camera::Destroy()
	{
		m_MatrixUniformBuffer->Destroy();
		delete m_MatrixUniformBuffer;

		m_PosUniformBuffer->Destroy();
		delete m_PosUniformBuffer;
	}

	void Camera::UpdateUniformBuffer()
	{
		m_Matrices.oldProjView = m_Matrices.projView;

		glm::mat4 projMat = glm::perspective(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
		glm::mat4 viewMat = glm::lookAt(m_Pos, m_Pos + m_ViewDir, m_Up);
		m_Matrices.projView = projMat * viewMat;

		m_MatrixUniformBuffer->SetData(sizeof(CameraMatrices), &m_Matrices, 0, 0);
		m_PosUniformBuffer->SetData(sizeof(glm::vec3), &m_Pos, 0, 0);
	}

	void Camera::Move(const glm::vec3& move)
	{
		glm::vec3 frontMove = glm::normalize(m_ViewDir * glm::vec3(1.0f, 0.0f, 1.0f)) * move.z;
		glm::vec3 sideMove = glm::normalize(glm::cross(m_ViewDir, m_Up)) * move.x;
		glm::vec3 upMove(0.0f, move.y, 0.0f);
		m_Pos += frontMove + sideMove + upMove;
	}

	void Camera::RotateViewDir(float phi, float theta)
	{
		glm::vec3 phiAxis = m_Up;
		glm::mat3 phiMat = glm::rotate(glm::identity<glm::mat4>(), phi, phiAxis);

		glm::vec3 thetaAxis = glm::normalize(glm::cross(m_ViewDir, m_Up));
		glm::mat3 thetaMat = glm::rotate(glm::identity<glm::mat4>(), theta, thetaAxis);

		m_ViewDir = glm::normalize(thetaMat * phiMat * m_ViewDir);
	}

	const glm::vec3& Camera::GetPos() const
	{
		return m_Pos;
	}

	void Camera::SetPos(const glm::vec3& pos)
	{
		m_Pos = pos;
	}

	const glm::vec3& Camera::GetViewDir() const
	{
		return m_ViewDir;
	}

	void Camera::SetViewDir(const glm::vec3& viewDir)
	{
		m_ViewDir = glm::normalize(viewDir);
	}

	const glm::vec3& Camera::GetUp() const
	{
		return m_Up;
	}

	void Camera::SetUp(const glm::vec3& up)
	{
		m_Up = glm::normalize(up);
	}

	bool Camera::HasChanged() const
	{
		return m_Changed;
	}

	void Camera::SetChanged(bool changed)
	{
		m_Changed = changed;
	}

	float Camera::GetAspectRatio() const
	{
		return m_AspectRatio;
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
	}

	void Camera::SetAspectRatio(uint32_t width, uint32_t height)
	{
		if (height == 0)
			height = 1;
		m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
	}

	float Camera::GetFov() const
	{
		return m_Fov;
	}

	void Camera::SetFov(float fov)
	{
		m_Fov = fov;
	}

	float Camera::GetNearPlane() const
	{
		return m_NearPlane;
	}

	void Camera::SetNearPlane(float nearPlane)
	{
		m_NearPlane = nearPlane;
	}

	float Camera::GetFarPlane() const
	{
		return m_FarPlane;
	}

	void Camera::SetFarPlane(float farPlane)
	{
		m_FarPlane = farPlane;
	}

	VkDescriptorSet Camera::GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}
}
