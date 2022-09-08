layout(constant_id = 0) const uint RENDER_WIDTH = 1;
layout(constant_id = 1) const uint RENDER_HEIGHT = 1;

layout(constant_id = 2) const uint PATH_VERTEX_COUNT = 1;

layout(constant_id = 3) const uint SPATIAL_KERNEL_SIZE = 1;
layout(constant_id = 4) const uint TEMPORAL_KERNEL_SIZE = 1;

const vec3 skySize = vec3(125.0, 85.0, 153.0) / 2.0;
const vec3 skyPos = vec3(0.0);

const uint RENDER_PIXEL_COUNT = RENDER_WIDTH * RENDER_HEIGHT;

const int SPATIAL_KERNEL_MAX = int(SPATIAL_KERNEL_SIZE) / 2;
const int SPATIAL_KERNEL_MIN = -SPATIAL_KERNEL_MAX;

const uint RESERVOIR_SIZE = RENDER_PIXEL_COUNT * PATH_VERTEX_COUNT;

#define ONE_OVER_RENDER_WIDTH (1.0 / float(RENDER_WIDTH))
#define ONE_OVER_RENDER_HEIGHT (1.0 / float(RENDER_HEIGHT))
