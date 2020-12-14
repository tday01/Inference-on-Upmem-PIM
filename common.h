#ifndef __COMMON_H__
#define __COMMON_H__

#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#define NR_TASKLETS 1

#define BUFFER_SIZE (8 << 20)
#define BLOCK_SIZE (256)

#define DPU_BUFFER dpu_mram_buffer
#define DPU_BUFFER2 dpu_mram_buffer2
#define DPU_CACHES dpu_wram_caches
#define DPU_CACHES2 dpu_wram_caches2
#define DPU_CACHES3 dpu_wram_caches3
#define DPU_CACHES4 dpu_wram_caches4
#define DPU_CACHES5 dpu_wram_caches5
#define DPU_RESULTS dpu_wram_results

typedef struct {
    uint32_t result_t;
    uint32_t cycles;
} dpu_result_t;

typedef struct {
    dpu_result_t tasklet_result[NR_TASKLETS];
} dpu_results_t;

#endif // common.h
