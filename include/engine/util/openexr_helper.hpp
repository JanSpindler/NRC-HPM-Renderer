#pragma once

namespace en
{
	bool ReadEXR(const char* name, float*& rgba, int& xRes, int& yRes, bool& hasAlpha);
	void WriteEXR(const char* name, float* pixels, int xRes, int yRes);
}
