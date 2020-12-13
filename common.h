#ifndef __COMMON_H__
#define __COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x

/* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_BUFFER2 dpu_mram_buffer2
#define DPU_CACHES dpu_wram_caches
#define DPU_CACHES2 dpu_wram_caches2
#define DPU_CACHES3 dpu_wram_caches3
#define DPU_CACHES4 dpu_wram_caches4
#define DPU_CACHES5 dpu_wram_caches5
#define DPU_RESULTS dpu_wram_results

/* Size of the buffer on which the checksum will be performed */
#define BUFFER_SIZE (8 << 20)

/* Structure used by both the host and the dpu to communicate information */

#include <stdint.h>

#define NR_TASKLETS 1

typedef struct {
    uint32_t result_t;
    uint32_t cycles;
} dpu_result_t;

typedef struct {
    dpu_result_t tasklet_result[NR_TASKLETS];
} dpu_results_t;

#endif /* __COMMON_H__ */
