layout(constant_id = 0) const uint RENDER_WIDTH = 1;
layout(constant_id = 1) const uint RENDER_HEIGHT = 1;

layout(constant_id = 2) const uint TRAIN_WIDTH = 1;
layout(constant_id = 3) const uint TRAIN_HEIGHT = 1;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

// TODO: performant?
#define ONE_OVER_RENDER_WIDTH (1.0 / float(RENDER_WIDTH))
#define ONE_OVER_RENDER_HEIGHT (1.0 / float(RENDER_HEIGHT))

const uint RENDER_SAMPLE_COUNT = RENDER_WIDTH * RENDER_HEIGHT;
const uint TRAIN_SAMPLE_COUNT = TRAIN_WIDTH * TRAIN_HEIGHT;

const uint RENDER_OVER_TRAIN_WIDTH = RENDER_WIDTH / TRAIN_WIDTH;
const uint RENDER_OVER_TRAIN_HEIGHT = RENDER_HEIGHT / TRAIN_HEIGHT;

const float PI = 3.14159265359;

const float MAX_RAY_DISTANCE = 100000.0;
const float MIN_RAY_DISTANCE = 0.125;

const uint SAMPLE_COUNT = 32;
