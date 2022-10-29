/*float preRand;
float prePreRand;

float rand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float myRand()
{
	float result = rand(vec2(preRand, prePreRand));
	prePreRand = preRand;
	preRand = result;
	return result;
}

float RandFloat(float maxVal)
{
	float f = myRand();
	return f * maxVal;
}*/

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uvec2 v) { return hash(v.x ^ hash(v.y)); }
uint hash(uvec3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
uint hash(uvec4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu;  // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u;       // 1.0 in IEEE binary32

    m &= ieeeMantissa;                      // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                           // Add fractional part to 1.0

    float  f = uintBitsToFloat(m);          // Range [1:2]
    return f - 1.0;                         // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random1(float x) { return floatConstruct(hash(floatBitsToUint(x))); }
float random2(vec2  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random3(vec3  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random4(vec4  v) { return floatConstruct(hash(floatBitsToUint(v))); }

float randomState;

void InitRandom(const vec2 fragUV)
{
    randomState = random2(vec2(random2(fragUV), random4(random)));
}

float RandFloat(float maxVal)
{
    randomState = random1(randomState);
    return randomState * maxVal;
}
