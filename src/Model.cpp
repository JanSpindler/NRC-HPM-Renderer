#include <engine/objects/Model.hpp>
#include <engine/util/Log.hpp>
#include <glm/gtx/transform.hpp>
#include <engine/graphics/VulkanAPI.hpp>

const std::string MODEL_DIR = "data/model/";

namespace en
{
    Model::Model(const std::string& filePath, bool flipUv) :
        m_FilePath(MODEL_DIR + filePath)
    {
        Log::Info("Loading model " + m_FilePath);
        
        m_Directory = m_FilePath.substr(0, std::max(m_FilePath.find_last_of('/'), m_FilePath.find_last_of('/')));

        // Import Scene
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            m_FilePath,
            aiProcess_Triangulate | (flipUv ? aiProcess_FlipUVs : 0));

        if (!scene ||
            scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
            !scene->mRootNode)
            Log::Error(std::string("Assimp Error - ") + importer.GetErrorString(), true);

        // Load Model structure
        Log::Info("\tModel has " + std::to_string(scene->mNumMeshes) + " Meshes");
        LoadMaterials(scene); // Load Material before Meshes because Meshes refence Materials
        ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
    }

    void Model::Destroy()
    {
        for (Mesh* mesh : m_Meshes)
        {
            mesh->DestroyVulkanBuffers();
            delete mesh;
        }

        for (Material* material : m_Materials)
        {
            material->Destroy();
            delete material;
        }

        for (const std::pair<std::string, vk::Texture2D*> entry : m_Textures)
        {
            vk::Texture2D* tex = entry.second;
            tex->Destroy();
            delete tex;
        }
    }

    uint32_t Model::GetMeshCount() const
    {
        return m_Meshes.size();
    }

    const Mesh* Model::GetMesh(uint32_t index) const
    {
        return m_Meshes[index];
    }

    uint32_t Model::GetMaterialCount() const
    {
        return m_Materials.size();
    }

    const Material* Model::GetMaterial(uint32_t index) const
    {
        return m_Materials[index];
    }

    void Model::SetMeshMaterial(uint32_t index, const Material* material)
    {
        m_Meshes[index]->SetMaterial(material);
    }

    void Model::ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentT)
    {
        aiMatrix4x4 localAiT = node->mTransformation;
        glm::mat4 localT(
            localAiT.a1, localAiT.b1, localAiT.c1, localAiT.d1,
            localAiT.a2, localAiT.b2, localAiT.c2, localAiT.d2,
            localAiT.a3, localAiT.b3, localAiT.c3, localAiT.d3,
            localAiT.a4, localAiT.b4, localAiT.c4, localAiT.d4);
        glm::mat4 totalT = parentT * localT;

        uint32_t meshCount = node->mNumMeshes;
        Log::Info("\tNode has " + std::to_string(meshCount) + " meshes");
        for (uint32_t i = 0; i < meshCount; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            m_Meshes.push_back(ProcessMesh(mesh, scene, totalT));
        }

        uint32_t childCount = node->mNumChildren;
        Log::Info("\tNode has " + std::to_string(childCount) + " children");
        for (uint32_t i = 0; i < node->mNumChildren; i++)
            ProcessNode(node->mChildren[i], scene, totalT);
    }

    Mesh* Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, glm::mat4 t)
    {
        glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(t)));

        std::vector<PNTVertex> vertices;
        std::vector<uint32_t> indices;

        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            glm::vec3 pos(0.0f);
            if (mesh->HasPositions())
            {
                pos.x = mesh->mVertices[i].x;
                pos.y = mesh->mVertices[i].y;
                pos.z = mesh->mVertices[i].z;
            }

            glm::vec3 normal(0.0f);
            if (mesh->HasNormals())
            {
                normal.x = mesh->mNormals[i].x;
                normal.y = mesh->mNormals[i].y;
                normal.z = mesh->mNormals[i].z;
            }

            glm::vec2 uv(0.0f);
            if (mesh->HasTextureCoords(0))
            {
                uv.x = mesh->mTextureCoords[0][i].x;
                uv.y = mesh->mTextureCoords[0][i].y;
            }

            pos = glm::vec3(t * glm::vec4(pos, 1.0f));
            normal = normalMat * normal;

            PNTVertex vert(pos, normal, uv);
            vertices.push_back(vert);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace& face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        return new Mesh(vertices, indices, m_Materials[mesh->mMaterialIndex]);
    }

    void Model::LoadMaterials(const aiScene* scene)
    {
        if (!scene->HasMaterials())
            return;

        uint32_t materialCount = scene->mNumMaterials;
        Log::Info("\tModel has " + std::to_string(materialCount) + " Materials");
        for (uint32_t i = 0; i < materialCount; i++)
        {
            Log::Info("\t\tMaterial[" + std::to_string(i) + "]:");

            aiMaterial* aiMat = scene->mMaterials[i];

            // Diffuse Texture
            uint32_t diffuseTextureCount = aiMat->GetTextureCount(aiTextureType_DIFFUSE);
            aiColor4D aiDiffuseColor;
            glm::vec4 diffuseColor;
            if (AI_SUCCESS != aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuseColor))
                diffuseColor = glm::vec4(1.0f);
            else
                diffuseColor = glm::vec4(aiDiffuseColor.r, aiDiffuseColor.g, aiDiffuseColor.b, aiDiffuseColor.a);
            Log::Info("\t\t\tDiffuse Color (" +
                std::to_string(diffuseColor.r) + ", " +
                std::to_string(diffuseColor.r) + ", " +
                std::to_string(diffuseColor.r) + ", " +
                std::to_string(diffuseColor.r) + ")");

            // Diffuse Textures
            uint32_t diffuseTexCount = aiMat->GetTextureCount(aiTextureType_DIFFUSE);
            Log::Info("\t\t\t" + std::to_string(diffuseTexCount) + " diffuse Textures found. Loading first.");
            vk::Texture2D* diffuseTex = nullptr;
            if (diffuseTexCount > 0)
            {
                aiString aiFileName;
                aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &aiFileName);
                std::string fullFilePath = m_Directory + "/" + aiFileName.C_Str();
                if (m_Textures.find(fullFilePath) == m_Textures.end())
                {
                    diffuseTex = new vk::Texture2D(fullFilePath, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
                    m_Textures.insert(std::pair<std::string, vk::Texture2D*>(fullFilePath, diffuseTex));
                }
                else
                {
                    diffuseTex = m_Textures.at(fullFilePath);
                }
            }

            // Create Material
            m_Materials.push_back(new Material(diffuseColor, diffuseTex));
        }
    }

    VkDescriptorSetLayout ModelInstance::m_DescriptorSetLayout;
    VkDescriptorPool ModelInstance::m_DescriptorPool;

    void ModelInstance::Init()
    {
        VkDevice device = VulkanAPI::GetDevice();

        // Create Descriptor Set Layout
        VkDescriptorSetLayoutBinding uniformBufferBinding;
        uniformBufferBinding.binding = 0;
        uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferBinding.descriptorCount = 1;
        uniformBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferBinding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { uniformBufferBinding };

        VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
        descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descSetLayoutCreateInfo.pNext = nullptr;
        descSetLayoutCreateInfo.flags = 0;
        descSetLayoutCreateInfo.bindingCount = layoutBindings.size();
        descSetLayoutCreateInfo.pBindings = layoutBindings.data();

        VkResult result = vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);
        ASSERT_VULKAN(result);

        // Create Descriptor Pool
        VkDescriptorPoolSize uniformBufferPoolSize;
        uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferPoolSize.descriptorCount = 1;

        std::vector<VkDescriptorPoolSize> poolSizes = { uniformBufferPoolSize };

        VkDescriptorPoolCreateInfo descPoolCreateInfo;
        descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolCreateInfo.pNext = nullptr;
        descPoolCreateInfo.flags = 0;
        descPoolCreateInfo.maxSets = MAX_COUNT;
        descPoolCreateInfo.poolSizeCount = poolSizes.size();
        descPoolCreateInfo.pPoolSizes = poolSizes.data();

        result = vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &m_DescriptorPool);
        ASSERT_VULKAN(result);
    }

    void ModelInstance::Shutdown()
    {
        VkDevice device = VulkanAPI::GetDevice();

        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    }

    VkDescriptorSetLayout ModelInstance::GetDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }

    ModelInstance::ModelInstance(const Model* model, const glm::mat4& modelMat) :
        m_Model(model),
        m_ModelMat(modelMat),
        m_UniformBuffer(new vk::Buffer(
            sizeof(glm::mat4),
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            {}))
    {
        m_UniformBuffer->SetData(sizeof(glm::mat4), &m_ModelMat, 0, 0);

        VkDevice device = VulkanAPI::GetDevice();

        VkDescriptorSetAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.descriptorPool = m_DescriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
        ASSERT_VULKAN(result);

        VkDescriptorBufferInfo uniformBufferInfo;
        uniformBufferInfo.buffer = m_UniformBuffer->GetVulkanHandle();
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = static_cast<VkDeviceSize>(sizeof(glm::mat4));

        VkWriteDescriptorSet uniformBufferWrite;
        uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferWrite.pNext = nullptr;
        uniformBufferWrite.dstSet = m_DescriptorSet;
        uniformBufferWrite.dstBinding = 0;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorCount = 1;
        uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferWrite.pImageInfo = nullptr;
        uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
        uniformBufferWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, 1, &uniformBufferWrite, 0, nullptr);
    }

    void ModelInstance::Destroy()
    {
        m_UniformBuffer->Destroy();
        delete m_UniformBuffer;
    }

    const Model* ModelInstance::GetModel() const
    {
        return m_Model;
    }

    const glm::mat4& ModelInstance::GetModelMat() const
    {
        return m_ModelMat;
    }

    void ModelInstance::SetModelMat(const glm::mat4& modelMat)
    {
        m_ModelMat = modelMat;
        m_UniformBuffer->SetData(sizeof(glm::mat4), &m_ModelMat, 0, 0);
    }

    VkDescriptorSet ModelInstance::GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }
}
