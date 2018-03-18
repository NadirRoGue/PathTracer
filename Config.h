#pragma once

//#define _RT_DEBUG
//#define _RT_MEASURE_PERFORMANCE

#define _USE_MATH_DEFINES
#include <math.h>

#define _RT_SUPERSAMPLING_SAMPLES 100

#define _RT_RAND_PRECISSION 10000

#define _RT_PROCESS_PER_PIXEL // Somehow is faster than processing batches of pixels O_o

#ifndef _RT_PROCESS_PER_PIXEL
#define _RT_PROCESS_PIXEL_BATCH_SIZE_X 4
#define _RT_PROCESS_PIXEL_BATCH_SIZE_Y 4
#endif

#define _RT_MAX_BOUNCES 4
#define _RT_BIAS 0.001f

#define _RT_DEG_TO_RAD (M_PI / 180.0f)
#define _RT_RAD_TO_DEG (180.0f / M_PI)

// During intersections test, transform rays to objects local space
// to account for transformations. Otherwise, affine transforms will be applied
#define _RT_TRANSFORM_RAY_TO_LOCAL_SPACE 

#define _RT_MC_PIXEL_SAMPLES 4
#define _RT_MC_BOUNCES_SAMPLES 8
