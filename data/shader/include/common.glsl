#include "extensions.glsl"

#ifdef NRC
#include "nrc-descriptors.glsl"
#include "nrc-constants.glsl"
#endif

#ifdef RESTIR
#include "restir-constants.glsl"
#include "restir-descriptors.glsl"
#endif

#ifdef MC
#include "mc-constants.glsl"
#include "mc-descriptors.glsl"
#endif

#include "random.glsl"
#include "volume.glsl"
#include "dir_gen.glsl"
#include "path_trace.glsl"
