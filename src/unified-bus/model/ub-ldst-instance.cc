// SPDX-License-Identifier: GPL-2.0-only
#include "ns3/ub-ldst-instance.h"
#include "ns3/ub-ldst-thread.h"
#include "ns3/hbm-bank.h"
#include "ns3/hbm-helper.h"
#include "ns3/hbm-helper-simple.h"
#include "ns3/hbm-controller-simple.h"
#include "ub-ldst-instance.h"
#include "ns3/random-variable-stream.h"
#include "control-macro.h"
#include "traffic-macro.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("UbLdstInstance");

NS_OBJECT_ENSURE_REGISTERED(UbLdstInstance);
TypeId UbLdstInstance::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UbLdstInstance")
                            .SetParent<Object>()
                            .SetGroupName("UnifiedBus")
                            .AddAttribute("ThreadNum",
                                          "Number of LDST worker threads.",
                                          UintegerValue(48),
                                          MakeUintegerAccessor(&UbLdstInstance::m_threadNum),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("QueuePriority",
                                          "Queue (VOQ) priority for packets emitted by this thread.",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&UbLdstInstance::m_queuePriority),
                                          MakeUintegerChecker<uint32_t>())
                            .AddTraceSource("MemTaskStartsNotify",
                                            "Emitted when a memory task starts on this thread.",
                                            MakeTraceSourceAccessor(&UbLdstInstance::m_traceMemTaskStartsNotify),
                                            "ns3::UbLdstInstance::MemTaskStartsNotify")
                            .AddTraceSource("LastPacketACKsNotify",
                                            "Emitted when the last packet of a task is ACKed.",
                                            MakeTraceSourceAccessor(&UbLdstInstance::m_traceLastPacketACKsNotify),
                                            "ns3::UbLdstInstance::LastPacketACKsNotify")
                            .AddTraceSource("MemTaskCompletesNotify",
                                            "Emitted when a memory task completes.",
                                            MakeTraceSourceAccessor(&UbLdstInstance::m_traceMemTaskCompletesNotify),
                                            "ns3::UbLdstInstance::MemTaskCompletesNotify")
                            .AddTraceSource("FirstPacketSendsNotify",
                                            "Emitted when the first packet of a memory task is sent.",
                                            MakeTraceSourceAccessor(&UbLdstInstance::m_traceFirstPacketSendsNotify),
                                            "ns3::UbLdstInstance::FirstPacketSendsNotify")
                            .AddTraceSource("LastPacketSendsNotify",
                                            "Emitted when the last packet of a memory task is sent.",
                                            MakeTraceSourceAccessor(&UbLdstInstance::m_traceLastPacketSendsNotify),
                                            "ns3::UbLdstInstance::LastPacketSendsNotify");

    return tid;
}

UbLdstInstance::UbLdstInstance() 
{
}

UbLdstInstance::~UbLdstInstance()
{
}

void UbLdstInstance::Init(uint32_t nodeId)
{
    for (uint32_t threadId = 0; threadId < m_threadNum; threadId++) {
        auto ldstThread = CreateObject<UbLdstThread>();

        ldstThread->SetNode(nodeId);
        ldstThread->SetThreadId(threadId);
        m_threads.push_back(ldstThread);
    }
    
}

void UbLdstInstance::DoDispose()
{
    m_threads.clear();
}

void UbLdstInstance::SetClientCallback(Callback<void, uint32_t> cb)
{
    FinishCallback = cb;
}

void UbLdstInstance::HandleLdstTask(uint32_t src, uint32_t dest, uint32_t length, uint32_t taskId,
                                    UbMemOperationType type, const std::vector<uint32_t> &threadIds, uint64_t address)
{
    uint32_t threadsNum = threadIds.size();
    // 将数据均分下发给thread
    uint32_t partSize = length / threadsNum;

    FirstPacketSendsNotify(this->GetObject<Node>()->GetId(), taskId);
    MemTaskStartsNotify(this->GetObject<Node>()->GetId(), taskId);
    for (uint32_t i = 0; i < threadsNum; i++) {
        uint32_t segmentSize = partSize;
        if (i == threadsNum - 1) {
            segmentSize += length - partSize * threadsNum;
        }
        uint32_t threadId = threadIds[i];
        auto ldstThread = GetLdstThread(threadId);
        auto taskSegment = CreateObject<UbLdstTaskSegment>();
        taskSegment->SetSrc(src);
        taskSegment->SetDest(dest);
        taskSegment->SetSize(segmentSize);
        taskSegment->SetTaskId(taskId);
        taskSegment->SetTaskSegmentId(m_currentTaskId);
        taskSegment->SetType(type);
        taskSegment->SetThreadId(threadId);
        
        m_taskToSegmentMap[taskId].push_back(taskSegment);
        m_taskSegmentsMap[m_currentTaskId] = taskSegment;
        m_taskSegmentCompletedNum[taskId] = 0;
        m_currentTaskId++;
        Simulator::ScheduleNow(&UbLdstThread::PushTaskSegment, ldstThread, taskSegment);
    }
}

void UbLdstInstance::OnRecvAck(uint32_t taskSegmentId)
{
    auto taskSegment = m_taskSegmentsMap[taskSegmentId];
    if (taskSegment == nullptr) {
        NS_ASSERT_MSG(0, "taskSegment invalid!");
    }
    uint32_t threadId = taskSegment->GetThreadId();
    auto ldstThread = GetLdstThread(threadId);
    Simulator::ScheduleNow(&UbLdstThread::UpdateTask, ldstThread, taskSegment);
}

void UbLdstInstance::OnTaskSegmentCompleted(uint32_t taskId)
{
    m_taskSegmentCompletedNum[taskId]++;
    if (m_taskSegmentCompletedNum[taskId] == m_taskToSegmentMap[taskId].size()) {
        LastPacketACKsNotify(this->GetObject<Node>()->GetId(), taskId);
        MemTaskCompletesNotify(this->GetObject<Node>()->GetId(), taskId);
        FinishCallback(taskId);
    }
}

Ptr<UbLdstThread> UbLdstInstance::GetLdstThread(uint32_t threadId)
{
    if (threadId > m_threads.size()) {
        NS_ASSERT_MSG(0, "Invalid threadId! Cannot Get Ldst Thread.");
    }
    return m_threads[threadId];
}

void UbLdstInstance::LastPacketACKsNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceLastPacketACKsNotify(nodeId, taskId);
}

void UbLdstInstance::MemTaskCompletesNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceMemTaskCompletesNotify(nodeId, taskId);
}

void UbLdstInstance::MemTaskStartsNotify(uint32_t nodeId, uint32_t memTaskId)
{
    m_traceMemTaskStartsNotify(nodeId, memTaskId);
}

void UbLdstInstance::FirstPacketSendsNotify(uint32_t nodeId, uint32_t memTaskId)
{
    m_traceFirstPacketSendsNotify(nodeId, memTaskId);
}
 
void UbLdstInstance::LastPacketSendsNotify(uint32_t nodeId, uint32_t memTaskId)
{
    m_traceLastPacketSendsNotify(nodeId, memTaskId);
}

#ifdef USE_SIMPLE_HBM
    void UbLdstInstance::InternalHBMAccess(void) {
        auto node = GetObject<Node>();
        auto rng = node->GetObject<UniformRandomVariable>();
        auto hbm = node->GetObject<SimpleHBMController>();
        
        // Changable. Add logic to simulate more realistic background traffic
        auto bg_intensity = rng->GetInteger(0, 5);
        hbm->SetBackgroundIntensity(bg_intensity);

        Simulator::Schedule(NanoSeconds(m_fire_period), &UbLdstInstance::InternalHBMAccess, this);
    }

    void UbLdstInstance::Init(void) {
        Simulator::Schedule(NanoSeconds(10), &UbLdstInstance::InternalHBMAccess, this);
    }

#else
    void UbLdstInstance::Init(void) {
        uint64_t pages = std::max(1UL, m_workingSetSize / HBM_ROW_SIZE);

        auto rng = GetObject<Node>()->GetObject<UniformRandomVariable>();
        uint32_t random_address_lower_half = rng->GetInteger(0, 0xFFFFFFFF) & ROW_ALIGNED_AND_BIT_MASK;
        uint32_t random_address_upper_half = rng->GetInteger(0, MAX_PHYSICAL_ADDRESS_UPPER_HALF);
        uint64_t random_base_address = (static_cast<uint64_t>(random_address_upper_half) << 32ULL) + random_address_lower_half;
        
        m_usedPages.reserve(pages);

        for(uint64_t i = 0; i < pages; i++) {
            m_usedPages.push_back(random_base_address + i * HBM_ROW_SIZE);
        }

        m_exp_rng = CreateObject<ExponentialRandomVariable>();
        
        double lambda = LLC_MISS_PER_KILO_INSTRUCTION / 1000.0 * m_ipc * m_clockHz * m_numSM * m_activeSMRatio;
        NS_LOG_INFO("lambda is " << lambda);
        m_exp_rng->SetAttribute("Mean", DoubleValue(1.0 / lambda));

        Simulator::ScheduleNow(&UbLdstInstance::ScheduleNextAccess, this);
    }

    void UbLdstInstance::ScheduleNextAccess() {
        if(m_blocked) return;
        auto rng = GetObject<Node>()->GetObject<UniformRandomVariable>();
        if (m_outstanding > m_maxOutstanding) {
            uint32_t delay = 1000 + rng->GetInteger(0, 1000);
            Simulator::Schedule(NanoSeconds(delay), &UbLdstInstance::ScheduleNextAccess, this);
        }
        else {
            double interval = m_exp_rng->GetValue();
            NS_LOG_INFO("Wait for " << interval);
            Simulator::Schedule(Seconds(interval), &UbLdstInstance::InternalHBMAccess, this);
        }
    }

    void UbLdstInstance::InternalHBMAccess(void) {
        if (m_outstanding < m_maxOutstanding) {
            NS_LOG_INFO("outstanding is " << m_outstanding);
            m_outstanding ++;
            NS_LOG_INFO("Issued one HBM access at " << Simulator::Now().GetNanoSeconds() << " on node " << GetObject<Node>()->GetId());
            bool backpressure = not GetObject<Node>()->GetObject<HBMController>()->SendRequest(ChooseAddress(), ChooseSize(), true,
                false, MakeCallback(&UbLdstInstance::OnHBMComplete, this), static_cast<void*>((int*)1));

            if (backpressure) {
                NS_LOG_INFO("HBM backpressured, stalled");
                m_blocked = true;
            } 
            else Simulator::ScheduleNow(&UbLdstInstance::ScheduleNextAccess, this);
        }
        else
            NS_LOG_INFO("stalled because used up all otsd credits on " << GetObject<Node>()->GetId());
    }

    void UbLdstInstance::OnHBMComplete(void* arg) {
        if (arg != nullptr)
            m_outstanding --;
        if (m_blocked) {
            m_blocked = false;
            NS_LOG_INFO("Resumed execution");
            Simulator::ScheduleNow(&UbLdstInstance::ScheduleNextAccess, this);
        }
    }

    uint64_t UbLdstInstance::ChooseAddress() {
        auto rng = GetObject<Node>()->GetObject<UniformRandomVariable>();
        double p = rng->GetValue();

        if(p < m_intraSetProbability) {
            uint64_t random_base_address = m_usedPages[rng->GetInteger(0, m_usedPages.size()-1)];
            uint64_t max_offset = HBM_ROW_SIZE - 64;
            uint64_t offset = rng->GetInteger(0, max_offset / 64) * 64;

            return random_base_address + offset;
            
        }
        else {
            uint32_t random_address_lower_half = rng->GetInteger(0, 0xFFFFFFFF) & ROW_ALIGNED_AND_BIT_MASK;
            uint32_t random_address_upper_half = rng->GetInteger(0, MAX_PHYSICAL_ADDRESS_UPPER_HALF);
            return (static_cast<uint64_t>(random_address_upper_half) << 32ULL) + random_address_lower_half;

        }
    }

    uint32_t UbLdstInstance::ChooseSize() {
        double x = GetObject<Node>()->GetObject<UniformRandomVariable>()->GetValue();
        double sum = 0.0;

        for(auto& i : m_sizeDist) {
            sum += i.second;
            if (x <= sum) return i.first;
        }
        return m_sizeDist.back().first;
    }

#endif


}