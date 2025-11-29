// All parameteres quoted from JEDEC HBM3 Spec Sheet
// The following implements Micron's 8H 24GB HBM3E device
#ifndef HBM_MACRO_H
#define HBM_MACRO_H

#define HBM_STACK_COUNT 8
#define HBM_DIE_COUNT 4
#define HBM_CHANNEL_PER_DIE 2
#define HBM_CHANNEL_COUNT HBM_CHANNEL_PER_DIE * HBM_DIE_COUNT
#define HBM_PC_PER_CHANNEL 2
#define HBM_PC_COUNT HBM_CHANNEL_COUNT * HBM_PC_PER_CHANNEL
#define HBM_BANK_PER_PC 32
#define HBM_PC_PER_STACK HBM_PC_PER_CHANNEL * HBM_CHANNEL_COUNT
#define HBM_BANK_PER_BANK_GROUP 4
#define HBM_BANK_GROUP_PER_PC HBM_BANK_PER_PC / HBM_BANK_PER_BANK_GROUP
#define HBM_ROW_PER_BANK 16384
// 16384 rows per bank
#define HBM_COLUMN_PER_ROW 32
#define HBM_COLUMN_WIDTH_BIT 32
#define HBM_COLUMN_WIDTH HBM_COLUMN_WIDTH_BIT / 8
#define HBM_BURST_LENGTH 8
#define HBM_ATOMIC_SIZE HBM_BURST_LENGTH * HBM_COLUMN_WIDTH
#define HBM_BUS_BANDWIDTH_BITS 1400
// 819 Gbps
#define HBM_BUS_BANDWIDTH_BYTES 1400 / 8
#define HBM_BUS_BANDWIDTH_PER_DIE HBM_BUS_BANDWIDTH_BYTES
// Dies share the same bus, therefore each die still has all of the bandwidth
#define HBM_BUS_BANDWIDTH_PER_PC HBM_BUS_BANDWIDTH_PER_DIE / HBM_CHANNEL_PER_DIE / HBM_PC_PER_CHANNEL
// Converted to bytes

#define HBM_CLOCK_FREQUENCY 3.6
// GHz
#define HBM_CAS_LATENCY 3
// = int(10/3.6), CAS usually takes 10 cycles
#define HBM_ROW_MISS_PENALTY 100
// 50 ns for row misses
// Usually just 1 nanoseconds

//-----------------------------------------
// Memory Address Format:
// bit 63 - 31: not used (They are virtual address bits)
//  30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 
//  --------|--------|-----------------------------------------|----|---|---------|----     
//     SID  |  PC Id |                  Row Id                 | G  | B | Col Id  | sub

#define EXTRACT_COLUMN(x) ((x & 0b1111100) >> 2 ) 
#define EXTRACT_BANK(x) ((x & 0b110000000) >> 7)
#define EXTRACT_BANK_GROUP(x) (((x & 0b11000000000) >> 9) + ((x & 0b10000000000000000000000000000) >> 26))
#define EXTRACT_ROW(x) ((x & 0b1111111111111100000000000) >> 11)
#define EXTRACT_PC(x) (( x & 0b1110000000000000000000000000) >> 25)
#define EXTRACT_STACK(x) (( x & 0b1110000000000000000000000000000) >> 28)

#endif