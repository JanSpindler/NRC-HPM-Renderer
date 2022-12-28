layout(constant_id = 0) const uint RENDER_WIDTH = 1;
layout(constant_id = 1) const uint RENDER_HEIGHT = 1;
layout(constant_id = 2) const uint TRAIN_WIDTH = 1;
layout(constant_id = 3) const uint TRAIN_HEIGHT = 1;
layout(constant_id = 4) const uint TRAIN_X_DIST = 1;
layout(constant_id = 5) const uint TRAIN_Y_DIST = 1;
layout(constant_id = 6) const uint TRAIN_SPP = 1;
layout(constant_id = 7) const uint PRIMARY_RAY_LENGTH = 2;
layout(constant_id = 8) const float PRIMARY_RAY_PROB = 0.0;
layout(constant_id = 9) const uint TRAIN_RING_BUF_SIZE = 1;

layout(constant_id = 10) const uint INFER_BATCH_SIZE = 1;
layout(constant_id = 11) const uint TRAIN_BATCH_SIZE = 1;

layout(constant_id = 12) const float VOLUME_SIZE_X = 1.0;
layout(constant_id = 13) const float VOLUME_SIZE_Y = 1.0;
layout(constant_id = 14) const float VOLUME_SIZE_Z = 1.0;
layout(constant_id = 15) const float VOLUME_DENSITY_FACTOR = 1.0;
layout(constant_id = 16) const float VOLUME_G = 0.7;

layout(constant_id = 17) const float HDR_ENV_MAP_STRENGTH = 1.0;

const vec3 skySize = vec3(VOLUME_SIZE_X, VOLUME_SIZE_Y, VOLUME_SIZE_Z);
const vec3 skyPos = vec3(0.0);

// TODO: performant?
#define ONE_OVER_RENDER_WIDTH (1.0 / float(RENDER_WIDTH))
#define ONE_OVER_RENDER_HEIGHT (1.0 / float(RENDER_HEIGHT))

const uint RENDER_SAMPLE_COUNT = RENDER_WIDTH * RENDER_HEIGHT;
const uint TRAIN_SAMPLE_COUNT = TRAIN_WIDTH * TRAIN_HEIGHT;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;

const float MAX_RAY_DISTANCE = 100000.0;
const float MIN_RAY_DISTANCE = 0.125;
