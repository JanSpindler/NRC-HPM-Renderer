#pragma once

#include <engine/compute/Matrix.hpp>
#include <engine/compute/KomputeManager.hpp>
#include <engine/util/Log.hpp>

namespace en
{
	struct NrcInput
	{
		float x;
		float y;
		float z;
		float theta;
		float phi;
	};

	struct NrcTarget
	{
		float r;
		float g;
		float b;
		float a;
	};
}
