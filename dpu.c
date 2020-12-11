/*
 * Copyright (c) 2014-2017 - uPmem
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * An example of checksum computation with multiple tasklets.
 *
 * Every tasklet processes specific areas of the MRAM, following the "rake"
 * strategy:
 *  - Tasklet number T is first processing block number TxN, where N is a
 *    constant block size
 *  - It then handles block number (TxN) + (NxM) where M is the number of
 *    scheduled tasklets
 *  - And so on...
 *
 * The host is in charge of computing the final checksum by adding all the
 * individual results.
 */
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"

/* Use blocks of 256 bytes */
#define BLOCK_SIZE (256)

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t DPU_CACHES2[NR_TASKLETS][BLOCK_SIZE];
__host dpu_results_t DPU_RESULTS;

__mram uint64_t nRows;
__mram uint64_t nCols;
__mram uint64_t pSum;

__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];
__mram_noinit uint8_t DPU_BUFFER2[BUFFER_SIZE];
/*
void matrix_compress_csr(uint8_t m, uint8_t n, uint8_t matrix[m][n]);

void matrix_compress_csr(uint8_t m, uint8_t n, uint8_t matrix[m][n]){
    uint8_t totalSize=m*n;
    uint8_t csr[totalSize]; 
    uint8_t csrIndex[totalSize]; 
    uint8_t csrOffset[n+1];
    uint8_t i, j, k;
	for (i=0; i<m*n; i++)
	{
		csr[i]=0;
		csrIndex[i]=0;
	}
	uint8_t totolElement=0;
	for (j=0; j<n; j++)
	{
		csrOffset[j]=totolElement;
		for (i=0; i<m; i++)
		{
			if(matrix[j][i]!=0)
			{
				csr[totolElement]=matrix[j][i];
				csrIndex[totolElement]=i;				
				totolElement++;
			}
		}
	}
	csrOffset[n]=totolElement;
}
*/

/**
 * @fn main
 * @brief main function executed by each tasklet
 * @return the checksum result
 */
int main()
{
    uint32_t tasklet_id = me();
    uint8_t *cache = DPU_CACHES[tasklet_id];
    uint8_t *cache2 = DPU_CACHES2[tasklet_id];
    dpu_result_t *result = &DPU_RESULTS.tasklet_result[tasklet_id];
    uint32_t checksum = 0;
    uint64_t tmp_sum = 0;

    //uint64_t data = nRows;
    //uint64_t data2 = nCols;
    //nRows = data*data2;
    nRows = nRows * nCols;

    /* Initialize once the cycle counter */
    if (tasklet_id == 0)
        perfcounter_config(COUNT_CYCLES, true);

    for (uint32_t buffer_idx = tasklet_id * BLOCK_SIZE; buffer_idx < BUFFER_SIZE; buffer_idx += (NR_TASKLETS * BLOCK_SIZE)) {

        /* load cache with current mram block. */
        mram_read(&DPU_BUFFER[buffer_idx], cache, BLOCK_SIZE);
        mram_read(&DPU_BUFFER2[buffer_idx], cache2, BLOCK_SIZE);

        /* computes the checksum of a cached block */
        for (uint32_t cache_idx = 0; cache_idx < BLOCK_SIZE; cache_idx++) {
            tmp_sum += (cache[cache_idx] + cache2[cache_idx]);
            
            checksum += cache[cache_idx];
        }
    }

    /* keep the 32-bit LSB on the 64-bit cycle counter */
    result->cycles = (uint32_t)perfcounter_get();
    result->checksum = checksum;
    pSum = tmp_sum;

    printf("\nrpSum = %lu\n", pSum);
    //printf("[%02d] Checksum = 0x%08x\n", tasklet_id, result->checksum);
    return 0;
}
