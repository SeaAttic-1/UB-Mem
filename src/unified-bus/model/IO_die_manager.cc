#include "IO_die_manager.h"

namespace ns3 {
    TypeId IO_Die_Manager::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::IO_Die_Manager")
            .SetParent<Object> ()
            .SetGroupName("UnifiedBus")
            .AddConstructor<IO_Die_Manager>()
            .AddAttribute("IODieCount",
                        "Number of IO Dies",
                        UintegerValue(IO_DIE_PER_NODE),
                        MakeUintegerAccessor(&IO_Die_Manager::m_die_count),
                        MakeUintegerChecker<uint32_t>());
        return tid;
    }

    IO_Die_Manager::IO_Die_Manager(void) : m_dies(0) {}

    IO_Die_Manager::~IO_Die_Manager(void) {}
    
    void IO_Die_Manager::DoInitialize(void) {
        this->m_dies.reserve(m_die_count);
        for(uint32_t i = 0; i < m_die_count; i++)
            m_dies.push_back(CreateObject<UbSwitch>());
        for(uint32_t i = 0; i < m_die_count; i++)
            m_dies[i]->SetIODieId(i);
    }

    void IO_Die_Manager::SetNodeType(UbNodeType_t nodeType) {
        for(uint32_t i = 0; i < m_die_count; i++)
            this->m_dies[i]->SetNodeType(nodeType);
    }

    void IO_Die_Manager::SetNodeId(void) {
        uint32_t nodeId = GetObject<Node>()->GetId();
        m_nodeId = nodeId;
        for(uint32_t i = 0; i < m_die_count; i++)
            m_dies[i]->SetNodeId(nodeId);
    }

    void IO_Die_Manager::SetNodeId(uint32_t nodeId) {
        m_nodeId = nodeId;
    }

    uint32_t IO_Die_Manager::GetNodeId(void) {
        return m_nodeId;
    }

    void IO_Die_Manager::SetForwardDelay(string forwardDelay) {
        if (!forwardDelay.empty())
            for(uint32_t i = 0; i < m_die_count; i++)
                m_dies[i]->GetAllocator()->SetAttribute("AllocationTime", StringValue(forwardDelay));
    }

    void IO_Die_Manager::Init(void) {
        for(uint32_t i = 0; i < m_die_count; i++)
            m_dies[i]->Init();
    }

    void IO_Die_Manager::CongestionControlInit(void) {
        for (uint32_t i = 0; i < m_die_count; i++)
        {
            auto cc = UbCongestionControl::Create(UB_SWITCH);
            cc->SwitchInit(m_dies[i], m_nodeId, i);
        }
    }

    void IO_Die_Manager::SetIODieCount(uint32_t io_die_count) {
        m_die_count = io_die_count;
    }

    uint32_t IO_Die_Manager::GetIODieCount(void) {
        return m_die_count;
    }

    void IO_Die_Manager::SetPortCountPerIODie(uint32_t port_count_per_io_die) {
        m_port_count_per_die = port_count_per_io_die;
    }
    uint32_t IO_Die_Manager::GetPortCountPerIODie(void) {
        return m_port_count_per_die;
    }

    void IO_Die_Manager::StartCongestionControl(void) {
        for(uint32_t i = 0; i < m_die_count; i++) {
            Ptr<UbCongestionControl> congestionCtrl = m_dies[i]->GetCongestionCtrl();
            if (congestionCtrl->GetCongestionAlgo() == CAQM) {
                Ptr<UbSwitchCaqm> swCaqm = DynamicCast<UbSwitchCaqm>(congestionCtrl);
                swCaqm->ResetLocalCc();
            }
        }
    }

    IO_DIE_INFO IO_Die_Manager::GetIODie(void) {
        // Very simple scheduling logic here
        // Can add more advanced scheduling logic depending on needs

        if (m_die_count == 1) return {m_dies[0], 0};
        IO_DIE_INFO ret = {m_dies[m_die_salt], m_die_salt++};
        m_die_salt = m_die_salt % m_die_count;
        return ret;
    }

    Ptr<UbSwitch> IO_Die_Manager::GetIODieById(uint32_t io_die_id){
        if (io_die_id >= m_die_count) return nullptr;
        return m_dies[io_die_id];
    }

}

