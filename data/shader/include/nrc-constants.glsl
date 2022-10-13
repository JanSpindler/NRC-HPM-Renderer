layout(constant_id = 0) const uint RENDER_WIDTH = 1;
layout(constant_id = 1) const uint RENDER_HEIGHT = 1;

layout(constant_id = 2) const uint TRAIN_WIDTH = 1;
layout(constant_id = 3) const uint TRAIN_HEIGHT = 1;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

// TODO: performant?
#define ONE_OVER_RENDER_WIDTH (1.0 / float(RENDER_WIDTH))
#define ONE_OVER_RENDER_HEIGHT (1.0 / float(RENDER_HEIGHT))

#define ONE_OVER_DIR_FEATURE_COUNT (1.0 / float(DIR_FEATURE_COUNT))

const uint RENDER_SAMPLE_COUNT = RENDER_WIDTH * RENDER_HEIGHT;
const uint TRAIN_SAMPLE_COUNT = TRAIN_WIDTH * TRAIN_HEIGHT;
#define ONE_OVER_TRAIN_SAMPLE_COUNT (1.0 / float(TRAIN_SAMPLE_COUNT))

const uint RENDER_OVER_TRAIN_WIDTH = RENDER_WIDTH / TRAIN_WIDTH;
const uint RENDER_OVER_TRAIN_HEIGHT = RENDER_HEIGHT / TRAIN_HEIGHT;
