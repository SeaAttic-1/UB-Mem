# ns-3-UB: Unified-Bus Network Simulation Framework

**Language**: [English](README_en.md) | [中文](README.md)

[Quick Start Guide](QUICK_START_en.md)

## Project Overview

`ns-3-UB` is an ns-3 simulation module built based on the [UnifiedBus™ (UB) Base Specification](https://www.unifiedbus.com/zh), implementing the protocol frameworks and supporting algorithms for the function layer, transaction layer, transport layer, network layer, and data link layer defined in the UB Base Specification. This project aims to provide a simulation platform for protocol innovation, network architecture exploration, and research on network algorithms such as congestion control, flow control, load balancing, and routing algorithms.

> Although every effort has been made to align with the UB Base Specification, differences still exist between the two. Please refer to the UB Base Specification as the authoritative guide.

`ns-3-UB` can be used to research UB protocol-based:
- Traffic pattern affinity, low-cost, highly reliable innovative topological architectures.
- Collective communication operators and traffic orchestration algorithm optimization techniques.
- New transaction layer ordering and reliability techniques for bus memory transaction networking scenarios.
- New memory semantic transport control techniques for super-node networks.
- Innovative adaptive routing, load balancing, congestion control, and QoS optimization algorithms.

> This project provides pluggable "reference implementations" for strategies/algorithms not specified in the specification (such as switch modeling methods, routing selection, congestion marking, buffering and arbitration strategies, etc.). These implementations are not part of the UB Base Specification and serve only as examples/baselines that can be replaced or disabled.
>
> Functions not included in this project include but are not limited to: hardware internal detail modeling, physical layer, performance parameters, control plane behavior (such as initialization behavior, exception event handling, etc.), memory management, security policies, etc.

The **typical simulation capabilities** supported by this project are shown in the following table.

<table>
  <thead>
    <tr>
      <th align="center">Layer</th>
      <th align="center">Capability Category</th>
      <th align="left">Supported Features</th>
      <th align="left">Incomplete / User-Customizable Features</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center" rowspan="2">Function Layer</td>
      <td>Function Types</td>
      <td>Ld/St interface, URMA interface</td>
      <td>URPC, Entity-related advanced features</td>
    </tr>
    <tr>
      <td>Jetty</td>
      <td>Jetty, many-to-many / one-to-one communication models</td>
      <td>Jetty Group, Jetty state machine</td>
    </tr>
    <tr>
      <td align="center" rowspan="3">Transaction Layer</td>
      <td>Service Modes</td>
      <td>ROI, ROL, UNO</td>
      <td>ROT</td>
    </tr>
    <tr>
      <td>Transaction Ordering</td>
      <td>NO / RO / SO equivalent functional implementation</td>
      <td>—</td>
    </tr>
    <tr>
      <td>Transaction Types</td>
      <td>Write, Read, Send</td>
      <td>Atomic, Write_with_notify, etc.</td>
    </tr>
    <tr>
      <td align="center" rowspan="4">Transport Layer</td>
      <td>Service Modes</td>
      <td>RTP</td>
      <td>UTP, CTP</td>
    </tr>
    <tr>
      <td>Reliability</td>
      <td>PSN, timeout retransmission, Go-Back-N</td>
      <td>Selective retransmission, fast retransmission</td>
    </tr>
    <tr>
      <td>Congestion Control</td>
      <td>RTP congestion control mechanism, CAQM</td>
      <td>LDCP, DCQCN algorithm, CTP congestion control mechanism</td>
    </tr>
    <tr>
      <td>Load Balancing</td>
      <td>RTP load balancing: TPG mechanism, per-TP / per-packet load balancing, out-of-order reception</td>
      <td>CTP load balancing</td>
    </tr>
    <tr>
      <td align="center" rowspan="5">Network Layer</td>
      <td>Packet Format</td>
      <td>Support for IPv4 address format-based headers, 16-bit CNA address format-based headers</td>
      <td>Other header formats</td>
    </tr>
    <tr>
      <td>Address Management</td>
      <td>CNA to IP address translation, Primary / Port CNA</td>
      <td>User-customizable address allocation and translation strategies</td>
    </tr>
    <tr>
      <td>Routing Lookup</td>
      <td>Basic routing strategy based on destination address + header RT field, routing strategy based on path Cost, Hash-based ECMP, per-flow / per-packet Hash based on load balancing factors</td>
      <td>User-customizable adaptive routing and other strategies</td>
    </tr>
    <tr>
      <td>Quality of Service</td>
      <td>SL-VL mapping, SP-based inter-VL scheduling</td>
      <td>User-customizable inter-VL scheduling strategies (such as DWRR, etc.)</td>
    </tr>
    <tr>
      <td>Congestion Marking</td>
      <td>CAQM marking mode based on header CC field</td>
      <td>FECN / FECN_RTT marking modes</td>
    </tr>
    <tr>
      <td align="center" rowspan="3">Data Link Layer</td>
      <td>Packet Transmission</td>
      <td>Packet-level modeling</td>
      <td>Cell / Flit level modeling</td>
    </tr>
    <tr>
      <td>Virtual Channels</td>
      <td>Point-to-point links support up to 16 VLs, SP-based inter-VL scheduling</td>
      <td>DWRR-based inter-VL scheduling</td>
    </tr>
    <tr>
      <td>Credit Flow Control</td>
      <td>Credit exclusive mode, CBFC, PFC adaptation</td>
      <td>Control plane credit initialization behavior, credit sharing mode</td>
    </tr>
  </tbody>
</table>

## Project Architecture

```
├── README.md                   # Project documentation
├── scratch/                    # Simulation examples and test cases
│   ├── ub-quick-example.cc     # Main simulation program
│   ├── test_CLOS/              # CLOS topology test cases
│   ├── 2dfm4_4*/               # 2D FullMesh 4x4 test case set
│
└── src/unified-bus/            # UB Base Specification-based simulation components
    ├── model/                  
    │   ├── protocol/
    │   │   └── ub-*            # Protocol stack related modeling components
    │   └── ub-*                # Network element and algorithm components
    ├── test/                   # Unit tests
    └── doc/                    # Documentation and diagrams
```

## Core Components

### 1. UnifiedBus (UB) Module

The UB module is a simulation component implemented based on the UB Base Specification:

#### Network Element Modeling Components
<p align="center">
<img src="src/unified-bus/doc/figures/arch2-light.D3-LpLKH.png.png" alt="UB Domain System Architecture" width="85%">
<br>
<em>UB Domain system architecture diagram.</em><em>Source: www.unifiedbus.com</em><br>
</p>

- **UB Controller** (`ub-controller.*`) - Key component for executing UB protocol stack, providing user interfaces
- **UB Switch** (`ub-switch.*`) - Used for data forwarding between UB ports
- **UB Port** (`ub-port.*`) - Port abstraction, handling packet input and output
- **UB Link** (`ub-link.*`) - Point-to-point connections between nodes

#### Protocol Stack Components
- **Programming Interface Instances** (`ub-api-ldst*`, `ub-app.*`) - Load/Store and URMA programming interface instances, interfacing with function layer programming models
- **UB Function** (`ub-function.*`) - Function layer protocol framework implementation, supporting Load/Store and URMA programming models
- **UB Transaction** (`ub-transaction.*`) - Transaction layer protocol framework implementation
- **UB Transport** (`ub-transport.*`) - Transport layer protocol framework implementation
- **UB Network** (composed of `ub-routing-table.*`, `ub-congestion-ctrl.*`, `ub-switch.*` functionalities) - Network layer protocol framework implementation
- **UB Datalink** (`ub-datalink.*`) - Data link layer protocol framework implementation

#### Network Algorithm Components
- **Traffic Injection Component** (`ub-traffic-gen.*`) - Reads user traffic configuration, injects traffic for simulation nodes according to serial and parallel relationships
- **TP Connection Manager** (`ub-tp-connection-manager.h`) - TP Channel manager, facilitating user lookup of TP Channel information for each node
- **Switch Allocator** (`ub-allocator.*`) - Models the process of switch allocating output ports for packets
- **Queue Manager** (`ub-buffer-manager.*`) - Buffer management module, affecting load balancing, flow control, queuing, packet dropping and other behaviors
- **Routing Process** (`ub-routing-process.*`) - Routing table module, implementing routing table management and query functionality
- **Congestion Control** (`ub-congestion-control.*`) - Congestion control algorithm framework module
- **CAQM Algorithm** (`ub-caqm.*`) - C-AQM congestion control algorithm implementation
- **Flow Control** (`ub-flow-control.*`) - Flow control framework module
- **Fault Injection Module** (`ub-fault.*`) - Fault injection, helping users inject packet loss rates, high latency, congestion levels, packet errors, flash disconnections, lane reduction and other faults during specific traffic processes

#### Data Types and Tools
- **Datatype** (`ub-datatype.*`) - UB data type definitions
- **Header** (`ub-header.*`) - UB protocol header definition and parsing
- **Network Address** (`ub-network-address.h`) - Network address related utility functions, including address translation, mask matching and other functionalities

### 2. Core Simulation Features

#### Topology and Traffic Support
- **Arbitrary Topology**: Supports simulation and modeling of arbitrary topologies, users can quickly build topologies and routing tables using UB toolsets
- **Arbitrary Traffic**: Supports configuration of arbitrary simulation traffic, users can quickly build collective communications and communication operator graphs using UB toolsets in conjunction with UbClientDag
- **Performance Monitoring**: Comprehensive performance metric collection and analysis

#### Protocol Stack Modeling
- **UB Protocol Stack**: Supports complete protocol stack modeling from physical layer to application layer
- **Memory Semantics**: Implements Load/Store-based memory semantic behavior modeling
- **Message Semantics**: Implements URMA-based message semantic behavior modeling
- **Native Multipath**: Implements native multipath support through TP/TP Group protocol mechanisms

#### Protocol Algorithm Support
- **Flow Control**: Implements credit-based flow control mechanism framework, compatible with PFC
- **Congestion Control**: Implements network-side marking, receiver response, sender response framework commonly used in congestion control algorithms, supports C-AQM algorithm
- **Routing Strategies**: Supports shortest path routing, bypass strategies, supports packet spraying, ECMP and other load balancing strategies
- **QoS Support**: Provides end-to-end QoS support, currently supports SP strategy
- **Switch Arbitration**: Modular implementation of UbSwitch switching arbitration mechanism modeling, currently supports priority-based SP scheduling

### 3. Script Toolset

Provides complete network simulation workflow support:

- **Network Topology Generation**: Automatically generates various network topologies (Clos, 2D Fullmesh, etc.)
- **Traffic Pattern Generation**: Supports All-Reduce, All-to-All, All-to-All-V and other communication patterns, supports RHD, NHR, OneShot multiple collective communication algorithms
- **Performance Analysis Tools**: Throughput calculation, latency analysis, CDF plotting
- **Formatted Result Output**: Automatically generates basic result information tables for flow completion time, bandwidth, etc., with optional generation of packet-level hop-by-hop information within the network

## License

This project follows the ns-3 license agreement, GPL v2. See the `LICENSE` file for details.

## Citation
```bibtex
@software{UBNetworkSimulator,
  month = {10},
  title = {{ns-3-UB: Unified-Bus Network Simulation Framework}},
  url = {https://gitcode.com/open-usim/ns-3-ub},
  version = {1.0.0},
  year = {2025}
}
```