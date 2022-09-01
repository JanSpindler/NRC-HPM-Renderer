layout(constant_id = 0) const uint RENDER_WIDTH = 1;
layout(constant_id = 1) const uint RENDER_HEIGHT = 1;

layout(constant_id = 2) const uint TRAIN_WIDTH = 1;
layout(constant_id = 3) const uint TRAIN_HEIGHT = 1;

layout(constant_id = 4) const uint POS_ENCODING = 1;
layout(constant_id = 5) const uint DIR_ENCODING = 1;

layout(constant_id = 6) const uint POS_FREQ_COUNT = 1;
layout(constant_id = 7) const uint POS_MIN_FREQ = 1;
layout(constant_id = 8) const uint POS_MAX_FREQ = 1;
layout(constant_id = 9) const uint POS_LEVEL_COUNT = 1;
layout(constant_id = 10) const uint POS_HASH_TABLE_SIZE = 1;
layout(constant_id = 11) const uint POS_FEATURE_COUNT = 1;

layout(constant_id = 12) const uint DIR_FREQ_COUNT = 1;
layout(constant_id = 13) const uint DIR_FEATURE_COUNT = 1;

layout(constant_id = 14) const uint LAYER_COUNT = 1;
layout(constant_id = 15) const uint LAYER_WIDTH = 1;
layout(constant_id = 16) const uint INPUT_FEATURE_COUNT = 1;
layout(constant_id = 17) const float NRC_LEARNING_RATE = 0.0;
layout(constant_id = 18) const float MRHE_LEARNING_RATE = 0.0;
layout(constant_id = 19) const uint BATCH_SIZE = 1;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

// TODO: performant?
#define ONE_OVER_RENDER_WIDTH (1.0 / float(RENDER_WIDTH))
#define ONE_OVER_RENDER_HEIGHT (1.0 / float(RENDER_HEIGHT))

#define ONE_OVER_DIR_FEATURE_COUNT (1.0 / float(DIR_FEATURE_COUNT))

const uint INPUT_WEIGHTS_COUNT = INPUT_FEATURE_COUNT * LAYER_WIDTH;
const uint HIDDEN_WEIGHTS_COUNT = LAYER_WIDTH * LAYER_WIDTH;
const uint OUTPUT_WEIGHTS_COUNT = LAYER_WIDTH * 3;

const uint TOTAL_WEIGHTS_COUNT = INPUT_WEIGHTS_COUNT + HIDDEN_WEIGHTS_COUNT + OUTPUT_WEIGHTS_COUNT;
const uint TOTAL_BIASES_COUNT = (LAYER_WIDTH * LAYER_COUNT) + 3;
const uint TOTAL_MRHE_COUNT = POS_FEATURE_COUNT * POS_LEVEL_COUNT * POS_HASH_TABLE_SIZE;

const uint NN_MAT_TYPE_OG = 0;
const uint NN_MAT_TYPE_DELTA = 1;
const uint NN_MAT_TYPE_MOMENTUM = 2;

const uint OUTPUT_NEURONS_INDEX = BATCH_SIZE * (INPUT_FEATURE_COUNT + (LAYER_WIDTH * LAYER_COUNT));
