#ifndef IO_DIE_MANAGER_H
#define IO_DIE_MANAGER_H

#define IO_DIE_PER_NODE 2

#include <vector>
#include <queue>
#include "ns3/ub-routing-process.h"
#include "ns3/ub-queue-manager.h"
#include "ns3/node.h"
#include "ns3/traced-callback.h"
#include "ns3/ub-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/ub-switch.h"
#include "ns3/ub-congestion-control.h"
#include "ns3/ub-switch-allocator.h"
#include "protocol/ub-caqm.h"

namespace ns3 {

    class UbCongestionControl;
    class UbSwitch;
    class UbSwitchCaqm;
    class UbPort;
    class UbSwitchAllocator;

    typedef struct {
        Ptr<UbSwitch> io_die_ptr;
        uint32_t io_die_id;
    } IO_DIE_INFO;

    class IO_Die_Manager : public Object {
        public:
            static TypeId GetTypeId(void);
            IO_Die_Manager(void);
            ~IO_Die_Manager(void);
            void DoInitialize(void);
            IO_DIE_INFO GetIODie(void);
            Ptr<UbSwitch> GetIODieById(uint32_t io_die_id);
            void SetNodeType(UbNodeType_t nodeType);
            void SetNodeId(void); // Called after aggregating the manager onto a node
            void SetNodeId(uint32_t nodeId);
            uint32_t GetNodeId(void);
            void SetIODieCount(uint32_t io_die_count);
            uint32_t GetIODieCount(void);
            void SetForwardDelay(string forwardDelay);
            void Init(void);
            void CongestionControlInit(void);
            void StartCongestionControl(void);

        private:
            std::vector<Ptr<UbSwitch>> m_dies;
            uint32_t m_die_count = IO_DIE_PER_NODE;
            uint32_t m_die_salt = 0;
            uint32_t m_nodeId;
            
    };
}
#endif