// SPDX-License-Identifier: GPL-2.0-only
#include <iostream>
#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/flow-id-tag.h"
#include "ns3/ub-switch-allocator.h"
#include "ns3/ub-caqm.h"
#include "ns3/ub-port.h"
#include "ns3/ub-switch.h"

// Modified:
// Other files may also contain this additional header
#include "ns3/IO_die_manager.h"

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED(UbSwitch);
NS_LOG_COMPONENT_DEFINE("UbSwitch");


/*-----------------------------------------UbSwitchNode----------------------------------------------*/

TypeId UbSwitch::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::UbSwitch")
        .SetParent<Object> ()
        .SetGroupName("UnifiedBus")
        .AddConstructor<UbSwitch> ()
        .AddAttribute("EnableCBFC",
                      "Enable CBFC.",
                      BooleanValue(false),
                      MakeBooleanAccessor(&UbSwitch::m_isCBFCEnable),
                      MakeBooleanChecker())
        .AddAttribute("EnablePFC",
                      "Enable PFC.",
                      BooleanValue(false),
                      MakeBooleanAccessor(&UbSwitch::m_isPFCEnable),
                      MakeBooleanChecker())
        .AddTraceSource("LastPacketTraversesNotify",
                        "Last Packet Traverses, NodeId",
                        MakeTraceSourceAccessor(&UbSwitch::m_traceLastPacketTraversesNotify),
                        "ns3::UbSwitch::LastPacketTraversesNotify");
    return tid;
}
/**
 * @brief Init UbNode, create algorithm, queueManager, fc and so on
 */
void UbSwitch::Init()
{
    // auto node = GetObject<Node>();
    // m_portsNum = node->GetNDevices();
    auto node = NodeList::GetNode(m_nodeId);
    
    // Modified GetObject<Node>() -> NodeList::GetNode(m_nodeId);
    m_portsNum = node->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    // alg init
    std::cout << "Test\n";
    m_allocator = CreateObject<UbRoundRobinAllocator>();
    m_allocator->SetNodeId(node->GetId());
    std::cout << "m_io_die_id: " << m_io_die_id << "\n";
    m_allocator->SetIODieId(m_io_die_id);
    std::cout << "Test 2\n";
    m_allocator->Init();
    std::cout << "Test 3\n";
    VoqInit();
    std::cout << "Test 4\n";
    AddVoqIntoAlgroithm();
    std::cout << "Test 5\n";

    // queueManager init
    m_queueManager = CreateObject<UbQueueManager>();
    m_queueManager->SetVLNum(m_vlNum);
    m_queueManager->SetPortsNum(m_portsNum);
    m_queueManager->Init();

    std::cout << "Test 6\n";

    NodePortsFcInit();
    std::cout << "Test 7\n";
    m_routingProcess = CreateObject<UbRoutingProcess>();
    m_Ipv4Addr = utils::NodeIdToIp(node->GetId());
}

void UbSwitch::DoDispose()
{
    m_queueManager = nullptr;
    m_congestionCtrl = nullptr;
    m_allocator = nullptr;
    m_voq.clear();
    m_routingProcess = nullptr;
}

/**
 * @brief Init flow control for each port
 */
void UbSwitch::NodePortsFcInit()
{
    NS_LOG_DEBUG("[UbSwitch NodePortsFcInit] m_portsNum: " << m_portsNum << " m_isCBFCEnable: " << m_isCBFCEnable
                << " m_isPFCEnable: " << m_isPFCEnable);

    for (uint32_t pidx = m_io_die_id * m_portsNum; pidx < (m_io_die_id + 1) * m_portsNum; pidx++) {
        std::cout << "pidx: " << pidx << '\n';
        Ptr<UbPort> port = DynamicCast<ns3::UbPort>(NodeList::GetNode(m_nodeId)->GetDevice(pidx));
        if (m_isCBFCEnable) {
            port->CreateAndInitFc("CBFC");
        } else if (m_isPFCEnable) {
            port->CreateAndInitFc("PFC");
        } else {
            port->CreateAndInitFc("UBFC");
        }
    }
}

/**
 * @brief 将初始化后的vop放入调度算法中
 */
void UbSwitch::AddVoqIntoAlgroithm()
{
    for (uint32_t i = 0; i < m_portsNum; i++) {
        for (uint32_t j = 0; j < m_vlNum; j++) {
            for (uint32_t k = 0 ; k < m_portsNum; k++) { // voq
                auto igq = m_voq[i][j][k];
                m_allocator->RegisterUbIngressQueue(igq, i, j);
            }
        }
    }
}

/**
 * @brief 将tp放入调度算法中
 */
void UbSwitch::AddTpIntoAlgroithm(Ptr<UbIngressQueue> tp, uint32_t outPort, uint32_t priority)
{
    if ((outPort < m_io_die_id * m_portsNum) || (outPort >= (m_io_die_id + 1) * m_portsNum) || (priority >= m_vlNum)) {
        NS_ASSERT_MSG(0, "Invalid indices (outPort, priority)!" << outPort << " " << priority << " " << m_portsNum << " " << m_vlNum << " " << m_io_die_id << " " << m_portsNum);
    }
    NS_LOG_DEBUG("[UbSwitch AddTpIntoAlgroithm] TP: outPortIdx: " << outPort
                 << "priorityIdx: " << priority << "outPort: " << outPort);
    tp->SetOutPortId(outPort);
    tp->SetInPortId(outPort); // tp不使用inport
    tp->SetIgqPriority(priority);
    m_allocator->RegisterUbIngressQueue(tp, outPort, priority);
}

UbSwitch::UbSwitch()
{
}
UbSwitch::~UbSwitch()
{
}

Ptr<UbSwitchAllocator> UbSwitch::GetAllocator()
{
    return m_allocator;
}

/**
 * @brief init voq
 */
void UbSwitch::VoqInit()
{
    uint32_t outPortIdx = 0;
    uint32_t priorityIdx = 0;
    uint32_t inPortIdx = 0;
    m_voq.resize(m_portsNum);
    for (auto &i : m_voq) {
        priorityIdx = 0;
        i.resize(m_vlNum);
        for (auto &j : i) {
            inPortIdx = 0;
            for (uint32_t k = 0; k < m_portsNum; k++) {
                auto q = CreateObject<UbPacketQueue>();
                q->SetOutPortId(outPortIdx);
                q->SetIgqPriority(priorityIdx);
                q->SetInPortId(inPortIdx); // tp不使用inport
                q->SetInPortId(k);
                j.push_back(q);
                inPortIdx++;
            }
            priorityIdx++;
        }
        outPortIdx++;
    }
}

/**
 * @brief push packet into voq
 */
void UbSwitch::AddPktToVoq(Ptr<Packet> p, uint32_t outPort, uint32_t priority, uint32_t inPort)
{
    NS_LOG_INFO("outPort " << outPort << " prioriy " << priority << " inPort " << inPort);
    NS_LOG_INFO("m_io_die_id " << m_io_die_id << " m_portsNum " << m_portsNum );

    // Modified:
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();

    if ((outPort % port_count_per_io_die > m_portsNum) || (priority > m_vlNum) || (inPort % port_count_per_io_die > m_portsNum)) { // 不合理请求
        NS_ASSERT_MSG(0, "Invalid VOQ indices (outPort, priority, inPort)!");
    }
    m_voq[outPort % port_count_per_io_die][priority][inPort % port_count_per_io_die]->Push(p);
}

UbPacketType_t UbSwitch::GetPacketType(Ptr<Packet> packet)
{
    UbDatalinkHeader dlHeader;
    packet->PeekHeader(dlHeader);
    if (dlHeader.IsControlCreditHeader())
        return UB_CONTROL_FRAME;
    if (dlHeader.IsPacketIpv4Header())
        return UB_URMA_DATA_PACKET;
    if (dlHeader.IsPacketUbMemHeader())
        return UB_LDST_DATA_PACKET;
    return UNKOWN_TYPE;
}

/**
 * @brief Receive packet from port. Node handle packet
 */
void UbSwitch::SwitchHandlePacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    NS_LOG_INFO("Called SwitchHandlePacket");
    // 帧类型判断
    auto packetType = GetPacketType(packet);
    switch (packetType) {
        case UB_CONTROL_FRAME:
            port->m_flowControl->HandleReceivedControlPacket(packet);
            break;
        case UB_URMA_DATA_PACKET:
            ParseURMAPacketHeader(packet);
            HandleURMADataPacket(port, packet);
            break;
        case UB_LDST_DATA_PACKET:
            ParseLdstPacketHeader(packet);
            HandleLdstDataPacket(port, packet);
            break;
        default:
            NS_ASSERT_MSG(0, "Invalid Packet Type!");
    }
    return;
}

/**
 * @brief Sink control frame
 */
void UbSwitch::SinkControlFrame(Ptr<UbPort> port, Ptr<Packet> packet)
{
    if (IsCBFCEnable()) {
        auto flowControl = DynamicCast<UbCbfc>(port->GetFlowControl());
        flowControl->CbfcRestoreCrd(packet);
    } else if (IsPFCEnable()) {
        auto flowControl = DynamicCast<UbPfc>(port->GetFlowControl());
        flowControl->UpdatePfcStatus(packet);
    }

    return;
}

/**
 * @brief Handle URMA type data packet
 */
void UbSwitch::HandleURMADataPacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    switch (GetNodeType()) {
        case UB_DEVICE:
            if (!SinkTpDataPacket(port, packet)) {
                ForwardDataPacket(port, packet);
            }
            break;
        case UB_SWITCH:
            ForwardDataPacket(port, packet);
            break;
        default:
            NS_ASSERT_MSG(0, "Invalid Node! ");
    }
}

/**
 * @brief Handle Mem type data packet
 */
void UbSwitch::HandleLdstDataPacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    NS_LOG_INFO("Call HandleLdstDataPacket");
    switch (GetNodeType()) {
        case UB_DEVICE:
            if (!SinkMemDataPacket(port, packet)) {
                ForwardDataPacket(port, packet);
            }
            break;
        case UB_SWITCH:
            NS_LOG_INFO("Switch Forwarding Packet");
            ForwardDataPacket(port, packet);
            break;
        default:
            NS_ASSERT_MSG(0, "Invalid Node! ");
    }
    NS_LOG_INFO("HandleLdstDataPacket returns");
}

/**
 * @brief Sink URMA type data packet
 */
bool UbSwitch::SinkTpDataPacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    NS_LOG_DEBUG("[UbPort recv] Psn: " << m_ubTpHeader.GetPsn() << " port: " << port);
    Ipv4Mask mask("255.255.255.0");

    // Forward
    if (!utils::IsInSameSubnet(m_ipv4Header.GetDestination(), GetNodIpv4Addr(), mask)) {
        return false;
    }
    // Sink
    NS_LOG_DEBUG("[UbPort recv] Pkt tb is local");
    if (IsCBFCEnable()) {
        NS_LOG_INFO("CBFC Enabled, Call HandleReceivedPacket");
        port->m_flowControl->HandleReceivedPacket(packet);
    }

    NS_LOG_INFO("Test");

    uint32_t dstTpn = m_ubTpHeader.GetDestTpn();
    NS_LOG_INFO("Test 2");
    // Modified:
    //auto targetTp = GetObject<UbController>()->GetTpByTpn(dstTpn);
    
    auto targetTp = NodeList::GetNode(m_nodeId)->GetObject<UbController>()->GetTpByTpn(dstTpn);
    
    NS_LOG_INFO("Test 3");
    NS_ASSERT_MSG(targetTp != nullptr, "Port Cannot Get TP By Tpn!");
    NS_LOG_INFO("Test 3");
    if (m_ubTpHeader.GetTPOpcode() == static_cast<uint8_t>(TpOpcode::TP_OPCODE_ACK_WITH_CETPH)
        || m_ubTpHeader.GetTPOpcode() == static_cast<uint8_t>(TpOpcode::TP_OPCODE_ACK_WITHOUT_CETPH)) {
        NS_LOG_DEBUG("[UbPort recv] is ACK");
        packet->RemoveHeader(m_datalinkHeader);
        packet->RemoveHeader(m_networkHeader);
        packet->RemoveHeader(m_ipv4Header);
        packet->RemoveHeader(m_udpHeader);
        targetTp->RecvTpAck(packet);
    } else {
        NS_LOG_INFO("Test 3");
        targetTp->RecvDataPacket(packet);
    }
    NS_LOG_INFO("SinkTpDataPacket Returned");
    return true;
    
}

/**
 * @brief Sink Mem type data packet
 */
bool UbSwitch::SinkMemDataPacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    // Store/load request: DLH cNTH cTAH(0x03/0x06) [cMAETAH] Payload
    // Store/load response: DLH cNTH cATAH(0x11/0x12) Payload
    NS_LOG_DEBUG("[UbPort recv] ub mem frame");
    uint16_t dCna = m_memHeader.GetDcna();
    uint32_t dnode = utils::Cna16ToNodeId(dCna);
    // Forward
    if (dnode != NodeList::GetNode(m_nodeId)->GetId()) {
        return false;
    }
    // Sink Packet
    if (IsCBFCEnable()) {
        port->m_flowControl->HandleReceivedPacket(packet);
    }

    auto ldstApi = NodeList::GetNode(m_nodeId)->GetObject<UbController>()->GetUbFunction()->GetUbLdstApi();
    NS_ASSERT_MSG(ldstApi != nullptr, "UbLdstApi can not be nullptr!");

    uint8_t type = m_dummyTaHeader.GetTaOpcode();
    // 数据包
    if (type == (uint8_t)TaOpcode::TA_OPCODE_WRITE || type == (uint8_t)TaOpcode::TA_OPCODE_READ) {
        ldstApi->RecvDataPacket(packet);
    } else if (type == (uint8_t)TaOpcode::TA_OPCODE_TRANSACTION_ACK ||
               type == (uint8_t)TaOpcode::TA_OPCODE_READ_RESPONSE) {
        ldstApi->RecvResponse(packet);
        NS_LOG_DEBUG("mem packet is ack!");
    } else {
        NS_ASSERT_MSG(0, "packet Ta Op code is wrong!");
    }
    return true;
}

/**
 * @brief Parse packet header and forward different type data packet
 */
void UbSwitch::ForwardDataPacket(Ptr<UbPort> port, Ptr<Packet> packet)
{
    /* 解析包头 */
    NS_LOG_INFO("Call ForwardDataPacket");
    UbDatalinkPacketHeader m_datalinkHeader;
    packet->PeekHeader(m_datalinkHeader);
    int outPort = -1;
    RoutingKey rtKey;
    switch (GetPacketType(packet)) {
        case UB_URMA_DATA_PACKET:
            LastPacketTraversesNotify(NodeList::GetNode(m_nodeId)->GetId(), m_ubTpHeader);
            GetURMARoutingKey(packet, rtKey);
            break;
        case UB_LDST_DATA_PACKET:  {
            GetLdstRoutingKey(packet, rtKey);
            break;
        }
        default:
            NS_ASSERT_MSG(0, "Invalid Packet Type! ");
    }
    /* 路由 */
    outPort = m_routingProcess->GetOutPort(rtKey, port->GetIfIndex());
    NS_LOG_INFO("Select outPort = " << outPort);
    if (outPort < 0) {
        // Route failed
        NS_LOG_WARN("The route cannot be found. Packet Dropped!");
        return;
    }
    /* 绕路路由防止形成环 */
    if (!m_routingProcess->GetSelectShortestPath()) {
        ChangePakcetRoutingPolicy(packet, true);
    }
    /* 缓存管理 */
    uint32_t inPort = port->GetIfIndex();
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    uint8_t priority = m_datalinkHeader.GetPacketVL();
    if (!m_queueManager->CheckIngress(inPort % port_count_per_io_die, priority, packet->GetSize())) {
        // Drop
        NS_LOG_WARN("Ingress memory not enough. Packet Dropped!");
        return;
    }
    SendPacket(packet, inPort, outPort, priority);
    NS_LOG_INFO("ForwardDataPacket Return");
}

void UbSwitch::ChangePakcetRoutingPolicy(Ptr<Packet> packet, bool useShortestPath)
{
    UbDatalinkPacketHeader tempHeader;
    m_datalinkHeader.SetRoutingPolicy(useShortestPath);
    packet->RemoveHeader(tempHeader);
    packet->AddHeader(m_datalinkHeader);
}

void UbSwitch::ParseURMAPacketHeader(Ptr<Packet> packet)
{
    // 解析network包头
    // 解析ipv4包头
    // 解析udp包头
    // 解析tp包头
    packet->RemoveHeader(m_datalinkHeader);
    packet->RemoveHeader(m_networkHeader);
    packet->RemoveHeader(m_ipv4Header);
    packet->RemoveHeader(m_udpHeader);
    packet->PeekHeader(m_ubTpHeader);
    packet->AddHeader(m_udpHeader);
    packet->AddHeader(m_ipv4Header);
    packet->AddHeader(m_networkHeader);
    packet->AddHeader(m_datalinkHeader);
}

void UbSwitch::ParseLdstPacketHeader(Ptr<Packet> packet)
{
    packet->RemoveHeader(m_datalinkHeader);
    packet->RemoveHeader(m_memHeader);
    packet->PeekHeader(m_dummyTaHeader);
    packet->AddHeader(m_memHeader);
    packet->AddHeader(m_datalinkHeader);
}

void UbSwitch::GetURMARoutingKey(Ptr<Packet> packet, RoutingKey &rtKey)
{
    rtKey.sip = m_ipv4Header.GetSource().Get();
    rtKey.dip = m_ipv4Header.GetDestination().Get();
    rtKey.sport = m_udpHeader.GetSourcePort();
    rtKey.dport = m_udpHeader.GetDestinationPort();
    rtKey.priority = m_datalinkHeader.GetPacketVL();
    rtKey.useShortestPath = m_datalinkHeader.GetRoutingPolicy();
    rtKey.usePacketSpray = m_datalinkHeader.GetLoadBalanceMode();
}

void UbSwitch::GetLdstRoutingKey(Ptr<Packet> packet, RoutingKey &rtKey)
{
    uint16_t dCna = m_memHeader.GetDcna();
    uint16_t sCna = m_memHeader.GetScna();
    uint32_t snode = utils::Cna16ToNodeId(sCna);
    uint32_t dnode = utils::Cna16ToNodeId(dCna);
    uint16_t sport = utils::Cna16ToPortId(sCna);
    uint16_t dport = 0;
    uint16_t lb = m_memHeader.GetLb();
    rtKey.sip = utils::NodeIdToIp(snode, sport).Get();
    rtKey.dip = utils::NodeIdToIp(dnode, dport).Get();
    rtKey.sport = lb;
    rtKey.dport = dport;
    rtKey.priority = m_datalinkHeader.GetPacketVL();
    rtKey.useShortestPath = m_datalinkHeader.GetRoutingPolicy();
    rtKey.usePacketSpray = m_datalinkHeader.GetLoadBalanceMode();
}

/**
 * @brief Trigger port to send packet to nexthop
 */
void UbSwitch::SendPacket(Ptr<Packet> packet, uint32_t inPort, uint32_t outPort, uint32_t priority)
{
    NS_LOG_INFO("Sending out node " << m_nodeId <<", io die " << m_io_die_id << " inPort: " << inPort << " outPort: " << outPort);
    auto node = NodeList::GetNode(m_nodeId);
    Ptr<UbPort> recvPort = DynamicCast<ns3::UbPort>(node->GetDevice(inPort));
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    m_voq[outPort % port_count_per_io_die][priority][inPort % port_count_per_io_die]->Push(packet);
    NS_LOG_INFO("Chkpt 1");
    m_queueManager->PushIngress(inPort % port_count_per_io_die, priority, packet->GetSize());
    NS_LOG_INFO("Chkpt 2");
    if (IsPFCEnable()) {
        recvPort->m_flowControl->HandleReceivedPacket(packet);
    }
    m_queueManager->PushEgress(outPort % port_count_per_io_die, priority, packet->GetSize());
    NS_LOG_INFO("Chkpt 3");
    Ptr<UbPort> sendPort = DynamicCast<ns3::UbPort>(node->GetDevice(outPort));
    sendPort->TriggerTransmit();
}

/**
 * @brief Packet dequeue from ingress and push into egress.
 * This method is called by Port: m_node->NotifySwitchDequeue(inPortId, outPort, priority, packet)
 */
void UbSwitch::NotifySwitchDequeue(uint16_t inPortId, uint32_t outPort, uint32_t priority, Ptr<Packet> packet)
{
    NS_LOG_INFO("IO Die Id is " << m_io_die_id);
    uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
    UbDatalinkHeader dlHeader;
    packet->PeekHeader(dlHeader);
    if (!dlHeader.IsControlCreditHeader()) {
        NS_LOG_DEBUG("[QMU] Node:" << NodeList::GetNode(m_nodeId)->GetId()
              << " port:" << outPort
              << " egress size:" << m_queueManager->GetAllEgressUsed(outPort % port_count_per_io_die));
        m_congestionCtrl->SwitchForwardPacket(inPortId, outPort, packet);
        m_queueManager->PopIngress(inPortId % port_count_per_io_die, priority, packet->GetSize());
    }
}

bool UbSwitch::IsCBFCEnable()
{
    return m_isCBFCEnable;
}

bool UbSwitch::IsPFCEnable()
{
    return m_isPFCEnable;
}

Ptr<UbQueueManager> UbSwitch::GetQueueManager()
{
    return m_queueManager;
}

void UbSwitch::SwitchSendFinish(uint32_t portId, uint32_t pri, Ptr<Packet> packet)
{
    NS_LOG_INFO("Call SwitchSendFinish");
    UbDatalinkHeader dlHeader;

    packet->PeekHeader(dlHeader);
    if (!dlHeader.IsControlCreditHeader()) {
        NS_LOG_INFO("Test");
        uint32_t port_count_per_io_die = NodeList::GetNode(m_nodeId)->GetObject<IO_Die_Manager>()->GetPortCountPerIODie();
        m_queueManager->PopEgress(portId % port_count_per_io_die, pri, packet->GetSize());
        NS_LOG_DEBUG("[queueManager] Node:" << NodeList::GetNode(m_nodeId)->GetId()
                  << " port:" << portId
                  << " egress size:" << m_queueManager->GetAllEgressUsed(portId % port_count_per_io_die));
    }
}

void UbSwitch::SetCongestionCtrl(Ptr<UbCongestionControl> congestionCtrl)
{
    m_congestionCtrl = congestionCtrl;
}

Ptr<UbCongestionControl> UbSwitch::GetCongestionCtrl()
{
    return m_congestionCtrl;
}

void UbSwitch::LastPacketTraversesNotify(uint32_t nodeId, UbTransportHeader ubTpHeader)
{
    m_traceLastPacketTraversesNotify(nodeId, ubTpHeader);
}

}  // namespace ns3
