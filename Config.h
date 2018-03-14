#pragma once

//#define _RT_DEBUG
#define _RT_MEASURE_PERFORMANCE

#define _RT_PROCESS_PER_PIXEL // Somehow is faster than processing batches of pixels O_o
#define _RT_PROCESS_PIXEL_BATCH_SIZE_X 4
#define _RT_PROCESS_PIXEL_BATCH_SIZE_Y 4

#define _RT_MAX_BOUNCES 4
#define _RT_BIAS 0.00005f
