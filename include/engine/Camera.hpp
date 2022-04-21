#pragma once

#include <engine/common.hpp>
#include <glm/glm.hpp>
#include <engine/vulkan/Buffer.hpp>

namespace en
{
	const uint32_t MAX_CAMERA_COUNT = 16;

	struct CameraMatrices
	{
		glm::mat4 projView;
		glm::mat4 oldProjView;
	};

	class Camera
	{
	public:
		static void Init();
		static void Shutdown();

		static VkDescriptorSetLayout GetDescriptorSetLayout();

		Camera(const glm::vec3& pos, const glm::vec3& viewDir, const glm::vec3& up, float aspectRatio, float fov, float nearPlane, float farPlane);

		void Destroy();
		void UpdateUniformBuffer();

		void Move(const glm::vec3& move);
		void RotateViewDir(float phi, float theta);

		const glm::vec3& GetPos() const;
		void SetPos(const glm::vec3& pos);

		const glm::vec3& GetViewDir() const;
		void SetViewDir(const glm::vec3& viewDir);

		const glm::vec3& GetUp() const;
		void SetUp(const glm::vec3& up);

		float GetAspectRatio() const;
		void SetAspectRatio(float aspectRatio);
		void SetAspectRatio(uint32_t width, uint32_t height);

		float GetFov() const;
		void SetFov(float fov);

		float GetNearPlane() const;
		void SetNearPlane(float nearPlane);

		float GetFarPlane() const;
		void SetFarPlane(float farPlane);

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		glm::vec3 m_Pos;
		glm::vec3 m_ViewDir;
		glm::vec3 m_Up;

		float m_AspectRatio;
		float m_Fov;
		float m_NearPlane;
		float m_FarPlane;

		VkDescriptorSet m_DescriptorSet;
		CameraMatrices m_Matrices;
		vk::Buffer* m_MatrixUniformBuffer;
		vk::Buffer* m_PosUniformBuffer;
	};
}
