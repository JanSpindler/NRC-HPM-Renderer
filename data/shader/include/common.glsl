#include "extensions.glsl"

#include "defines.glsl"

#ifdef NRC
#include "nrc-descriptors.glsl"
#include "nrc-constants.glsl"
#include "mrhe.glsl"
#include "oneblob.glsl"
#endif

#ifdef RESTIR
#include "restir-constants.glsl"
#include "restir-descriptors.glsl"
#endif

#include "random.glsl"
#include "volume.glsl"
#include "dir_gen.glsl"
#include "path_trace.glsl"
