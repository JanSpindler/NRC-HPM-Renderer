layout(constant_id = 0) const uint RENDER_WIDTH = 0;
layout(constant_id = 1) const uint RENDER_HEIGHT = 0;

layout(constant_id = 2) const uint TRAIN_WIDTH = 0;
layout(constant_id = 3) const uint TRAIN_HEIGHT = 0;

layout(constant_id = 4) const uint POS_ENCODING = 0;
layout(constant_id = 5) const uint DIR_ENCODING = 0;

layout(constant_id = 6) const uint POS_FREQ_COUNT = 0;
layout(constant_id = 7) const uint POS_MIN_FREQ = 0;
layout(constant_id = 8) const uint POS_MAX_FREQ = 0;
layout(constant_id = 9) const uint POS_LEVEL_COUNT = 0;
layout(constant_id = 10) const uint POS_HASH_TABLE_SIZE = 0;
layout(constant_id = 11) const uint POS_FEATURE_COUNt = 0;

layout(constant_id = 12) const uint DIR_FREQ_COUNT = 0;
layout(constant_id = 13) const uint DIR_FEATURE_COUNT = 0;

layout(constant_id = 14) const uint LAYER_COUNT = 0;
layout(constant_id = 15) const uint LAYER_WIDTH = 0;
layout(constant_id = 16) const uint INPUT_FEATURE_COUNT = 0;
layout(constant_id = 17) const float NRC_LEARNING_RATE;
layout(constant_id = 18) const float MRHE_LEARNING_RATE;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

const uint INPUT_WEIGHTS_COUNT = (LAYER_COUNT > 0) ? (INPUT_FEATURE_COUNT * LAYER_WIDTH) : (INPUT_FEATURE_COUNT * 3);
const uint HIDDEN_WEIGHTS_COUNT = LAYER_WIDTH * LAYER_WIDTH;
const uint OUTPUT_WEIGHTS_COUNT = (LAYER_COUNT > 0) ? (LAYER_WIDTH * 3) : (INPUT_FEATURE_COUNT * 3);
