#!/usr/bin/env python3

# Copyright (c) 2020 - UPMEM
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


from dpu import DpuSet
from dpu import ALLOCATE_ALL
import argparse
import os
import random
import struct
import sys

DPU_BINARY = 'dpu'

DPU_BUFFER = 'dpu_mram_buffer'
DPU_RESULTS = 'dpu_wram_results'
BUFFER_SIZE = 8 << 20
RESULT_SIZE = 12 # 4 for each new stuct var

ANSI_COLOR_RED = '\x1b[31m'
ANSI_COLOR_GREEN = '\x1b[32m'
ANSI_COLOR_RESET = '\x1b[0m'


def main(nr_dpus, nr_tasklets):
    ok = True

    with DpuSet(nr_dpus, binary = DPU_BINARY, log = sys.stdout) as dpus:
        print('Allocated {} DPU(s)'.format(len(dpus)))

        # must send 256 bytes at a time so pad with zeros, mark new line with x0D
        #num_list = [1,2,3,4,5]
        #num_length = len(num_list)
        #test_file = bytearray(256-num_length) + bytearray(num_list)
        #print(test_file)

        theor, test_file = create_test_file()

        print('Load input data')
        dpus.copy(DPU_BUFFER, test_file)
        
        print('Run program on DPU(s)')
        dpus.exec()

        results = [bytearray(RESULT_SIZE * nr_tasklets) for _ in dpus]
        dpus.copy(results, DPU_RESULTS)

        print('Retrieve results')
        for dpu, result in zip(dpus, results):
            dpu_checksum = 0
            dpu_cycles = 0
            dpu_counts = 0

            # Retrieve tasklet results and compute the final checksum.
            for task_id in range(nr_tasklets):
                result_checksum, result_cycles, result_counts = struct.unpack_from("<III", result, task_id * RESULT_SIZE)
                dpu_checksum += result_checksum
                dpu_cycles = max(dpu_cycles, result_cycles)
                dpu_counts += result_counts

            print('DPU execution time  = {:g} cycles'.format(dpu_cycles))
            print('performance         = {:g} cycles/byte'.format(dpu_cycles / BUFFER_SIZE))
            print('checksum computed by the DPU = 0x{:x}'.format(dpu_checksum))
            print('actual checksum value        = 0x{:x}'.format(theor))
            print('total counts all dpus        = {:d}'.format(dpu_counts))

            if dpu_checksum == theor:
                print('[' + ANSI_COLOR_GREEN + 'OK' + ANSI_COLOR_RESET + '] checksums are equal')
            else:
                print('[' + ANSI_COLOR_RED + 'ERROR' + ANSI_COLOR_RESET + '] checksums differ!')
                ok = False

    if not ok:
        sys.exit(os.EX_SOFTWARE)


def create_test_file():
    random.seed(0)
    test_file = bytearray([random.randrange(256) for _ in range(BUFFER_SIZE)])

    checksum = sum(test_file)

    return checksum, test_file


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('nr_dpus')
    parser.add_argument('nr_tasklets', type=int)
    args = parser.parse_args()

    if args.nr_dpus == 'DPU_ALLOCATE_ALL':
        nr_dpus = ALLOCATE_ALL
    else:
        try:
            nr_dpus = int(args.nr_dpus)
        except ValueError:
            parser.error("argument nr_dpus: invalid value: '{}'".format(args.nr_dpus))
            sys.exit(os.EX_USAGE)

    main(nr_dpus, args.nr_tasklets)
