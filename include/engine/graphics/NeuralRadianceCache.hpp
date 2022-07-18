#pragma once

#include <engine/graphics/vulkan/Buffer.hpp>
#include <array>

namespace en
{
	class NeuralRadianceCache
	{
	public:
		struct StatsData
		{
			float mseLoss;
		};

		static void Init(VkDevice device);
		static void Shutdown(VkDevice device);
		static VkDescriptorSetLayout GetDescSetLayout();

		NeuralRadianceCache(float learningRate, float weightDecay);

		void Destroy();

		void ResetStats();

		VkDescriptorSet GetDescSet() const;

		const StatsData& GetStats();

		void PrintWeights() const;

	private:
		struct ConfigData
		{
			float learningRate;
			float weightDecay;
		};
		
		static VkDescriptorSetLayout m_DescSetLayout;
		static VkDescriptorPool m_DescPool;

		std::array<vk::Buffer*, 6> m_Weights;
		std::array<vk::Buffer*, 6> m_DeltaWeights;
		
		std::array<vk::Buffer*, 6> m_Biases;
		std::array<vk::Buffer*, 6> m_DeltaBiases;

		ConfigData m_ConfigData;
		vk::Buffer m_ConfigUniformBuffer;

		StatsData m_StatsData;
		vk::Buffer m_StatsBuffer;

		VkDescriptorSet m_DescSet;

		void InitWeightBuffers();
		void InitBiasBuffers();
	};
}
