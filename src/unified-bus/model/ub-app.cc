// SPDX-License-Identifier: GPL-2.0-only
#include "ub-app.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/callback.h"
#include "ns3/ub-datatype.h"
#include "ns3/ub-function.h"
#include "ub-traffic-gen.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UbApp");

NS_OBJECT_ENSURE_REGISTERED(UbApp);
TypeId UbApp::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::UbApp")
            .SetParent<Application>()
            .SetGroupName("UnifiedBus")
            .AddConstructor<UbApp>()
            .AddAttribute("EnableMultiPath",
                          "Enable Multi Path",
                          BooleanValue(false),
                          MakeBooleanAccessor(&UbApp::m_multiPathEnable),
                          MakeBooleanChecker())
            .AddTraceSource("MemTaskStartsNotify",
                            "MEM Task Starts, taskId",
                            MakeTraceSourceAccessor(&UbApp::m_traceMemTaskStartsNotify),
                            "ns3::UbApp::MemTaskStartsNotify")
            .AddTraceSource("MemTaskCompletesNotify",
                            "MEM Task Completes, taskId",
                            MakeTraceSourceAccessor(&UbApp::m_traceMemTaskCompletesNotify),
                            "ns3::UbApp::MemTaskCompletesNotify")
            .AddTraceSource("WqeTaskStartsNotify",
                            "WQE Task Starts, taskId",
                            MakeTraceSourceAccessor(&UbApp::m_traceWqeTaskStartsNotify),
                            "ns3::UbApp::WqeTaskStartsNotify")
            .AddTraceSource("WqeTaskCompletesNotify",
                            "WQE Task Completes, taskId",
                            MakeTraceSourceAccessor(&UbApp::m_traceWqeTaskCompletesNotify),
                            "ns3::UbApp::WqeTaskCompletesNotify");
    return tid;
}

UbApp::UbApp()
{
}

UbApp::~UbApp()
{
}

void UbApp::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void UbApp::GetTpnConn(TpConnectionManager tpConnection)
{
    m_tpnConn = tpConnection;
}

void UbApp::SetFinishCallback(Callback<void, uint32_t, uint32_t> cb, Ptr<UbJetty> jetty)
{
    jetty->SetClientCallback(cb);
}

void UbApp::SetFinishCallback(Callback<void, uint32_t> cb, Ptr<UbApiLdst> ubApiLdst)
{
    ubApiLdst->SetClientCallback(cb);
}

void UbApp::DoDispose(void)
{
    Application::DoDispose();
}

void UbApp::SendTraffic(TrafficRecord record)
{
    if (record.priority == 0) {
        NS_LOG_DEBUG("Task uses the highest priority, not recommended.");
    }

    if (record.opType == "MEM_STORE" || record.opType == "MEM_LOAD") {
        // 内存语义发送
        UbMemOperationType type = UbMemOperationType::STORE;
        if (record.opType == "MEM_STORE") {
            type = UbMemOperationType::STORE;
        } else if (record.opType == "MEM_LOAD") {
            type = UbMemOperationType::LOAD;
        }
        Ptr<UbFunction> ubFunc =
            m_node->GetObject<UbController>()->GetUbFunction();
        Ptr<UbApiLdst> Ldst = ubFunc->GetUbLdst();
        SetFinishCallback(MakeCallback(&UbApp::OnMemTaskCompleted, this),
                          Ldst);
        NS_LOG_INFO("MEM Task Starts, taskId: " << record.taskId);
        MemTaskStartsNotify(m_node->GetId(), record.taskId);
        ubFunc->PushLdstTask(record.sourceNode, record.destNode, record.dataSize,
                          record.taskId, type, threadId);
        threadId++;
        if (threadId >= Ldst->GetThreadNum()) {
            threadId = threadId % Ldst->GetThreadNum();
        }
    } else if (record.opType == "URMA_WRITE") {
        // URMA发送
        Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
        bool jettyExist = ubFunc->IsJettyExists(m_jettyNum);
        if (jettyExist) {
            NS_LOG_ERROR("Jetty already exists");
            return;
        }
        ubFunc->CreateJetty(record.sourceNode, record.destNode, m_jettyNum);
        vector<uint32_t> m_tpns = m_tpnConn.GetTpns(
            m_getTpnRule, m_useShortestPath, record.sourceNode,
            record.destNode, UINT32_MAX, UINT32_MAX, record.priority);
        NS_ASSERT_MSG(m_tpns.empty() == false, "Tpns Not Exist");
        bool bindRst = ubFunc->jettyBindTp(record.sourceNode, record.destNode,
                                        m_jettyNum, m_multiPathEnable, m_tpns);
        if (bindRst) {
            Ptr<UbJetty> curr_jetty = ubFunc->GetJetty(m_jettyNum);
            SetFinishCallback(MakeCallback(&UbApp::OnTaskCompleted, this), curr_jetty);
            NS_LOG_INFO("WQE Starts, jettyNum: " << m_jettyNum << " taskId: " << record.taskId);
            WqeTaskStartsNotify(m_node->GetId(), m_jettyNum, record.taskId);
            NS_LOG_INFO("[APPLICATION INFO] taskId: " << record.taskId << ",start time:"
                        << Simulator::Now().GetNanoSeconds() << "ns");
            Ptr<UbWqe> wqe = ubFunc->CreateWqe(record.sourceNode, record.destNode,
                                record.dataSize, record.taskId);
            ubFunc->PushWqeToJetty(wqe, m_jettyNum);
        }
        m_jettyNum++; // m_jettyNum 在client里是唯一的，不重复的
    } else {
            NS_ASSERT_MSG(0, "TaOpcode Not Exist");
    }
}

void UbApp::OnTaskCompleted(uint32_t taskId, uint32_t jettyNum)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("WQE Completes, jettyNum: " << jettyNum << " taskId: " << taskId);
    WqeTaskCompletesNotify(m_node->GetId(), jettyNum, taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId: " << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
    ubFunc->DestroyJetty(jettyNum);
    UbTrafficGen::GetInstance().OnTaskCompleted(taskId);
}

void UbApp::OnTestTaskCompleted(uint32_t taskId, uint32_t jettyNum)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("WQE Completes, jettyNum:" << jettyNum << " taskId:" << taskId);
    WqeTaskCompletesNotify(m_node->GetId(), jettyNum, taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId:" << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
    UbTrafficGen::GetInstance().OnTaskCompleted(taskId);
}

void UbApp::OnMemTaskCompleted(uint32_t taskId)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("MEM Task Completes, taskId: " << taskId);
    MemTaskCompletesNotify(m_node->GetId(), taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId: " << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    UbTrafficGen::GetInstance().OnTaskCompleted(taskId);
}

void UbApp::MemTaskStartsNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceMemTaskStartsNotify(nodeId, taskId);
}

void UbApp::MemTaskCompletesNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceMemTaskCompletesNotify(nodeId, taskId);
}

void UbApp::WqeTaskStartsNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId)
{
    m_traceWqeTaskStartsNotify(nodeId, jettyNum, taskId);
}

void UbApp::WqeTaskCompletesNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId)
{
    m_traceWqeTaskCompletesNotify(nodeId, jettyNum, taskId);
}

} // namespace ns3

