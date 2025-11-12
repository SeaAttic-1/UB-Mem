// SPDX-License-Identifier: GPL-2.0-only
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ub-datatype.h"
#include "ns3/ub-controller.h"
#include "ns3/ub-transaction.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UbTransaction");

NS_OBJECT_ENSURE_REGISTERED(UbTransaction);

TypeId UbTransaction::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UbTransaction")
        .SetParent<Object>()
        .SetGroupName("UnifiedBus");
    return tid;
}

UbTransaction::UbTransaction()
{
    NS_LOG_DEBUG("UbTransaction created");
}

UbTransaction::~UbTransaction()
{
}

UbTransaction::UbTransaction(Ptr<Node> node)
{
    m_nodeId = node->GetId();
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(1.0));
    m_pushWqeSegmentToTpCb = MakeCallback(&UbTransaction::OnScheduleWqeSegmentFinish, this);
}

void UbTransaction::TpInit(Ptr<UbTransportChannel> tp)
{
    m_tpnMap[tp->GetTpn()] = tp;
    m_tpRRIndex[tp->GetTpn()] = 0;
    m_tpSchedulingStatus[tp->GetTpn()] = false;
}


Ptr<UbFunction> UbTransaction::GetFunction()
{
    return NodeList::GetNode(m_nodeId)->GetObject<UbController>()->GetUbFunction();
}

Ptr<UbJetty> UbTransaction::GetJetty(uint32_t jettyNum)
{
    return GetFunction()->GetJetty(jettyNum);

}

bool UbTransaction::JettyBindTp(uint32_t src, uint32_t dest, uint32_t jettyNum,
                                bool multiPath, std::vector<uint32_t> tpns)
{
    NS_LOG_DEBUG(this);
    Ptr<UbJetty> ubJetty = GetJetty(jettyNum);
    if (ubJetty == nullptr) {
        return false;
    }

    std::vector<Ptr<UbTransportChannel>> ubTransportGroup;

    for (uint32_t i = 0; i < tpns.size(); i++) {
        uint32_t tpn = tpns[i];
        ubTransportGroup.push_back(m_tpnMap[tpn]);
        if (m_tpRelatedJetties.find(tpn) == m_tpRelatedJetties.end()) {
            m_tpRelatedJetties[tpn] = std::vector<Ptr<UbJetty>>();
        }
    }
    // 在事务层模式为ROL时只能开启单路径模式
    if (m_serviceMode == TransactionServiceMode::ROL) {
        NS_LOG_WARN("ROL, set to single path forced.");
        multiPath = false;
    }
    if (multiPath) {
        NS_LOG_DEBUG("Multiple tp");
        for (uint32_t i = 0; i < ubTransportGroup.size(); i++) {
            if (std::find(m_tpRelatedJetties[tpns[i]].begin(),
                          m_tpRelatedJetties[tpns[i]].end(),
                          ubJetty) == m_tpRelatedJetties[tpns[i]].end()) {
                m_tpRelatedJetties[tpns[i]].push_back(ubJetty);
            }
        }
    } else {
        NS_LOG_DEBUG("Single tp");
        double res = m_random->GetValue();
        int pos;
        if (res < 1.0) {
            pos = (int)(res * ubTransportGroup.size());
        } else {
            pos = ubTransportGroup.size() - 1;
        }
        if (std::find(m_tpRelatedJetties[tpns[pos]].begin(),
                      m_tpRelatedJetties[tpns[pos]].end(),
                      ubJetty) == m_tpRelatedJetties[tpns[pos]].end()) {
            m_tpRelatedJetties[tpns[pos]].push_back(ubJetty);
        }
    }

    m_jettyTpGroup[jettyNum] = ubTransportGroup;

    NS_LOG_DEBUG("jetty bind tp:");
    for (auto it = m_jettyTpGroup.begin(); it != m_jettyTpGroup.end(); it++) {
        NS_LOG_DEBUG("jettyNum:" << it->first);
        for (uint32_t i = 0; i < it->second.size(); i++) {
            NS_LOG_DEBUG("tpn:" << it->second[i]->GetTpn());
        }
    }

    NS_LOG_DEBUG("tp related jetty:");
    for (auto it = m_tpRelatedJetties.begin(); it != m_tpRelatedJetties.end(); it++) {
        NS_LOG_DEBUG("tpn: " << it->first);
        for (uint32_t i = 0; i < it->second.size(); i++) {
            NS_LOG_DEBUG("jettyNum:" << it->second[i]->GetJettyNum());
        }
    }
    return true;
}

void UbTransaction::DestroyJettyTpMap(uint32_t jettyNum)
{
    auto itJettyTp = m_jettyTpGroup.find(jettyNum);
    if (itJettyTp != m_jettyTpGroup.end()) {
        // 解除关系
        m_jettyTpGroup.erase(itJettyTp);
        NS_LOG_DEBUG("Destroyed jetty in m_jettyTpGroup");
    } else {
        NS_LOG_WARN("Jetty Tp map not found for destruction");
    }

    for (auto it = m_tpRelatedJetties.begin(); it != m_tpRelatedJetties.end(); it++) {
        for (size_t i = 0; i < it->second.size(); i++) {
            if (it->second[i]->GetJettyNum() == jettyNum) {
                it->second.erase(it->second.begin() + i);
            }
        }
    }
}

const std::vector<Ptr<UbTransportChannel>> UbTransaction::GetJettyRelatedTpVec(uint32_t jettyNum)
{
    NS_LOG_DEBUG(this);
    auto it = m_jettyTpGroup.find(jettyNum);
    if (it != m_jettyTpGroup.end()) {
        return it->second;
    }
    NS_LOG_DEBUG("UbTransportChannel vector not found");
    return {};
}

std::vector<Ptr<UbJetty>> UbTransaction::GetTpRelatedJettyVec(uint32_t tpn)
{
    NS_LOG_DEBUG(this);
    auto it = m_tpRelatedJetties.find(tpn);
    if (it != m_tpRelatedJetties.end()) {
        return it->second;
    }
    NS_LOG_DEBUG("UbJetty vector not found");
    return {};
}

void UbTransaction::TriggerScheduleWqeSegment(uint32_t jettyNum)
{
    // 遍历与该jetty绑定的tp，全部进行调度
    auto tpVec = GetJettyRelatedTpVec(jettyNum);
    if (!tpVec.empty()) {
        for (uint32_t i = 0; i < tpVec.size(); i++) {
            Simulator::ScheduleNow(&UbTransaction::ScheduleWqeSegment, this, tpVec[i]);
        }
    }
}

void UbTransaction::ApplyScheduleWqeSegment(Ptr<UbTransportChannel> tp)
{
    Simulator::ScheduleNow(&UbTransaction::ScheduleWqeSegment, this, tp);
}

void UbTransaction::ScheduleWqeSegment(Ptr<UbTransportChannel> tp)
{
    uint32_t tpn = tp->GetTpn();

    // 若当前TP正处于调度状态，则结束，否则继续进行，并将状态设置为true
    if (m_tpSchedulingStatus[tpn]) {
        return;
    } else {
        m_tpSchedulingStatus[tpn] = true;
    }
    // 找到tp相关的Jetty
    auto tpRelatedJetties = GetTpRelatedJettyVec(tpn);
    // 找到tp相关的remoteRequest
    std::map<uint32_t, std::vector<Ptr<UbWqeSegment>>> remoteRequestSegMap;
    // 找到tp相关的remoteRequest
    if (m_tpRelatedRemoteRequests.find(tpn) != m_tpRelatedRemoteRequests.end()) {
        remoteRequestSegMap = m_tpRelatedRemoteRequests[tpn];
    }

    // 记录开始轮询的位置， 避免无限循环
    uint32_t jettyCount = tpRelatedJetties.size();
    uint32_t rrCount = jettyCount + remoteRequestSegMap.size();

    if (rrCount == 0) {
        // 该TP无对应jetty，不进行调度，状态重置
        m_tpSchedulingStatus[tpn] = false;
        return;
    }
    // 检查当前TP是否队列满，以及待发送的wqesegment队列长度是否小于2
    if (tp->IsWqeSegmentLimited() || tp->GetWqeSegmentVecSize() > 1) {
        tp->SetTpFullStatus(true);
        NS_LOG_DEBUG("Full TP");
        // 满队列或满segment
        m_tpSchedulingStatus[tpn] = false;
        return;
    }

    // m_tpRRIndex每次更新时都会进行取余操作，不会大于rrCount
    // 只有某个jetty完成后删除，导致rrCount变小时才会出现这种情况。此时重置轮询位置
    if (m_tpRRIndex[tpn] > rrCount) {
        m_tpRRIndex[tpn] = 0;
    }

    uint32_t rrIndex = m_tpRRIndex[tpn] % rrCount;
    NS_LOG_DEBUG("This tp relatedJettys.size(): " << std::to_string(tpRelatedJetties.size()));
    NS_LOG_DEBUG("This tp RelatedRemoteRequests.size(): " << std::to_string(remoteRequestSegMap.size()));
    if (rrIndex < jettyCount) { // 轮询本地jetty
        // 获取当前jetty
        Ptr<UbJetty> currentJetty = tpRelatedJetties[rrIndex];
        // 检查jetty是否有效
        if (currentJetty == nullptr) { // 当前jetty无效，轮询下一个
            NS_LOG_WARN("Found null Jetty at index" << rrIndex);
            m_tpRRIndex[tpn] = (rrIndex + 1) % rrCount;
            ScheduleWqeSegment(tp);
        }
        // 尝试从当前Jetty获取WQE Segment
        Ptr<UbWqeSegment> wqeSegment = currentJetty->GetNextWqeSegment();
        if (wqeSegment != nullptr) {
            NS_LOG_INFO("Successfully got WQE Segment from Jetty: " << rrIndex);
            // 设置wqesegment所属tpn
            wqeSegment->SetTpn(tpn);
            // 更新下一次轮询的位置
            m_tpRRIndex[tpn] = (rrIndex + 1) % rrCount;
            // 此处暂时以schedule形式模拟内存调度是时延，当前视为0
            Simulator::ScheduleNow(&UbTransaction::OnScheduleWqeSegmentFinish, this, wqeSegment);
        }
    } else { // 轮询RemoteRequest
        uint32_t remoteIndex = rrIndex - jettyCount;
        auto it = remoteRequestSegMap.begin();
        // it地址只想第index个位置
        std::advance(it, remoteIndex);
        std::vector<Ptr<UbWqeSegment>> segVec = it->second;
        // 取出该vec的第一个WqeSegment
        if (segVec.size() > 0) {
            Ptr<UbWqeSegment> wqeSegment = segVec[0];
            if (wqeSegment != nullptr) {
                NS_LOG_INFO("Successfully got WQE Segment from RemoteRequests, " << remoteIndex);
                // 更新下一次轮询的位置
                m_tpRRIndex[tpn] = (rrIndex + 1) % rrCount;
                // 此处暂时以schedule形式模拟内存调度的时延
                Simulator::ScheduleNow(&UbTransaction::OnScheduleWqeSegmentFinish, this, wqeSegment);
            }
            it->second.erase(it->second.begin());
        }
    }
}

void UbTransaction::OnScheduleWqeSegmentFinish(Ptr<UbWqeSegment> segment)
{
    Ptr<UbTransportChannel> tp = m_tpnMap[segment->GetTpn()];
    segment->SetTpMsn(tp->GetMsnCnt());
    segment->SetPsnStart(tp->GetPsnCnt());
    tp->UpDatePsnCnt(segment->GetPsnSize());
    tp->UpDateMsnCnt(1);
    tp->PushWqeSegment(segment);
    NS_LOG_INFO("WQE Segment Sends, taskId:" << segment->GetTaskId()
        << "TASSN: "<< segment->GetTaSsn());
    tp->WqeSegmentTriggerPortTransmit(segment);
    // TP调度状态重置
    m_tpSchedulingStatus[segment->GetTpn()] = false;
    ScheduleWqeSegment(tp);
}

bool UbTransaction::ProcessWqeSegmentComplete(Ptr<UbWqeSegment> wqeSegment)
{
    Ptr<UbJetty> jetty = GetJetty(wqeSegment->GetJettyNum());
    return jetty->ProcessWqeSegmentComplete(wqeSegment->GetTaSsn());
}

void UbTransaction::TriggerTpTransmit(uint32_t jettyNum)
{
    const std::vector<Ptr<UbTransportChannel>> ubTransportGroupVec = GetJettyRelatedTpVec(jettyNum);
    for (uint32_t i = 0; i < ubTransportGroupVec.size(); i++) {
        ubTransportGroupVec[i]->ApplyNextWqeSegment();
    }
}


// 用于判断某个wqe是否order
bool UbTransaction::IsOrderedByInitiator(Ptr<UbWqe> wqe)
{
    // ROI
    bool res;
    if (m_serviceMode == TransactionServiceMode::ROI) {
        // 判断NO/RO/SO关系
        switch (wqe->GetOrderType()) {
            case OrderType::ORDER_NO:
            case OrderType::ORDER_RELAX:
                res = true;
                break;
            case OrderType::ORDER_STRONG:
                res = (m_wqeVector[0] == wqe->GetWqeId());
                break;
            case OrderType::ORDER_RESERVED:
                res = true;
            default:
                res = true;
        }
    } else { // 不是ROI，直接返回true
        res = true;
    }
    return res;
}

bool UbTransaction::IsOrderedByTarget(Ptr<UbWqe> wqe)
{
    NS_LOG_DEBUG("IsOrderedByTarget");
    return true;
}

bool UbTransaction::IsReliable(Ptr<UbWqe> wqe)
{
    return true;
}

bool UbTransaction::IsUnreliable(Ptr<UbWqe> wqe)
{
    return false;
}

void UbTransaction::AddWqe(Ptr<UbWqe> wqe)
{
    if (m_serviceMode == TransactionServiceMode::ROI
        && (wqe->GetOrderType() == OrderType::ORDER_RELAX || wqe->GetOrderType() == OrderType::ORDER_STRONG)) {
        m_wqeVector.push_back(wqe->GetWqeId());
    }
}

void UbTransaction::WqeFinish(Ptr<UbWqe> wqe)
{
    if (m_serviceMode == TransactionServiceMode::ROI) {
        // 从vector中寻找该wqe并删除
        auto it = std::find(m_wqeVector.begin(), m_wqeVector.end(), wqe->GetWqeId());
        if (it != m_wqeVector.end()) {
            m_wqeVector.erase(it);
        }
    }
}

void UbTransaction::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_wqeVector.clear();
    for (auto &it : m_jettyTpGroup) {
        for (auto tp : it.second) {
            tp = nullptr;
        }
    }
    m_jettyTpGroup.clear();
    m_random = nullptr;
    Object::DoDispose();
}

} // namespace ns3