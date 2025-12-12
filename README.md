# An adaption of UnifiedBus Simulator. (Branch Test)

This branch holds the code for modeling multiple IO dies on a single host. Each host can maintain a set of ports. This allows greater parallelisms.

## Implementation

The implementation revolves around the existing **UbSwitch** class. This class models a minimum switch and maintains its own set of ports and virtual output queues. The original UB code base has each node maintain exactly one instance of UbSwitch, although technically speaking the UB specification allows more than 1 UbSwitch per node. Adding more UbSwitch instances to each node serve as an efficient way of modeling multiple IO dides. 

## Changes

Changes are mainly about the handling of UbSwitch instances. Because the original UB code base uses exactly one UbSwitch instance per node, and now there can be multiple UbSwitch instances, care must be taken to choose the one that should be used. In the original UB code base, the UbSwitch object is directly aggregated onto the ns-3 node object using

`node->AggregateObject(CreateObject<UbSwitch>());`

Since `AggregateObject()` does not allow aggregating more than one instance of the same type, I defined a custom class **IO_Die_Manager** which is essentially a vector of UbSwitches, with some basic functionalities such as initialization and info registration/retrieval. Additionally private members called `m_io_die_id` are added to several classes as an identifier which IO die these class instances belong to.

### Ports

The original UB code base attaches ports directly to each node. This is fine when each node maintains only one UbSwitch instance (or equivalently, one IO die), but doing so makes it harder to identify which port belongs to which IO dies in the presence of multiple IO dies. Technically speaking, ports should be maintained by the UbSwitch class, not the nodes. However, to avoid too much refactoring I stuck with the old practice. Ports are still aggregated to the node only, and the following operation is used to determine the IO die a certain port belongs to:

Suppose one node has 8 IO dies and each IO die maintains 8 ports. Then this node has a total of 8 * 8 = 64 ports. Port 0...7 belongs to IO die 0, port 8...15 belongs to IO die 1, and so on. Note that for the sake of brevity, IO dies on the same node must maintain the same number of ports. IO dies on different nodes can have different number of ports.

Another thing to note is that the original UB code space uses `std::vector` to hold some internal information, including queue usages, etc. Some of these vectors are indexed using the port number. Because now there are multiple IO dies,  these vetors get duplicated as well. If no change is made, then Port 9, which is the second port of IO die 1, will index to the 9-th position of these vectors, which should have only 8 entries. To avoid this kind of index-out-of-range errors, a mod operation is used: the **IO_Die_Manager** instance on each node keeps track of `m_port_count_per_io_die`, i.e. how many ports one IO die maintain. Calling the function `IO_Die_Manager::GetPortCountPerIODie()` returns `m_port_count_per_io_die`. Modding the port ID by this number gives the correct index to use. Using the previously example, modding port number 9 by 8 gives 1, which is the correct index to use.

### IO Die Scheduling

The exact IO die scheduling mechanism is left for the user to define. Currently, the scheduling uses a basic round-robin logic, although preliminary experiments indicated that the basic round-robin logic is good enough to ensure a roughly equal IO die usage. If you want a more advanced scheduling algorithms monitoring each IO die's usage and making arbitrations based on that usage info, you may take advantage of the internal states maintained by each IO dies' **UbCongestionControl** class instances. Currently, Ub uses its custom **CAQM** CC algorithm for evaluating each transport-channel's usage, but you can also write your own CC. Consider adding a function to **UbCaqm** or your custom CC class that returns a score for each transport channel's usage, and then average these scores to get a final score. Use this final score as an indication of how severely loaded the IO die is. You can then modify `IO_Die_Manager::GetIODie()` and use the score info there for a more advanced scheduling logic.

### Per IO Die Send/Receive Queue

Because each IO Die is implemented using a UbSwitch instance, it inherently has the virtual output queue instance `m_voq`. Therefore, no additional queue is needed as of now. 

## Testing

### Test Files

Test files largely follow the same formats as those of the original UB simulator. There are only two differences concerning *node.csv*. 

- Addition of an extra column at the end: **IODieCount**. This specifies how many IO dies a node has. Omit it to use the default value **IO_DIE_PER_NODE** defined in `IO_Die_Manager.h`, which is set to 2.

- Change of meaning of the **portsNum** column. This specify how many ports a single IO die has. So the total number pof ports on a node can be calculated as **IODieCount \* portsNum**.

### Functional Correctness

It is expected that with **IODieCount** set to 1, the current simulator should behave exactly the same as the original UB simulator. Multiple tests (`2nodes_simple, 2nodes_single-tp, 2nodes_multiple-tp`) have been performed and this was indeed verified.

### New testcase

A new test case called `2nodes_multiple-io` has been provided. It features 2 nodes, each bearing 3 IO dies and each IO die bearing 3 ports. The two nodes are interconnected by 3 parallel switches. 

Interestingly, when compared with the setting where each node has only 1 IO die but 9 ports (so the total port number is the same), the total throughput lags by about less tha 10Gbps. It is possible this performance drop is caused by three IO die having their own CC instance running, and applying stricter control. 

### Remarks

More testings, if required, will be performed. You may also clone this repo and implement your custom logic. This branch will be merged with the main branch if required.