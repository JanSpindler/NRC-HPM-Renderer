#include <engine/compute/Dataset.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	Dataset Dataset::FromMnist(KomputeManager& manager, const std::vector<std::vector<uint8_t>>& inputs, const std::vector<uint8_t>& labels)
	{
		if (inputs.size() != labels.size())
		{
			Log::Error("Mnist inputs and labels size not equal", true);
		}

		std::vector<Matrix> datasetInputs(inputs.size());
		std::vector<Matrix> datasetTargets(labels.size());
		for (size_t i = 0; i < inputs.size(); i++)
		{
			// Inputs
			const size_t mnistInputSize = 784;

			if (inputs[i].size() != mnistInputSize)
			{
				Log::Error("Mnist inputs needs size of " + std::to_string(mnistInputSize), true);
			}

			std::vector<std::vector<float>> inputMatData(mnistInputSize);
			for (size_t pixel = 0; pixel < mnistInputSize; pixel++)
			{
				inputMatData[pixel] = { static_cast<float>(inputs[i][pixel]) / 255.0f };
			}

			datasetInputs[i] = Matrix(manager, inputMatData);

			// Labels
			std::vector<std::vector<float>> targetMatData(10);
			for (size_t number = 0; number < 10; number++)
			{
				targetMatData[number] = 
					number == labels[i] ? 
					std::vector<float>{ 1.0f } : 
					std::vector<float>{ 0.0f };
			}

			datasetTargets[i] = Matrix(manager, targetMatData);
		}

		return Dataset(datasetInputs, datasetTargets);
	}

	Dataset::Dataset(const std::vector<Matrix>& inputs, const std::vector<Matrix>& targets) :
		m_Inputs(0),
		m_Targets(0)
	{
		if (inputs.size() != targets.size())
		{
			Log::Error("inputs and targets size not equal", true);
		}

		m_Inputs = inputs;
		m_Targets = targets;
	}

	void Dataset::SyncToDevice(KomputeManager& manager)
	{
		std::vector<std::shared_ptr<kp::Tensor>> syncTensors;
		for (size_t i = 0; i < m_Inputs.size(); i++)
		{
			syncTensors.push_back(m_Inputs[i].GetTensor());
			syncTensors.push_back(m_Targets[i].GetTensor());
		}

		manager.sequence()->record<kp::OpTensorSyncDevice>(syncTensors)->eval();
	}

	size_t Dataset::GetSize() const
	{
		return m_Inputs.size();
	}

	const Matrix& Dataset::GetInput(size_t index) const
	{
		if (index >= m_Inputs.size())
		{
			Log::Error("Requested input is out of bounds", true);
		}

		return m_Inputs[index];
	}

	const Matrix& Dataset::GetTarget(size_t index) const
	{
		if (index >= m_Targets.size())
		{
			Log::Error("Requested target is out of bounds", true);
		}

		return m_Targets[index];
	}
}
