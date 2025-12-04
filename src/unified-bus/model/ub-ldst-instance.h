// SPDX-License-Identifier: GPL-2.0-only
#ifndef UB_LDST_INSTANCE_H
#define UB_LDST_INSTANCE_H

#include "ns3/ub-datatype.h"
#include "ns3/ub-network-address.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/hbm-helper.h"
#include "ns3/hbm-controller.h"
#include "ns3/hbm-helper-simple.h"
#include "ns3/hbm-controller-simple.h"
#include "control-macro.h"

namespace ns3 {
class UbLdstThread;
class HBMController;
class UbLdstInstance : public Object {
public:
    static TypeId GetTypeId(void);
    UbLdstInstance();
    virtual ~UbLdstInstance();
    void DoDispose(void) override;
    void Init(uint32_t nodeId);
    // 接收任务接口，分配给thread
    void HandleLdstTask(uint32_t src, uint32_t dest, uint32_t size, uint32_t taskId,
                        UbMemOperationType type, const std::vector<uint32_t> &threadIds, uint64_t address);

    void SetClientCallback(Callback<void, uint32_t> cb);
    Ptr<UbLdstThread> GetLdstThread(uint32_t threadId);
    Callback<void, uint32_t> FinishCallback;
    void OnRecvAck(uint32_t taskSegmentId);
    void OnTaskSegmentCompleted(uint32_t taskId);

    void InternalHBMAccess(void);
    void Init(void);
    

private:
    void MemTaskStartsNotify(uint32_t nodeId, uint32_t memTaskId);
    void LastPacketACKsNotify(uint32_t nodeId, uint32_t taskId);
    void MemTaskCompletesNotify(uint32_t nodeId, uint32_t taskId);
    void FirstPacketSendsNotify(uint32_t nodeId, uint32_t memTaskId);
    void LastPacketSendsNotify(uint32_t nodeId, uint32_t memTaskId);
    std::unordered_map<uint32_t, std::vector<Ptr<UbLdstTaskSegment>>> m_taskToSegmentMap;  // taskid -> taskSegments
    std::vector<Ptr<UbLdstThread>> m_threads;
    std::unordered_map<uint32_t, uint32_t> m_taskSegmentCompletedNum;
    std::unordered_map<uint32_t, Ptr<UbLdstTaskSegment>> m_taskSegmentsMap;
     
    // This implements the HBM model
    
    uint32_t m_currentTaskId = 0;
    uint32_t m_threadNum = 0;
    uint32_t m_queuePriority = 0;
    
    
    TracedCallback<uint32_t, uint32_t> m_traceLastPacketACKsNotify;
    TracedCallback<uint32_t, uint32_t> m_traceMemTaskCompletesNotify;
    TracedCallback<uint32_t, uint32_t> m_traceMemTaskStartsNotify;
    TracedCallback<uint32_t, uint32_t> m_traceFirstPacketSendsNotify;
    TracedCallback<uint32_t, uint32_t> m_traceLastPacketSendsNotify;

    #ifdef USE_SIMPLE_HBM
        uint32_t m_fire_period = 500;
    #else
        uint64_t m_workingSetSize = 8 * 1024 * 1024; // 8MB working set
        std::vector<uint64_t> m_usedPages;
        std::vector<std::pair<uint32_t, double>> m_sizeDist = {{128, 0.6}, {256, 0.3}, {64, 0.1}};
        uint32_t m_outstanding = 0;

        bool m_blocked = false;

        void ScheduleNextAccess();
        uint64_t ChooseAddress();
        uint32_t ChooseSize();
        void OnHBMComplete(void* arg);

        Ptr<ExponentialRandomVariable> m_exp_rng;

        double m_ipc = 2.0;
        double m_clockHz = 1.5e9;
        double m_activeSMRatio = 0.2;
        uint32_t m_numSM = 128;
        uint32_t m_maxOutstanding = 4096;
        double m_intraSetProbability = 0.8;


    #endif
}; 
} 

#endif /* UB_LDST_INSTANCE_H*/