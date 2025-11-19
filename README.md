# An adaption of Huawei's UnifiedBus Simulator.

## Requirements

The same requirements as the original UnifiedBus simulator

## Functionalities Added

- Added a simple HBM model, with 1 HBM die, 8 banks per die, a burst length of 32 Bytes and an access latency of 50ns
- Integrated the HBM model onto the receiver to simulate real mem ops upon request reception

## Core Files

- **src/hbm/*:** simple HBM implementation.
- **src/unified-bus/protocol/ub-ldst-api.*:** modified, added logic to simulate real ld/st ops
- **src/unified-bus/ub-ldst-instance.*:** modified, added logic to simulate real ld/st ops

## Running Tests

Please refer to the original UB quick-start doc. All the provided examples can be used.  
Please note that the traffic type should not be URMA_WRITE or URMA_READ, because the above modifications are only for ld/st simulations;

## Remarks

- Currently assumes that each HBM bank blocks on the second request. This is generally enough for simulation purpuse, but for real HBMs, requests accessing the same row in the same bank are generally faster
- Currently assumes that requests map to each HBM bank with equal possibilities

