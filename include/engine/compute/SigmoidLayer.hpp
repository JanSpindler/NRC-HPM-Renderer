#include <engine/compute/Layer.hpp>

namespace en::vk
{
	class SigmoidLayer : public Layer
	{
	public:
		SigmoidLayer(uint32_t inputSize, uint32_t outputSize);

		void Destroy() override;

		void Forward(Matrix* input) override;
		void Backprop(Matrix* totalJacobian) override;

	private:
		void GetLocalJacobian(Matrix* localJacobian) override;

		static float Sigmoid(float x);
		static float SigmoidDeriv(float x);
	};
}
