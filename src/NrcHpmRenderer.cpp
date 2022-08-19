#include <engine/graphics/renderer/NrcHpmRenderer.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	NrcHpmRenderer::NrcHpmRenderer(
		uint32_t width,
		uint32_t height,
		uint32_t trainWidth,
		uint32_t trainHeight,
		const Camera& camera,
		const VolumeData& volumeData,
		const DirLight& dirLight,
		const PointLight& pointLight,
		const HdrEnvMap& hdrEnvMap,
		const NeuralRadianceCache& nrc)
		:
		m_FrameWidth(width),
		m_FrameHeight(height),
		m_TrainWidth(trainWidth),
		m_TrainHeight(trainHeight),
		m_VertShader("nrc-forward/nrc-forward.vert", false),
		m_FragShader("nrc-forward/nrc-forward.frag", false),
		m_TrainShader("nrc-train/nrc-train.comp", false),
		m_StepShader("nrc-step/nrc-step.comp", false),
		m_MrheStepShader("mrhe-step/mrhe-step.comp", false),
		m_GenRaysShader("gen_rays/gen_rays.comp", false),
		m_CommandPool(0, VulkanAPI::GetGraphicsQFI()),
		m_Camera(camera),
		m_VolumeData(volumeData),
		m_DirLight(dirLight),
		m_PointLight(pointLight),
		m_HdrEnvMap(hdrEnvMap),
		m_Nrc(nrc)
	{
		VkDevice device = VulkanAPI::GetDevice();

		CreatePipelineLayout(device);

		InitSpecializationConstants();

		CreateRenderRenderPass(device);
		CreateRenderPipeline(device);

		CreateTrainPipeline(device);
		CreateStepPipeline(device);
		CreateMrheStepPipeline(device);
		CreateGenRaysShader(device);

		CreateColorImage(device);
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

		vkDestroyImageView(device, m_ColorImageView, nullptr);
		vkFreeMemory(device, m_ColorImageMemory, nullptr);
		vkDestroyImage(device, m_ColorImage, nullptr);

		vkDestroyPipeline(device, m_GenRaysPipeline, nullptr);
		m_GenRaysShader.Destroy();

		vkDestroyPipeline(device, m_MrheStepPipeline, nullptr);
		m_MrheStepShader.Destroy();

		vkDestroyPipeline(device, m_StepPipeline, nullptr);
		m_StepShader.Destroy();

		vkDestroyPipeline(device, m_TrainPipeline, nullptr);
		m_TrainShader.Destroy();

		vkDestroyPipeline(device, m_RenderPipeline, nullptr);
		m_VertShader.Destroy();
		m_FragShader.Destroy();

		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}

//	void NrcHpmRenderer::ResizeFrame(uint32_t width, uint32_t height)
//	{
//		m_FrameWidth = width;
//		m_FrameHeight = height;
//
//		VkDevice device = VulkanAPI::GetDevice();
//
//		// Destroy
//		m_CommandPool.FreeBuffers();
//		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
//
//		vkDestroyImageView(device, m_ColorImageView, nullptr);
//		vkFreeMemory(device, m_ColorImageMemory, nullptr);
//		vkDestroyImage(device, m_ColorImage, nullptr);
//
//		// Create
//		CreateColorImage(device);
//		CreateFramebuffer(device);
//
//		m_CommandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
//		m_CommandBuffer = m_CommandPool.GetBuffer(0);
//
//		RecordCommandBuffer();
//	}

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

	void NrcHpmRenderer::CreatePipelineLayout(VkDevice device)
	{
		std::vector<VkDescriptorSetLayout> layouts = {
			Camera::GetDescriptorSetLayout(),
			VolumeData::GetDescriptorSetLayout(),
			DirLight::GetDescriptorSetLayout(),
			NeuralRadianceCache::GetDescriptorSetLayout(),
			PointLight::GetDescriptorSetLayout(),
			HdrEnvMap::GetDescriptorSetLayout() };

		VkPipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.setLayoutCount = layouts.size();
		layoutCreateInfo.pSetLayouts = layouts.data();
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &m_PipelineLayout);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::InitSpecializationConstants()
	{
		// Fill struct
		m_SpecData.renderWidth = m_FrameWidth;
		m_SpecData.renderHeight = m_FrameHeight;
		
		m_SpecData.trainWidth = m_TrainWidth;
		m_SpecData.trainHeight = m_TrainHeight;

		m_SpecData.posEncoding = m_Nrc.GetPosEncoding();
		m_SpecData.dirEncoding = m_Nrc.GetDirEncoding();

		m_SpecData.posFreqCount = m_Nrc.GetPosFreqCount();
		m_SpecData.posMinFreq = m_Nrc.GetPosMinFreq();
		m_SpecData.posMaxFreq = m_Nrc.GetPosMaxFreq();
		m_SpecData.posLevelCount = m_Nrc.GetPosLevelCount();
		m_SpecData.posHashTableSize = m_Nrc.GetPosHashTableSize();
		m_SpecData.posFeatureCount = m_Nrc.GetPosFeatureCount();
		
		m_SpecData.dirFreqCount = m_Nrc.GetDirFreqCount();
		m_SpecData.dirFeatureCount = m_Nrc.GetDirFeatureCount();
		
		m_SpecData.layerCount = m_Nrc.GetLayerCount();
		m_SpecData.layerWidth = m_Nrc.GetLayerWidth();
		m_SpecData.inputFeatureCount = m_Nrc.GetDirFeatureCount();
		m_SpecData.nrcLearningRate = m_Nrc.GetNrcLearningRate();
		m_SpecData.mrheLearningRate = m_Nrc.GetMrheLearningRate();

		// Init map entries
		// Render size
		VkSpecializationMapEntry renderWidthEntry;
		renderWidthEntry.constantID = 0;
		renderWidthEntry.offset = offsetof(SpecializationData, SpecializationData::renderWidth);
		renderWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry renderHeightEntry;
		renderHeightEntry.constantID = 1;
		renderHeightEntry.offset = offsetof(SpecializationData, SpecializationData::renderHeight);
		renderHeightEntry.size = sizeof(uint32_t);

		// Train size
		VkSpecializationMapEntry trainWidthEntry;
		trainWidthEntry.constantID = 2;
		trainWidthEntry.offset = offsetof(SpecializationData, SpecializationData::trainWidth);
		trainWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry trainHeightEntry;
		trainHeightEntry.constantID = 3;
		trainHeightEntry.offset = offsetof(SpecializationData, SpecializationData::trainHeight);
		trainHeightEntry.size = sizeof(uint32_t);

		// Encoding
		VkSpecializationMapEntry posEncodingEntry;
		posEncodingEntry.constantID = 4;
		posEncodingEntry.offset = offsetof(SpecializationData, SpecializationData::posEncoding);
		posEncodingEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry dirEncodingEntry;
		dirEncodingEntry.constantID = 5;
		dirEncodingEntry.offset = offsetof(SpecializationData, SpecializationData::dirEncoding);
		dirEncodingEntry.size = sizeof(uint32_t);

		// Pos parameters
		VkSpecializationMapEntry posFreqCountEntry;
		posFreqCountEntry.constantID = 6;
		posFreqCountEntry.offset = offsetof(SpecializationData, SpecializationData::posFreqCount);
		posFreqCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry posMinFreqEntry;
		posMinFreqEntry.constantID = 7;
		posMinFreqEntry.offset = offsetof(SpecializationData, SpecializationData::posMinFreq);
		posMinFreqEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry posMaxFreqEntry;
		posMaxFreqEntry.constantID = 8;
		posMaxFreqEntry.offset = offsetof(SpecializationData, SpecializationData::posMaxFreq);
		posMaxFreqEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry posLevelCountEntry;
		posLevelCountEntry.constantID = 9;
		posLevelCountEntry.offset = offsetof(SpecializationData, SpecializationData::posLevelCount);
		posLevelCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry posHashTableSizeEntry;
		posHashTableSizeEntry.constantID = 10;
		posHashTableSizeEntry.offset = offsetof(SpecializationData, SpecializationData::posHashTableSize);
		posHashTableSizeEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry posFeatureCountEntry;
		posFeatureCountEntry.constantID = 11;
		posFeatureCountEntry.offset = offsetof(SpecializationData, SpecializationData::posFeatureCount);
		posFeatureCountEntry.size = sizeof(uint32_t);

		// Dir parameters
		VkSpecializationMapEntry dirFreqCountEntry;
		dirFreqCountEntry.constantID = 12;
		dirFreqCountEntry.offset = offsetof(SpecializationData, SpecializationData::dirFreqCount);
		dirFreqCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry dirFeatureCountEntry;
		dirFeatureCountEntry.constantID = 13;
		dirFeatureCountEntry.offset = offsetof(SpecializationData, SpecializationData::dirFeatureCount);
		dirFeatureCountEntry.size = sizeof(uint32_t);

		// Nn
		VkSpecializationMapEntry layerCountEntry;
		layerCountEntry.constantID = 14;
		layerCountEntry.offset = offsetof(SpecializationData, SpecializationData::layerCount);
		layerCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry layerWidthEntry;
		layerWidthEntry.constantID = 15;
		layerWidthEntry.offset = offsetof(SpecializationData, SpecializationData::layerWidth);
		layerWidthEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry inputFeatureCountEntry;
		inputFeatureCountEntry.constantID = 16;
		inputFeatureCountEntry.offset = offsetof(SpecializationData, SpecializationData::inputFeatureCount);
		inputFeatureCountEntry.size = sizeof(uint32_t);

		VkSpecializationMapEntry nrcLearningRateEntry;
		nrcLearningRateEntry.constantID = 17;
		nrcLearningRateEntry.offset = offsetof(SpecializationData, SpecializationData::nrcLearningRate);
		nrcLearningRateEntry.size = sizeof(float);

		VkSpecializationMapEntry mrheLearningRateEntry;
		mrheLearningRateEntry.constantID = 18;
		mrheLearningRateEntry.offset = offsetof(SpecializationData, SpecializationData::mrheLearningRate);
		mrheLearningRateEntry.size = sizeof(float);

		m_SpecMapEntries = {
			renderWidthEntry,
			renderHeightEntry,

			trainWidthEntry,
			trainHeightEntry,

			posEncodingEntry,
			dirEncodingEntry,

			posFreqCountEntry,
			posMinFreqEntry,
			posMaxFreqEntry,
			posLevelCountEntry,
			posHashTableSizeEntry,
			posFeatureCountEntry,

			dirFreqCountEntry,
			dirFeatureCountEntry,

			layerCountEntry,
			layerWidthEntry,
			inputFeatureCountEntry,
			nrcLearningRateEntry,
			mrheLearningRateEntry };

		VkSpecializationInfo specInfo;
		specInfo.mapEntryCount = m_SpecMapEntries.size();
		specInfo.pMapEntries = m_SpecMapEntries.data();
		specInfo.dataSize = sizeof(SpecializationData);
		specInfo.pData = &m_SpecData;
	}

	void NrcHpmRenderer::CreateRenderRenderPass(VkDevice device)
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

	void NrcHpmRenderer::CreateRenderPipeline(VkDevice device)
	{
		// Shader stage
		VkPipelineShaderStageCreateInfo vertStageCreateInfo;
		vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageCreateInfo.pNext = nullptr;
		vertStageCreateInfo.flags = 0;
		vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageCreateInfo.module = m_VertShader.GetVulkanModule();
		vertStageCreateInfo.pName = "main";
		vertStageCreateInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragStageCreateInfo;
		fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageCreateInfo.pNext = nullptr;
		fragStageCreateInfo.flags = 0;
		fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageCreateInfo.module = m_FragShader.GetVulkanModule();
		fragStageCreateInfo.pName = "main";
		fragStageCreateInfo.pSpecializationInfo = &m_SpecInfo;

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
		createInfo.layout = m_PipelineLayout;
		createInfo.renderPass = m_RenderPass;
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_RenderPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateTrainPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_TrainShader.GetVulkanModule();
		shaderStage.pName = "main";
		shaderStage.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = VK_PIPELINE_CREATE_DISPATCH_BASE_BIT;
		pipelineCI.stage = shaderStage;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_TrainPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateStepPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_StepShader.GetVulkanModule();
		shaderStage.pName = "main";
		shaderStage.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStage;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_StepPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateMrheStepPipeline(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_MrheStepShader.GetVulkanModule();
		shaderStage.pName = "main";
		shaderStage.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStage;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_MrheStepPipeline);
		ASSERT_VULKAN(result);
	}

	void NrcHpmRenderer::CreateGenRaysShader(VkDevice device)
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pNext = nullptr;
		shaderStage.flags = 0;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_GenRaysShader.GetVulkanModule();
		shaderStage.pName = "main";
		shaderStage.pSpecializationInfo = &m_SpecInfo;

		VkComputePipelineCreateInfo pipelineCI;
		pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = nullptr;
		pipelineCI.flags = 0;
		pipelineCI.stage = shaderStage;
		pipelineCI.layout = m_PipelineLayout;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCI.basePipelineIndex = 0;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_GenRaysPipeline);
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
		// Begin
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
			Log::Error("Failed to begin VkCommandBuffer", true);

		// Collect descriptor sets
		std::vector<VkDescriptorSet> descSets = {
			m_Camera.GetDescriptorSet(),
			m_VolumeData.GetDescriptorSet(),
			m_DirLight.GetDescriptorSet(),
			m_Nrc.GetDescriptorSet(),
			m_PointLight.GetDescriptorSet(),
			m_HdrEnvMap.GetDescriptorSet() };

		// Bind descriptor sets
		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Bind train pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_TrainPipeline);

		// Dispatch training
		vkCmdDispatch(m_CommandBuffer, 100, 100, 1);

		// Pipeline barrier
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Bind pipeline layout
		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Bind nrc step pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_StepPipeline);

		// Dispatch nrc gradient step
		vkCmdDispatch(m_CommandBuffer, 4096, 1, 1);

		// Pipeline barrier
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Bind pipeline layout
		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Bind mrhe step pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_MrheStepPipeline);

		// Dispatch mrhe gradient step
		vkCmdDispatch(m_CommandBuffer, m_SpecData.posLevelCount * m_SpecData.posFeatureCount * m_SpecData.posHashTableSize, 1, 1);

		// Pipeline barrier
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Begin render pass
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

		// Bind render pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderPipeline);

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

		// Bind desc sets
		vkCmdBindDescriptorSets(
			m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			0, descSets.size(), descSets.data(),
			0, nullptr);

		// Draw
		vkCmdDraw(m_CommandBuffer, 6, 1, 0, 0);

		// End render pass
		vkCmdEndRenderPass(m_CommandBuffer);

		// End
		result = vkEndCommandBuffer(m_CommandBuffer);
		if (result != VK_SUCCESS)
			Log::Error("Failed to end VkCommandBuffer", true);
	}
}
