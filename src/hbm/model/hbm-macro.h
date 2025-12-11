// All parameteres quoted from JEDEC HBM2, HBM3 Spec Sheet

#ifndef HBM_MACRO_H
#define HBM_MACRO_H

#define H100_HBM

#if defined(MICRON_HBM_3E)
// The following implements Micron's 8H 24GB HBM3E device
    #define HBM_STACK_COUNT 6
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
    #define MAX_PHYSICAL_ADDRESS (0b110 << 28)

#else
    // Nvidia H100 HBM
    // According to the whitepaper, H100 has 80G of HBM, 5120 pins
    // 5120 = 1024 * 5, so 5 stacks
    // Each stack = 80 / 5 = 16 G
    // For HBM 3, each channel has 64 pins for I/O
    // Therefore, a total of 1024 / 64 = 16 channels
    // Therefore, each channel has 1GB = 8Gb of storage
    // Therefore, each PC has 4Gb of storage
    // Therefore, a total of 16 / 2 = 8 dies
    // This coincides with the 16Gb, 8High configuration of the table 3.2 in the HBM3 spec
    // We therefore have a total of 160 PCs, each stack, however, have only got 32 PCs
    #define HBM_STACK_COUNT 5
    #define HBM_DIE_COUNT 8
    #define HBM_CHANNEL_PER_DIE 2
    #define HBM_CHANNEL_COUNT (HBM_CHANNEL_PER_DIE * HBM_DIE_COUNT)
    #define HBM_PC_PER_CHANNEL 2
    #define HBM_PC_COUNT (HBM_CHANNEL_COUNT * HBM_PC_PER_CHANNEL)
    #define HBM_BANK_PER_PC 16
    #define HBM_PC_PER_STACK (HBM_PC_PER_CHANNEL * HBM_CHANNEL_COUNT)
    #define HBM_BANK_PER_BANK_GROUP 4
    #define HBM_BANK_GROUP_PER_PC (HBM_BANK_PER_PC / HBM_BANK_PER_BANK_GROUP)
    #define HBM_ROW_PER_BANK 32768
    // 2^15 rows per bank
    #define HBM_COLUMN_PER_ROW 32
    #define HBM_COLUMN_WIDTH_BIT 32
    #define HBM_COLUMN_WIDTH (HBM_COLUMN_WIDTH_BIT / 8)
    #define HBM_COLUMN_SIZE 32
    // each column contains 32 bytes, every read reads 4 bytes
    #define HBM_ROW_SIZE (HBM_COLUMN_PER_ROW * HBM_COLUMN_SIZE)
    // Each row is exactly 1KB = the page size
    #define HBM_BURST_LENGTH 8
    // burst length = 8 lets each burst operation transfer a whole column
    #define HBM_ATOMIC_SIZE (HBM_BURST_LENGTH * HBM_COLUMN_WIDTH)
    // 819 Gbps
    #define HBM_BUS_BANDWIDTH_BYTES (3352 / HBM_STACK_COUNT) 
    #define HBM_BUS_BANDWIDTH_PER_DIE HBM_BUS_BANDWIDTH_BYTES
    // Dies share the same bus, therefore each die still has all of the bandwidth
    #define HBM_BUS_BANDWIDTH_PER_PC (HBM_BUS_BANDWIDTH_PER_DIE / HBM_CHANNEL_PER_DIE / HBM_PC_PER_CHANNEL)
    // Converted to bytes

    #define HBM_CLOCK_FREQUENCY 3.6
    // GHz
    #define HBM_CAS_LATENCY 10
    // = int(10/1), CAS usually takes 10 cycles, clock frequency is around 1GHz
    #define HBM_TCCDL_LATENCY 1
    // Very quick
    #define HBM_PRECHARGE_LATENCY 10
    // 50 ns for row misses
    #define HBM_ACTIVE_ROW_LATENCY 50
    // Usually just 1 nanoseconds


    #define HBM_CONTROLLER_WRITE_BUFFER_MAX_SIZE 8
    #define HBM_CONTROLLER_WRITE_BUFFER_TIME_OUT 50
    #define HBM_MAX_BANK_QUEUE_SIZE 16
    #define HBM_MAX_PC_OTSD_LIMITS 64
    #define HBM_MAX_MC_OTSD_LIMITS 512
    #define OTHER_OVERHEAD 100
    #define TLB_ACCESS_LATENCY 10
    #define PAGE_TABLE_WALK_LATENCY 50
    #define TLB_MISS_PROBABILITY 0.2
    // latency for mem controller, NIC processing, packet processing, 100 ns is an estimate
    // Use larger packet or more nodes to amortize this delay

    #define INCLUDE_OTHER_OVERHEAD

    //-----------------------------------------
    // Memory Address Format:
    // Physical Address Bits
    // 36 35 34 33 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 
    // --------|--------------|--------------------------------------------|-----|-----|---------|----------
    //   sid   |      PC      |                   Row Id                   |  G  |  B  | Col Id  |   sub

    #define EXTRACT_OFFSET(x) ((x & 0b11111))
    #define EXTRACT_COLUMN(x) ((x & 0b1111100000ULL) >> 5ULL ) 
    #define EXTRACT_BANK(x) ((x & 0b110000000000ULL) >> 10ULL)
    #define EXTRACT_BANK_GROUP(x) ((x & 0b11000000000000ULL) >> 12ULL) 
    #define EXTRACT_ROW(x) ((x & 0b11111111111111100000000000000ULL) >> 14ULL)
    #define EXTRACT_PC(x) (( x & 0b1111100000000000000000000000000000ULL) >> 29ULL)
    #define EXTRACT_STACK(x) (( x & 0b1110000000000000000000000000000000000ULL) >> 34ULL)
    #define MAX_PHYSICAL_ADDRESS ((0b110ULL << 34ULL) - 1ULL)
    #define MAX_PHYSICAL_ADDRESS_UPPER_HALF 0b10011ULL
    #define MAP_TO_SAME_ROW_BIT_MASK 0b1111111111111111111111111110000000000ULL
    #define ROW_ALIGNED_AND_BIT_MASK (~(0b1111111111ULL))
#endif


#endif