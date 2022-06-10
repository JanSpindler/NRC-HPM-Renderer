#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/vulkan/CommandRecorder.hpp>
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <engine/util/openexr_helper.hpp>
#include <engine/compute/LinearLayer.hpp>

namespace en
{
	VkDescriptorSetLayout NrcHpmRenderer::m_DescriptorSetLayout;
	VkDescriptorSetLayout NrcHpmRenderer::m_NnDSL;
	VkDescriptorPool NrcHpmRenderer::m_DescriptorPool;

	void NrcHpmRenderer::Init(VkDevice device)
	{
		// Create descriptor set layout;
		VkDescriptorSetLayoutBinding lowPassImageBinding;
		lowPassImageBinding.binding = 0;
		lowPassImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		lowPassImageBinding.descriptorCount = 1;
		lowPassImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		lowPassImageBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { lowPassImageBinding };

		VkDescriptorSetLayoutCreateInfo layoutCI;
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);

		// Create NN descriptor set layout;
		// Weights
		VkDescriptorSetLayoutBinding weights0Binding;
		weights0Binding.binding = 0;
		weights0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights0Binding.descriptorCount = 1;
		weights0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		weights0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights1Binding;
		weights1Binding.binding = 1;
		weights1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights1Binding.descriptorCount = 1;
		weights1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		weights1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights2Binding;
		weights2Binding.binding = 2;
		weights2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights2Binding.descriptorCount = 1;
		weights2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		weights2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding weights3Binding;
		weights3Binding.binding = 3;
		weights3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		weights3Binding.descriptorCount = 1;
		weights3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		weights3Binding.pImmutableSamplers = nullptr;

		// Biases
		VkDescriptorSetLayoutBinding biases0Binding;
		biases0Binding.binding = 4;
		biases0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases0Binding.descriptorCount = 1;
		biases0Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		biases0Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases1Binding;
		biases1Binding.binding = 5;
		biases1Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases1Binding.descriptorCount = 1;
		biases1Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		biases1Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases2Binding;
		biases2Binding.binding = 6;
		biases2Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases2Binding.descriptorCount = 1;
		biases2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		biases2Binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding biases3Binding;
		biases3Binding.binding = 7;
		biases3Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		biases3Binding.descriptorCount = 1;
		biases3Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		biases3Binding.pImmutableSamplers = nullptr;

		bindings = {
			weights0Binding,
			weights1Binding,
			weights2Binding,
			weights3Binding,
			biases0Binding,
			biases1Binding,
			biases2Binding,
			biases3Binding};

		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext = nullptr;
		layoutCI.flags = 0;
		layoutCI.bindingCount = bindings.size();
		layoutCI.pBindings = bindings.data();

		result = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_NnDSL);
		ASSERT_VULKAN(result);

		// Create descriptor pool
		VkDescriptorPoolSize lowPassImagePoolSize;
		lowPassImagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		lowPassImagePoolSize.descriptorCount = 1;

		VkDescriptorPoolSize bufferPoolSize;
		bufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bufferPoolSize.descriptorCount = 8;

		std::vector<VkDescriptorPoolSize> poolSizes = { lowPassImagePoolSize, bufferPoolSize };

		VkDescriptorPoolCreateInfo poolCI;
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = 0;
		poolCI.maxSets = 10;
		poolCI.poolSizeCount = poolSizes.size();
		poolCI.pPoolSizes = poolSizes.data();

		result = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::Shutdown(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_NnDSL, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	NrcHpmRenderer::NrcHpmRenderer(
		uint32_t width,
		uint32_t height,
		const Camera* camera,
		const VolumeData* volumeData,
		const Sun* sun)
		:
		m_FrameWidth(width),
		m_FrameHeight(height),
		m_NrcForwardVertShader("nrc-forward/nrc-forward.vert", false),
		m_NrcForwardFragShader("nrc-forward/nrc-forward.frag", false),
		m_CommandPool(0, VulkanAPI::GetGraphicsQFI()),
		m_Camera(camera),
		m_VolumeData(volumeData),
		m_Sun(sun)
	{
		VkDevice device = VulkanAPI::GetDevice();

		CreateRenderPass(device);
		
		CreateNrcForwardResources(device);
		CreateNrcForwardPipelineLayout(device);
		CreateNrcForwardPipeline(device);
		
		CreateColorImage(device);
		
		CreateLowPassResources(device);
		CreateLowPassImage(device);

		CreateFramebuffer(device);

		m_CommandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CommandBuffer = m_CommandPool.GetBuffer(0);

		RecordCommandBuffer();
	}

	void NrcHpmRenderer::Render(VkQueue queue) const
	{
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::Destroy()
	{
		VkDevice device = VulkanAPI::GetDevice();

		m_CommandPool.Destroy();
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);

		vkDestroySampler(device, m_LowPassSampler, nullptr);
		vkDestroyImageView(device, m_LowPassImageView, nullptr);
		vkFreeMemory(device, m_LowPassImageMemory, nullptr);
		vkDestroyImage(device, m_LowPassImage, nullptr);
		
		vkDestroyImageView(device, m_ColorImageView, nullptr);
		vkFreeMemory(device, m_ColorImageMemory, nullptr);
		vkDestroyImage(device, m_ColorImage, nullptr);

		for (vk::Buffer* nrcForwardBuffer : m_NrcForwardBuffers)
		{
			nrcForwardBuffer->Destroy();
			delete nrcForwardBuffer;
		}

		vkDestroyPipelineLayout(device, m_NrcForwardPipelineLayout, nullptr);
		vkDestroyPipeline(device, m_NrcForwardPipeline, nullptr);
		m_NrcForwardVertShader.Destroy();
		m_NrcForwardFragShader.Destroy();

		vkDestroyRenderPass(device, m_RenderPass, nullptr);
	}

	void NrcHpmRenderer::ResizeFrame(uint32_t width, uint32_t height)
	{
		m_FrameWidth = width;
		m_FrameHeight = height;

		VkDevice device = VulkanAPI::GetDevice();

		// Destroy
		m_CommandPool.FreeBuffers();
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);

		vkDestroyImageView(device, m_LowPassImageView, nullptr);
		vkFreeMemory(device, m_LowPassImageMemory, nullptr);
		vkDestroyImage(device, m_LowPassImage, nullptr);

		vkDestroyImageView(device, m_ColorImageView, nullptr);
		vkFreeMemory(device, m_ColorImageMemory, nullptr);
		vkDestroyImage(device, m_ColorImage, nullptr);

		// Create
		CreateColorImage(device);
		CreateLowPassImage(device);
		CreateFramebuffer(device);

		m_CommandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CommandBuffer = m_CommandPool.GetBuffer(0);

		RecordCommandBuffer();
	}

	void NrcHpmRenderer::UpdateNnData(KomputeManager& manager, NeuralNetwork& nn)
	{
		nn.SyncLayersToHost(manager);
		std::vector<Layer*> layers = nn.GetLayers();

		size_t linearLayerIndex = 0;
		for (size_t i = 0; i < layers.size() && linearLayerIndex < 4; i++)
		{
			LinearLayer* linearLayer = dynamic_cast<LinearLayer*>(layers[i]);
			if (linearLayer != nullptr)
			{
				std::vector<float> weights = linearLayer->GetWeights().GetDataVector();
				std::vector<float> biases = linearLayer->GetBiases().GetDataVector();
				
				m_NrcForwardBuffers[linearLayerIndex]->SetData(weights.size(), weights.data(), 0, 0);
				m_NrcForwardBuffers[linearLayerIndex + 4]->SetData(biases.size(), biases.data(), 0, 0);

				linearLayerIndex++;
			}
		}
	}

	VkImage NrcHpmRenderer::GetImage() const
	{
		return m_ColorImage;
	}

	VkImageView NrcHpmRenderer::GetImageView() const
	{
		return m_ColorImageView;
	}

	size_t NrcHpmRenderer::GetImageDataSize() const
	{
		// Dependent on format
		return m_FrameWidth * m_FrameHeight * 4;
	}

	void NrcHpmRenderer::CreateRenderPass(VkDevice device)
	{
		// Color attachment
		VkAttachmentDescription colorAtt;
		colorAtt.flags = 0;
		colorAtt.format = VulkanAPI::GetSurfaceFormat().format;
		colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAtt.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkAttachmentReference colorAttRef;
		colorAttRef.attachment = 0;
		colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Subpass 0: Render with nn
		std::vector<VkAttachmentReference> subpass0ColorAttRefs = { colorAttRef };

		VkSubpassDescription subpass;
		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = subpass0ColorAttRefs.size();
		subpass.pColorAttachments = subpass0ColorAttRefs.data();
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = 0;

		std::vector<VkAttachmentDescription> attachments = { colorAtt };

		VkRenderPassCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &subpassDependency;

		VkResult result = vkCreateRenderPass(device, &createInfo, nullptr, &m_RenderPass);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateNrcForwardResources(VkDevice device)
	{
		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descSetAI;
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.pNext = nullptr;
		descSetAI.descriptorPool = m_DescriptorPool;
		descSetAI.descriptorSetCount = 1;
		descSetAI.pSetLayouts = &m_NnDSL;

		VkResult result = vkAllocateDescriptorSets(device, &descSetAI, &m_NrcForwardDS);
		ASSERT_VULKAN(result);

		// Set buffer sizes
		std::array<VkDeviceSize, 8> bufferSizes = {
			// Weights
			sizeof(float) * 5 * 64,
			sizeof(float) * 64 * 64,
			sizeof(float) * 64 * 64,
			sizeof(float) * 64 * 4,
			// Biases
			sizeof(float) * 64,
			sizeof(float) * 64,
			sizeof(float) * 64,
			sizeof(float) * 4, };

		// Create buffers
		for (size_t i = 0; i < bufferSizes.size(); i++)
		{
			m_NrcForwardBuffers[i] = new vk::Buffer(
				bufferSizes[i],
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				{});
		}

		// Set buffer infos
		std::array<VkDescriptorBufferInfo, 8> bufferInfos;
		for (size_t i = 0; i < bufferInfos.size(); i++)
		{
			bufferInfos[i].buffer = m_NrcForwardBuffers[i]->GetVulkanHandle();
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = bufferSizes[i];
		}

		// Set writes
		std::array<VkWriteDescriptorSet, 8> writes;
		for (size_t i = 0; i < writes.size(); i++)
		{
			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].pNext = nullptr;
			writes[i].dstSet = m_NrcForwardDS;
			writes[i].dstBinding = i;
			writes[i].dstArrayElement = 0;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writes[i].pImageInfo = nullptr;
			writes[i].pBufferInfo = &bufferInfos[i];
			writes[i].pTexelBufferView = nullptr;
		}

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	void NrcHpmRenderer::CreateNrcForwardPipelineLayout(VkDevice device)
	{
		std::vector<VkDescriptorSetLayout> layouts = { 
			Camera::GetDescriptorSetLayout(),
			VolumeData::GetDescriptorSetLayout(),
			Sun::GetDescriptorSetLayout(),
			m_DescriptorSetLayout,
			m_NnDSL };

		VkPipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.setLayoutCount = layouts.size();
		layoutCreateInfo.pSetLayouts = layouts.data();
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &m_NrcForwardPipelineLayout);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateNrcForwardPipeline(VkDevice device)
	{
		// Shader stage
		VkPipelineShaderStageCreateInfo vertStageCreateInfo;
		vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageCreateInfo.pNext = nullptr;
		vertStageCreateInfo.flags = 0;
		vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageCreateInfo.module = m_NrcForwardVertShader.GetVulkanModule();
		vertStageCreateInfo.pName = "main";
		vertStageCreateInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragStageCreateInfo;
		fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageCreateInfo.pNext = nullptr;
		fragStageCreateInfo.flags = 0;
		fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageCreateInfo.module = m_NrcForwardFragShader.GetVulkanModule();
		fragStageCreateInfo.pName = "main";
		fragStageCreateInfo.pSpecializationInfo = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertStageCreateInfo, fragStageCreateInfo };

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.pNext = nullptr;
		vertexInputCreateInfo.flags = 0;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.pNext = nullptr;
		inputAssemblyCreateInfo.flags = 0;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_FrameWidth);
		viewport.height = static_cast<float>(m_FrameHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { m_FrameWidth, m_FrameHeight };

		VkPipelineViewportStateCreateInfo viewportCreateInfo;
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.pNext = nullptr;
		viewportCreateInfo.flags = 0;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo;
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreateInfo.pNext = nullptr;
		rasterizerCreateInfo.flags = 0;
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizerCreateInfo.depthBiasClamp = 0.0f;
		rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizerCreateInfo.lineWidth = 1.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.pNext = nullptr;
		multisampleCreateInfo.flags = 0;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.pSampleMask = nullptr;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		// Depth and stencil testing
		/*VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo; // TODO
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.pNext = nullptr;
		depthStencilCreateInfo.flags = 0;
		depthStencilCreateInfo.depthTestEnable = VK_FALSE;
		depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilCreateInfo.front;
		depthStencilCreateInfo.back;
		depthStencilCreateInfo.minDepthBounds;
		depthStencilCreateInfo.maxDepthBounds;*/

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttStates = { colorBlendAttachment };

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.pNext = nullptr;
		colorBlendCreateInfo.flags = 0;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = colorBlendAttStates.size();
		colorBlendCreateInfo.pAttachments = colorBlendAttStates.data();
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		// Dynamic states
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pNext = nullptr;
		dynamicStateCreateInfo.flags = 0;
		dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

		// Creation
		VkGraphicsPipelineCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.stageCount = shaderStages.size();
		createInfo.pStages = shaderStages.data();
		createInfo.pVertexInputState = &vertexInputCreateInfo;
		createInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		createInfo.pTessellationState = nullptr;
		createInfo.pViewportState = &viewportCreateInfo;
		createInfo.pRasterizationState = &rasterizerCreateInfo;
		createInfo.pMultisampleState = &multisampleCreateInfo;
		createInfo.pDepthStencilState = nullptr; // TODO
		createInfo.pColorBlendState = &colorBlendCreateInfo;
		createInfo.pDynamicState = &dynamicStateCreateInfo;
		createInfo.layout = m_NrcForwardPipelineLayout;
		createInfo.renderPass = m_RenderPass;
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_NrcForwardPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateColorImage(VkDevice device)
	{
		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = VulkanAPI::GetSurfaceFormat().format;
		imageCI.extent = { m_FrameWidth, m_FrameHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // TODO: maybe?

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_ColorImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_ColorImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_ColorImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_ColorImage, m_ColorImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_ColorImage;
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = VulkanAPI::GetSurfaceFormat().format;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_ColorImageView);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateLowPassResources(VkDevice device)
	{
		// Create sampler
		VkSamplerCreateInfo samplerCI;
		samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCI.pNext = nullptr;
		samplerCI.flags = 0;
		samplerCI.magFilter = VK_FILTER_LINEAR;
		samplerCI.minFilter = VK_FILTER_LINEAR;
		samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCI.mipLodBias = 0.0f;
		samplerCI.anisotropyEnable = VK_FALSE;
		samplerCI.maxAnisotropy = 0.0f;
		samplerCI.compareEnable = VK_FALSE;
		samplerCI.compareOp = VK_COMPARE_OP_LESS;
		samplerCI.minLod = 0.0f;
		samplerCI.maxLod = 0.0f;
		samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCI.unnormalizedCoordinates = VK_FALSE;

		VkResult result = vkCreateSampler(device, &samplerCI, nullptr, &m_LowPassSampler);
		ASSERT_VULKAN(result);

		// Allocate descriptor set
		VkDescriptorSetAllocateInfo descAI;
		descAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descAI.pNext = nullptr;
		descAI.descriptorPool = m_DescriptorPool;
		descAI.descriptorSetCount = 1;
		descAI.pSetLayouts = &m_DescriptorSetLayout;

		result = vkAllocateDescriptorSets(device, &descAI, &m_DescriptorSet);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateLowPassImage(VkDevice device)
	{
		// Create Image
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = 0;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = VulkanAPI::GetSurfaceFormat().format;
		imageCI.extent = { m_FrameWidth, m_FrameHeight, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.queueFamilyIndexCount = 0;
		imageCI.pQueueFamilyIndices = nullptr;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkResult result = vkCreateImage(device, &imageCI, nullptr, &m_LowPassImage);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_LowPassImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_LowPassImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_LowPassImage, m_LowPassImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCI;
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.pNext = nullptr;
		imageViewCI.flags = 0;
		imageViewCI.image = m_LowPassImage;
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = VulkanAPI::GetSurfaceFormat().format;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCI, nullptr, &m_LowPassImageView);
		ASSERT_VULKAN(result);

		// Update descriptor set
		VkDescriptorImageInfo lowPassImageInfo;
		lowPassImageInfo.sampler = m_LowPassSampler;
		lowPassImageInfo.imageView = m_LowPassImageView;
		lowPassImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet lowPassImageWrite;
		lowPassImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		lowPassImageWrite.pNext = nullptr;
		lowPassImageWrite.dstSet = m_DescriptorSet;
		lowPassImageWrite.dstBinding = 0;
		lowPassImageWrite.dstArrayElement = 0;
		lowPassImageWrite.descriptorCount = 1;
		lowPassImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		lowPassImageWrite.pImageInfo = &lowPassImageInfo;
		lowPassImageWrite.pBufferInfo = nullptr;
		lowPassImageWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &lowPassImageWrite, 0, nullptr);

		// Change image layout to general
		vk::CommandPool commandPool(0, VulkanAPI::GetGraphicsQFI());
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		vk::CommandRecorder::ImageLayoutTransfer(
			commandBuffer,
			m_LowPassImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		result = vkEndCommandBuffer(commandBuffer);
		ASSERT_VULKAN(result);

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkQueue queue = VulkanAPI::GetGraphicsQueue();

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		commandPool.Destroy();
	}

	void NrcHpmRenderer::CreateFramebuffer(VkDevice device)
	{
		std::vector<VkImageView> attachments = { m_ColorImageView };

		VkFramebufferCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = m_RenderPass;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.width = m_FrameWidth;
		createInfo.height = m_FrameHeight;
		createInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &m_Framebuffer);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::RecordCommandBuffer()
	{
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
			Log::Error("Failed to begin VkCommandBuffer", true);

		std::vector<VkClearValue> clearValues = {
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = m_RenderPass;
		renderPassBeginInfo.framebuffer = m_Framebuffer;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { m_FrameWidth, m_FrameHeight };
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_NrcForwardPipeline);

		// Viewport
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_FrameWidth);
		viewport.height = static_cast<float>(m_FrameHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);

		// Scissor
		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { m_FrameWidth, m_FrameHeight };

		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);

		// Bind descriptor sets
		std::vector<VkDescriptorSet> descSets = {
			m_Camera->GetDescriptorSet(),
			m_VolumeData->GetDescriptorSet(),
			m_Sun->GetDescriptorSet(),
			m_DescriptorSet,
			m_NrcForwardDS };

		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_NrcForwardPipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Draw
		vkCmdDraw(m_CommandBuffer, 6, 1, 0, 0);

		// End render pass
		vkCmdEndRenderPass(m_CommandBuffer);

		// Copy result to low pass image
		VkImageCopy imageCopy;
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.mipLevel = 0;
		imageCopy.srcSubresource.baseArrayLayer = 0;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.srcOffset = { 0, 0, 0 };
		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.mipLevel = 0;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.dstOffset = { 0, 0, 0 };
		imageCopy.extent = { m_FrameWidth, m_FrameHeight, 1 };

		vkCmdCopyImage(
			m_CommandBuffer,
			m_ColorImage,
			VK_IMAGE_LAYOUT_GENERAL,
			m_LowPassImage,
			VK_IMAGE_LAYOUT_GENERAL,
			1,
			&imageCopy);

		// End
		result = vkEndCommandBuffer(m_CommandBuffer);
		if (result != VK_SUCCESS)
			Log::Error("Failed to end VkCommandBuffer", true);
	}
}
