#include "common.h"

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t DPU_CACHES2[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t DPU_CACHES3[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t DPU_CACHES4[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t DPU_CACHES5[NR_TASKLETS][BLOCK_SIZE];
__host dpu_results_t DPU_RESULTS;

__mram uint64_t nRows;
__mram uint64_t nCols;
__mram uint64_t pSum;

__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];
__mram_noinit uint8_t DPU_BUFFER2[BUFFER_SIZE];

/*
__mram_noinit uint8_t csr[BLOCK_SIZE];
__mram_noinit uint8_t csrIndex[BLOCK_SIZE];
__mram_noinit uint8_t csrOffset[BLOCK_SIZE/2];
__mram_noinit uint8_t matrix[BLOCK_SIZE/2][BLOCK_SIZE/2];
__mram_noinit uint8_t y[BLOCK_SIZE/2];
*/

uint8_t __dma_aligned csr[BLOCK_SIZE];
uint8_t __dma_aligned csrIndex[BLOCK_SIZE];
uint8_t __dma_aligned csrOffset[BLOCK_SIZE/2];
uint8_t __dma_aligned matrix[BLOCK_SIZE/2][BLOCK_SIZE/2];
uint8_t __dma_aligned y[BLOCK_SIZE/2];

int main()
{
    uint32_t tasklet_id = me();
    uint8_t *cache_index = DPU_CACHES[tasklet_id];
    uint8_t *cache_val = DPU_CACHES2[tasklet_id];
    uint8_t *cache_ptr = DPU_CACHES3[tasklet_id];
    uint8_t *cache_y = DPU_CACHES4[tasklet_id];
    uint8_t *cache_x = DPU_CACHES5[tasklet_id];
    dpu_result_t *result = &DPU_RESULTS.tasklet_result[tasklet_id];
    uint32_t result_t = 0;
    uint8_t m = nRows;
    uint8_t n = nCols;
    uint8_t i, j, k = 0;

    if (tasklet_id == 0)
        perfcounter_config(COUNT_CYCLES, true);
    
    for (j=0, k=0; j<m; j++){
        for (i=0; i<n; i++){
            matrix[j][i] = DPU_BUFFER[k];
            k++;            
        }
    }

    // matrix compression, CSR //
    uint8_t totolElement = 0;
    for (j=0; j<n; j++){
        csrOffset[j]=totolElement;
        for (i=0; i<m; i++){
	    if(matrix[j][i]!=0){
	        csr[totolElement]=matrix[j][i];
		csrIndex[totolElement]=i;				
		totolElement++;
	    }
	}
    }
    csrOffset[n]=totolElement;

    // clear result vector from last round
    for (i=0; i < nCols; i++){
        y[i] = 0;
    }

    // SpMV CSR //
    uint8_t temp = 0;
    for(i = 0; i < nRows ; i++){
      temp = y[i];
      for(j = csrOffset[i]; j < csrOffset[i+1]; j++){
        temp += csr[j] * DPU_BUFFER2[csrIndex[j]];
      }
      y[i] = temp;
    }

/*
    for (uint32_t buffer_idx = tasklet_id * BLOCK_SIZE; buffer_idx < BUFFER_SIZE; buffer_idx += (NR_TASKLETS * BLOCK_SIZE)) {

        mram_read(&csrIndex[buffer_idx], cache_index, BLOCK_SIZE);
        mram_read(&csr[buffer_idx], cache_val, BLOCK_SIZE);
        mram_read(&csrOffset[buffer_idx], cache_ptr, BLOCK_SIZE);
        mram_read(&DPU_BUFFER2[buffer_idx], cache_x, BLOCK_SIZE);
        //mram_read(&y[buffer_idx], cache_y, BLOCK_SIZE);

        // SpMV CSR //
        uint8_t temp;
        for(i = 0; i < nRows ; i++){
            temp = y[i];
            for(j = cache_ptr[i]; j < cache_ptr[i+1]; j++){
                temp += cache_val[j] * DPU_BUFFER2[cache_index[j]];
            }
            y[i] = temp;
        }   
    }
*/

    pSum = 0;
    for (i=0; i < nRows; i++){
        pSum += y[i];
    }

    result->cycles = (uint32_t)perfcounter_get();
    result->result_t = result_t = pSum;

    //printf("\npSum ON DPU = %lu\n", pSum);
    printf("Computed y Vector:\n");
    for(int i = 0; i < nRows; i++) {
        printf("%d,", y[i]);
    } 

    //printf("[%02d] y0 = %d\n", tasklet_id, y[0]);
    return 0;
}
