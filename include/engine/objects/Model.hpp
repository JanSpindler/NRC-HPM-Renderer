#pragma once

#include <engine/objects/Mesh.hpp>
#include <engine/objects/Material.hpp>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>

namespace en
{
	class Model
	{
	public:
		Model(const std::string& filePath, bool flipUv);

		void Destroy();

		uint32_t GetMeshCount() const;

		const Mesh* GetMesh(uint32_t index) const;

		uint32_t GetMaterialCount() const;

		const Material* GetMaterial(uint32_t index) const;

		void SetMeshMaterial(uint32_t index, const Material* material);

	private:
		std::string m_FilePath;
		std::string m_Directory;

		std::vector<Mesh*> m_Meshes;
		std::vector<Material*> m_Materials;
		std::unordered_map<std::string, vk::Texture2D*> m_Textures;

		void ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentT);
		Mesh* ProcessMesh(aiMesh* mesh, const aiScene* scene, glm::mat4 t);
		void LoadMaterials(const aiScene* scene);
	};

	class ModelInstance
	{
	public:
		static const uint32_t MAX_COUNT = 8192;

		static void Init();
		static void Shutdown();

		static VkDescriptorSetLayout GetDescriptorSetLayout();

		ModelInstance(const Model* model, const glm::mat4& modelMat);

		void Destroy();

		const Model* GetModel() const;

		const glm::mat4& GetModelMat() const;
		void SetModelMat(const glm::mat4& modelMat);

		VkDescriptorSet GetDescriptorSet() const;

	private:
		static VkDescriptorSetLayout m_DescriptorSetLayout;
		static VkDescriptorPool m_DescriptorPool;

		const Model* m_Model;
		glm::mat4 m_ModelMat;

		vk::Buffer* m_UniformBuffer;
		VkDescriptorSet m_DescriptorSet;
	};
}
