#!/usr/bin/env python3

from dpu import DpuSet
from dpu import ALLOCATE_ALL
from io import StringIO
import argparse
import time
import struct
import sys

DPU_BINARY = 'dpu'

DPU_BUFFER = 'dpu_mram_buffer'
DPU_BUFFER2 = 'dpu_mram_buffer2'
DPU_RESULTS = 'dpu_wram_results'
BUFFER_SIZE = 8 << 20
RESULT_SIZE = 8

A_RED = '\x1b[31m'
A_GREEN = '\x1b[32m'
A_RESET = '\x1b[0m'

def main(nr_dpus, nr_tasklets, nr_rounds):
    nr_tasklets = 1
    start_timer = time.perf_counter()    
    with DpuSet(nr_dpus, binary = DPU_BINARY, log = sys.stdout) as dpus:
        nDpus = len(dpus)
        print('Allocated {} DPU(s)'.format(nDpus))
        
        dpus.nRows = bytearray([16,0,0,0,0,0,0,0])
        dpus.nCols = bytearray([16,0,0,0,0,0,0,0])
        dpus.pSum = bytearray([0,0,0,0,0,0,0,0])

        matrix_array =bytearray(
[0,5,0,0,0,0,2,0,0,4,0,2,0,7,0,0,
0,0,8,0,0,0,8,2,0,0,0,5,0,0,9,0,
0,9,0,0,3,0,0,9,0,0,0,8,0,0,0,2,
0,8,0,0,5,0,0,1,0,0,0,0,0,0,0,0,
0,0,0,0,9,8,0,0,0,9,5,2,4,0,8,0,
0,0,0,0,5,0,4,6,0,0,0,7,0,0,0,7,
4,0,9,0,0,4,0,0,0,0,0,0,0,0,5,3,
0,0,0,4,0,0,0,0,0,0,5,0,5,6,0,0,
0,3,0,0,0,0,6,8,9,4,0,3,0,0,0,0,
6,4,0,0,7,0,0,0,0,0,0,7,2,2,0,0,
6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,2,0,0,0,6,0,0,0,0,0,6,0,
0,0,0,2,0,0,3,8,0,0,5,0,0,3,0,0,
0,0,6,0,0,0,9,0,5,0,0,0,0,0,0,8,
0,2,0,0,0,0,0,0,0,3,0,3,8,9,0,0,
8,0,0,0,5,0,0,8,4,0,0,0,0,3,3,0])

        vector_array = bytearray([1,6,0,1,4,8,6,0,6,0,9,6,2,9,0,5])
        vector_array =  bytearray(vector_array) + bytearray(256-(len(vector_array)))     

        numRounds = nr_rounds

        totalPerf = 0
        totalCycles = 0
        numFail = 0
        for rnd in range(numRounds):
            print('Load matrix data')
            dpus.copy(DPU_BUFFER, matrix_array)

            print('Load vector data')
            dpus.copy(DPU_BUFFER2, vector_array)

            print('Run program on DPU(s)')
            dpus.exec()

            # actual y_vector expected
            y_vector = [117,78,124,68,165,121,51,113,126,122,6,44,92,124,127,79]
            y_sum_actual = sum(y_vector)

            results = [bytearray(RESULT_SIZE * nr_tasklets) for _ in dpus]
            dpus.copy(results, DPU_RESULTS)

            stop_timer = time.perf_counter()
            elapsed_time = abs(start_timer - stop_timer)

            for dpu, result in zip(dpus, results):
                dpu_result = 0
                dpu_cycles = 0
                for task_id in range(nr_tasklets):
                    result_t, cycles = struct.unpack_from("<II", result, task_id * RESULT_SIZE)
                    dpu_result += result_t
                    dpu_cycles = max(dpu_cycles, cycles)

                totalCycles += dpu_cycles
                totalPerf += dpu_cycles / BUFFER_SIZE
                print('\nDPU execution time  = {:g} cycles'.format(dpu_cycles))
                print('Performance         = {:g} cycles/byte'.format(dpu_cycles / BUFFER_SIZE))
                print(f'Seconds             =',round(elapsed_time,2))
                print('DPU v_sum           =',dpu_result)
                print('Expected v_sum      =',y_sum_actual)
                #print('pSum                =',dpus.pSum.uint64())

                if nDpus > 1:
                    y_sum_computed = sum(dpus.pSum.uint64())/nDpus
                else:
                    y_sum_computed = dpus.pSum.uint64()

                if y_sum_actual == y_sum_computed:
                    print('[' + A_GREEN + 'PASS' + A_RESET + '] Correct Vector Computed')
                else:
                    ++numFail
                    print('[' + A_RED + 'FAIL' + A_RESET + '] Wrong Vector Returned!')

        stop_timer_fin = time.perf_counter()
        print('----------------')
        print('Total Rounds  =',numRounds)
        print('Total errors  =',A_GREEN + 'None' + A_RESET if not numFail else A_RED + str(numFail) + A_RESET)
        print('Total cycles  = {:g} cycles:'.format(totalCycles))
        print('Avg cycles    = {:g} cycles:'.format(totalCycles/numRounds/nDpus))
        print('Total perf    = {:g} cycles/byte'.format(totalPerf))
        print('Avg perf      = {:g} cycles/byte'.format(totalPerf/numRounds/nDpus))
        print('Total time    =', round(abs(stop_timer_fin-start_timer),2))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('nr_dpus')
    parser.add_argument('nr_tasklets', type=int)
    parser.add_argument('nr_rounds', type=int)
    args = parser.parse_args()

    if args.nr_dpus == 'DPU_ALLOCATE_ALL':
        nr_dpus = ALLOCATE_ALL
    else:
        try:
            nr_dpus = int(args.nr_dpus)
        except ValueError:
            parser.error("argument nr_dpus: invalid value: '{}'".format(args.nr_dpus))
            sys.exit(os.EX_USAGE)

    main(nr_dpus, args.nr_tasklets, args.nr_rounds)

