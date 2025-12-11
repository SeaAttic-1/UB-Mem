// SPDX-License-Identifier: GPL-2.0-only
#include "ns3/ub-switch-allocator.h"
#include "ns3/ub-switch.h"
#include "ns3/ub-port.h"
#include "protocol/ub-routing-process.h"
#include "ub-queue-manager.h"
#include "ns3/IO_die_manager.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(UbSwitchAllocator);
NS_LOG_COMPONENT_DEFINE("UbSwitchAllocator");

TypeId UbSwitchAllocator::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UbSwitchAllocator")
        .SetParent<Object>()
        .AddConstructor<UbSwitchAllocator>()
        .AddAttribute("AllocationTime",
                      "Time of Allocation Used.",
                      TimeValue(NanoSeconds(10)),
                      MakeTimeAccessor(&UbSwitchAllocator::m_allocationTime),
                      MakeTimeChecker());
    return tid;
}
UbSwitchAllocator::UbSwitchAllocator()
{
}

UbSwitchAllocator::~UbSwitchAllocator()
{
}

void UbSwitchAllocator::DoDispose()
{
    m_igsrc.clear();
}

void UbSwitchAllocator::TriggerAllocator(Ptr<UbPort> outPort)
{
}

void UbSwitchAllocator::Init()
{
}
void UbSwitchAllocator::RegisterUbIngressQueue(Ptr<UbIngressQueue> ingressQueue, uint32_t outPort, uint32_t priority)
{
    uint32_t portsNum = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    m_igsrc[outPort % portsNum][priority].push_back(ingressQueue);
}

void UbSwitchAllocator::RegisterEgressStauts(uint32_t portsNum)
{
    m_egStatus.resize(portsNum, true);
}

void UbSwitchAllocator::SetEgressStatus(uint32_t portId, bool status)
{
    m_egStatus[portId] = status;
}

bool UbSwitchAllocator::GetEgressStatus(uint32_t portId)
{
    return m_egStatus[portId];
}

/*-----------------------------------------UbRoundRobinAllocator----------------------------------------------*/
TypeId UbRoundRobinAllocator::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UbRoundRobinAllocator")
        .SetParent<UbSwitchAllocator>()
        .AddConstructor<UbRoundRobinAllocator>();
    return tid;
}

void UbRoundRobinAllocator::Init()
{
    std::cout << "test \n";
    auto node = NodeList::GetNode(m_nodeId);
    // Modified:
    // uint32_t portsNum = node->GetNDevices();
    // auto vlNum = node->GetObject<UbSwitch>()->GetVLNum();
    uint32_t portsNum = node->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    std::cout << m_io_die_id << " test 2\n";
    auto vlNum = node->GetObject<IO_Die_Manager>()->GetIODieById(m_io_die_id)->GetVLNum();
    std::cout << "test 3\n";
    m_rrIdx.resize(portsNum);
    for (auto &v: m_rrIdx) {
        v.resize(vlNum, 0);
    }
    m_igsrc.resize(portsNum);
    m_isRunning.resize(portsNum, false);
    m_oneMoreRound.resize(portsNum, false);
    for (auto &i : m_igsrc) {
        i.resize(vlNum);
    }
}

void UbRoundRobinAllocator::TriggerAllocator(Ptr<UbPort> outPort)
{
    NS_LOG_DEBUG("[UbRoundRobinAllocator TriggerAllocator] portId: " << outPort->GetIfIndex());
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    auto outPortId = outPort->GetIfIndex() % port_count_per_io_die;
    if (m_isRunning[outPortId]) {
        // one more round flag
        // 为了避免running过程中新生成的包：
        // 1.无法被当前轮次调度
        // 2.下一次trigger会被当前轮次的状态掩盖
        m_oneMoreRound[outPortId] = true;
        NS_LOG_DEBUG("[UbRoundRobinAllocator TriggerAllocator] Allocator is running, will retrigger.");
        return;
    }
    m_isRunning[outPortId] = true;
    Simulator::Schedule(m_allocationTime, &UbRoundRobinAllocator::AllocateNextPacket, this, outPort);
}

void UbRoundRobinAllocator::AllocateNextPacket(Ptr<UbPort> outPort)
{
    // 轮询调度
    NS_LOG_DEBUG("[UbRoundRobinAllocator AllocateNextPacket] portId: " << outPort->GetIfIndex());
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    auto outPortId = outPort->GetIfIndex() % port_count_per_io_die;
    auto ingressQueue = SelectNextIngressQueue(outPort);
    // 调度得到的ingressqueue加入egressqueue
    if (ingressQueue != nullptr) {
        auto packet = ingressQueue->GetNextPacket();
        auto inPortId = ingressQueue->GetInPortId();
        auto priority = ingressQueue->GetIgqPriority();
        auto packetEntry = std::make_tuple(inPortId, priority, packet);
        outPort->GetFlowControl()->HandleSentPacket(packet, ingressQueue);
        outPort->GetUbQueue()->DoEnqueue(packetEntry);
    }
    m_isRunning[outPortId] = false;
    // 通知port发包
    Simulator::ScheduleNow(&UbPort::NotifyAllocationFinish, outPort);
    if (m_oneMoreRound[outPortId] == true) {
        m_oneMoreRound[outPortId] = false;
        Simulator::ScheduleNow(&UbRoundRobinAllocator::TriggerAllocator, this, outPort);
        NS_LOG_DEBUG("[UbRoundRobinAllocator AllocateNextPacket] ReTriggerAllocator portId: " << outPort->GetIfIndex());
        return;
    }
}

Ptr<UbIngressQueue> UbRoundRobinAllocator::SelectNextIngressQueue(Ptr<UbPort> outPort)
{
    uint32_t idx;
    uint32_t pi;
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    uint32_t outPortId = outPort->GetIfIndex();
    auto node = NodeList::GetNode(m_nodeId);

    // Modified:
    // auto vlNum = node->GetObject<UbSwitch>()->GetVLNum();
    auto vlNum = node->GetObject<IO_Die_Manager>()->GetIODieById(m_io_die_id)->GetVLNum();

    for (pi = 0 ; pi < vlNum; pi++) {
        size_t qSize = m_igsrc[outPortId % port_count_per_io_die][pi].size();
        for (idx = 0; idx < qSize; idx++) {
            auto qidx = (idx + m_rrIdx[outPortId % port_count_per_io_die][pi]) % qSize;
            if (!m_igsrc[outPortId % port_count_per_io_die][pi][qidx]->IsEmpty() &&
                !outPort->GetFlowControl()->IsFcLimited(m_igsrc[outPortId % port_count_per_io_die][pi][qidx])) {
                m_rrIdx[outPortId % port_count_per_io_die][pi] = (qidx + 1) % qSize;
                NS_LOG_DEBUG("[UbSwitchAllocator DispatchPacket] " << " NodeId: " << node->GetId()
                << " PortId: " << outPortId <<" qidx: "<< qidx);
                return m_igsrc[outPortId % port_count_per_io_die][pi][qidx];
            }
        }
    }
    return nullptr;
}

} // namespae ns3
