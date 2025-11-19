# An adaption of Huawei's UnifiedBus Simulator.

## Requirements

The same requirements as the original UnifiedBus simulator. It is recommended to open up a python virtual environment, and clone the repo into it. Make sure to install all the python dependencies by following the original UnifiedBus quick start guide.   

## Functionalities Added

- Added a simple HBM model, with 1 HBM die, 8 banks per die, a burst length of 32 Bytes and an access latency of 50ns
- Integrated the HBM model onto the receiver to simulate real mem ops upon request reception

## Core Files

- **src/hbm/*:** simple HBM implementation.
- **src/unified-bus/protocol/ub-ldst-api.*:** modified, added logic to simulate real ld/st ops
- **src/unified-bus/ub-ldst-instance.*:** modified, added logic to simulate real ld/st ops

## Running Tests

Before testing, remember to first configure and build the project:

```
git submodule update --init --recursive
./ns3 configure
./ns3 build
```

Then you can run the simple test case hbm-ssim to test out the HBM model:

```
./ns3 run scratch/hbm-ssim
```

For running the network simulation, please refer to the original UB quick-start doc. All the provided examples can be used.  
Please note that the traffic type should not be URMA_WRITE or URMA_READ, because the above modifications are only for ld/st simulations;

## Remarks

- Currently assumes that each HBM bank blocks on the second request. This is generally enough for simulation purpuse, but for real HBMs, requests accessing the same row in the same bank are generally faster
- Currently assumes that requests map to each HBM bank with equal possibilities

