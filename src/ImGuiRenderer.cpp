#include <engine/graphics/renderer/ImGuiRenderer.hpp>
#include <engine/graphics/VulkanAPI.hpp>
#include <engine/graphics/Window.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace en
{
	bool ImGuiRenderer::m_Initialized = false;
	uint32_t ImGuiRenderer::m_Width;
	uint32_t ImGuiRenderer::m_Height;

	VkDescriptorPool ImGuiRenderer::m_ImGuiDescriptorPool;

	VkDescriptorSetLayout ImGuiRenderer::m_DescriptorSetLayout;
	VkDescriptorPool ImGuiRenderer::m_DescriptorPool;

	VkImageView ImGuiRenderer::m_BackgroundImageView;
	VkSampler ImGuiRenderer::m_Sampler;
	VkDescriptorSet ImGuiRenderer::m_DescriptorSet;

	VkRenderPass ImGuiRenderer::m_RenderPass;
	vk::Shader* ImGuiRenderer::m_VertShader;
	vk::Shader* ImGuiRenderer::m_FragShader;
	VkPipelineLayout ImGuiRenderer::m_PipelineLayout;
	VkPipeline ImGuiRenderer::m_Pipeline;

	VkFormat ImGuiRenderer::m_Format;
	VkImage ImGuiRenderer::m_Image;
	VkDeviceMemory ImGuiRenderer::m_ImageMemory;
	VkImageView ImGuiRenderer::m_ImageView;

	VkFramebuffer ImGuiRenderer::m_Framebuffer;
	vk::CommandPool* ImGuiRenderer::m_CommandPool;
	VkCommandBuffer ImGuiRenderer::m_CommandBuffer;

	void ImGuiRenderer::Init(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		m_Format = VulkanAPI::GetSurfaceFormat().format;

		VkDevice device = VulkanAPI::GetDevice();

		CreateImGuiDescriptorPool(device);
		CreateDescriptorSetLayout(device);
		CreateDescriptorPool(device);
		CreateSampler(device);
		CreateDescriptorSet(device);
		CreateRenderPass(device);
		m_VertShader = new vk::Shader("draw_tex2D/draw_tex2D.vert", false);
		m_FragShader = new vk::Shader("draw_tex2D/draw_tex2D.frag", false);
		CreatePipelineLayout(device);
		CreatePipeline(device);
		CreateImageResources(device);
		CreateFramebuffer(device);
		CreateCommandPoolAndBuffer();

		InitImGuiBackend(device);

		m_Initialized = true;
	}

	void ImGuiRenderer::Shutdown()
	{
		m_Initialized = false;

		VkDevice device = VulkanAPI::GetDevice();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		m_CommandPool->Destroy();
		delete m_CommandPool;
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);

		vkDestroyImageView(device, m_ImageView, nullptr);
		vkFreeMemory(device, m_ImageMemory, nullptr);
		vkDestroyImage(device, m_Image, nullptr);

		vkDestroyPipeline(device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		m_VertShader->Destroy();
		delete m_VertShader;
		m_FragShader->Destroy();
		delete m_FragShader;
		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		vkDestroySampler(device, m_Sampler, nullptr);

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, m_ImGuiDescriptorPool, nullptr);
	}

	void ImGuiRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		VkDevice device = VulkanAPI::GetDevice();

		// Destroy
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkFreeMemory(device, m_ImageMemory, nullptr);
		vkDestroyImage(device, m_Image, nullptr);

		// Create
		CreateImageResources(device);
		CreateFramebuffer(device);
	}

	void ImGuiRenderer::StartFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiRenderer::EndFrame(VkQueue queue, VkSemaphore waitSemaphore)
	{
		// Calculate ImGui draw data
		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();

		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		// Begin renderPass
		VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = m_RenderPass;
		renderPassBeginInfo.framebuffer = m_Framebuffer;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { m_Width, m_Height };
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind pipeline
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		// Viewport
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_Width);
		viewport.height = static_cast<float>(m_Height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);

		// Scissor
		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { m_Width, m_Height };

		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);

		// Draw brackground image
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
		vkCmdDraw(m_CommandBuffer, 6, 1, 0, 0);

		// Vulkan render
		ImGui_ImplVulkan_RenderDrawData(drawData, m_CommandBuffer);

		// End renderPass
		vkCmdEndRenderPass(m_CommandBuffer);

		// End command buffer
		result = vkEndCommandBuffer(m_CommandBuffer);
		ASSERT_VULKAN(result);

		// Submit
		std::vector<VkSemaphore> waitSemaphores = {};
		if (waitSemaphore == VK_NULL_HANDLE) { waitSemaphores.push_back(waitSemaphore); }
		std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
		
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = waitSemaphores.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);
	}

	bool ImGuiRenderer::IsInitialized()
	{
		return m_Initialized;
	}

	VkImage ImGuiRenderer::GetImage()
	{
		return m_Image;
	}

	void ImGuiRenderer::SetBackgroundImageView(VkImageView backgroundImageView)
	{
		m_BackgroundImageView = backgroundImageView;

		VkDescriptorImageInfo imageInfo;
		imageInfo.sampler = m_Sampler;
		imageInfo.imageView = m_BackgroundImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet texWrite;
		texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		texWrite.pNext = nullptr;
		texWrite.dstSet = m_DescriptorSet;
		texWrite.dstBinding = 0;
		texWrite.dstArrayElement = 0;
		texWrite.descriptorCount = 1;
		texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texWrite.pImageInfo = &imageInfo;
		texWrite.pBufferInfo = nullptr;
		texWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(VulkanAPI::GetDevice(), 1, &texWrite, 0, nullptr);
	}

	void ImGuiRenderer::CreateImGuiDescriptorPool(VkDevice device)
	{
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.pNext = nullptr;
		descriptorPoolCreateInfo.flags = 0;
		descriptorPoolCreateInfo.maxSets = 1000;
		descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

		VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_ImGuiDescriptorPool);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateDescriptorSetLayout(VkDevice device)
	{
		VkDescriptorSetLayoutBinding texBinding;
		texBinding.binding = 0;
		texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texBinding.descriptorCount = 1;
		texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		texBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.bindingCount = 1;
		createInfo.pBindings = &texBinding;

		VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_DescriptorSetLayout);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateDescriptorPool(VkDevice device)
	{
		VkDescriptorPoolSize texPoolSize;
		texPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texPoolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &texPoolSize;

		VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &m_DescriptorPool);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateSampler(VkDevice device)
	{
		VkSamplerCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.magFilter = VK_FILTER_NEAREST;
		createInfo.minFilter = VK_FILTER_NEAREST;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.mipLodBias = 0.0f;
		createInfo.anisotropyEnable = VK_FALSE;
		createInfo.maxAnisotropy = 0.0f;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		VkResult result = vkCreateSampler(device, &createInfo, nullptr, &m_Sampler);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateDescriptorSet(VkDevice device)
	{
		VkDescriptorSetAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateRenderPass(VkDevice device)
	{
		VkAttachmentDescription colorAttachment;
		colorAttachment.flags = 0;
		colorAttachment.format = m_Format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkAttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass;
		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;
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

		VkRenderPassCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &subpassDependency;

		VkResult result = vkCreateRenderPass(device, &createInfo, nullptr, &m_RenderPass);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreatePipelineLayout(VkDevice device)
	{
		VkPipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &m_PipelineLayout);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreatePipeline(VkDevice device)
	{
		// Shader stage
		VkPipelineShaderStageCreateInfo vertStageCreateInfo;
		vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageCreateInfo.pNext = nullptr;
		vertStageCreateInfo.flags = 0;
		vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageCreateInfo.module = m_VertShader->GetVulkanModule();
		vertStageCreateInfo.pName = "main";
		vertStageCreateInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragStageCreateInfo;
		fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageCreateInfo.pNext = nullptr;
		fragStageCreateInfo.flags = 0;
		fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageCreateInfo.module = m_FragShader->GetVulkanModule();
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
		viewport.width = static_cast<float>(m_Width);
		viewport.height = static_cast<float>(m_Height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { m_Width, m_Height };

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

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.pNext = nullptr;
		colorBlendCreateInfo.flags = 0;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
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

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateImageResources(VkDevice device)
	{
		// Create Image
		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = m_Format;
		imageCreateInfo.extent = { m_Width, m_Height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkResult result = vkCreateImage(device, &imageCreateInfo, nullptr, &m_Image);
		ASSERT_VULKAN(result);

		// Image Memory
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, m_Image, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanAPI::FindMemoryType(
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_ImageMemory);
		ASSERT_VULKAN(result);

		result = vkBindImageMemory(device, m_Image, m_ImageMemory, 0);
		ASSERT_VULKAN(result);

		// Create image view
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = m_Image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = m_Format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateFramebuffer(VkDevice device)
	{
		VkFramebufferCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = m_RenderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &m_ImageView;
		createInfo.width = m_Width;
		createInfo.height = m_Height;
		createInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &m_Framebuffer);
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::CreateCommandPoolAndBuffer()
	{
		m_CommandPool = new vk::CommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VulkanAPI::GetGraphicsQFI());
		m_CommandPool->AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		m_CommandBuffer = m_CommandPool->GetBuffer(0);
	}

	void CheckVkResult(VkResult result)
	{
		ASSERT_VULKAN(result);
	}

	void ImGuiRenderer::InitImGuiBackend(VkDevice device)
	{
		uint32_t qfi = VulkanAPI::GetGraphicsQFI();
		VkQueue queue = VulkanAPI::GetGraphicsQueue();
		uint32_t imageCount = 2; // Not correct but working

		// Init imgui backend
		IMGUI_CHECKVERSION();
		ImGui::SetCurrentContext(ImGui::CreateContext());
		ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWHandle(), true);

		ImGui_ImplVulkan_InitInfo implVulkanInitInfo = {};
		implVulkanInitInfo.Instance = VulkanAPI::GetInstance();
		implVulkanInitInfo.PhysicalDevice = VulkanAPI::GetPhysicalDevice();
		implVulkanInitInfo.Device = VulkanAPI::GetDevice();
		implVulkanInitInfo.QueueFamily = qfi;
		implVulkanInitInfo.Queue = queue;
		implVulkanInitInfo.PipelineCache = nullptr;
		implVulkanInitInfo.DescriptorPool = m_ImGuiDescriptorPool;
		implVulkanInitInfo.Subpass = 0;
		implVulkanInitInfo.MinImageCount = imageCount;
		implVulkanInitInfo.ImageCount = imageCount;
		implVulkanInitInfo.Allocator = nullptr;
		implVulkanInitInfo.CheckVkResultFn = CheckVkResult;
		if (!ImGui_ImplVulkan_Init(&implVulkanInitInfo, m_RenderPass))
			Log::Error("Failed to Init ImGuiManager", true);

		// Init imgui on GPU
		vk::CommandPool commandPool(0, qfi);
		commandPool.AllocateBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer commandBuffer = commandPool.GetBuffer(0);

		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		ASSERT_VULKAN(result);

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

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

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT_VULKAN(result);

		result = vkQueueWaitIdle(queue);
		ASSERT_VULKAN(result);

		commandPool.Destroy();
	}
}
