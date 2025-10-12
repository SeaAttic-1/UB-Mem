#include "ub-api-urma.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/callback.h"
#include "ns3/ub-datatype.h"
#include "ub-function.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UbApiUrma");

NS_OBJECT_ENSURE_REGISTERED(UbApiUrma);
TypeId UbApiUrma::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UbApiUrma")
        .SetParent<Application>()
        .SetGroupName("UnifiedBus")
        .AddConstructor<UbApiUrma>()
        .AddAttribute("EnableMultiPath",
                      "Enable multiPath.",
                      BooleanValue(false),
                      MakeBooleanAccessor(&UbApiUrma::m_multiPathEnable),
                      MakeBooleanChecker())
        .AddTraceSource("MemTaskStartsNotify",
                        "MEM Task Starts, taskId",
                        MakeTraceSourceAccessor(&UbApiUrma::m_traceMemTaskStartsNotify),
                        "ns3::UbApiUrma::MemTaskStartsNotify")
        .AddTraceSource("MemTaskCompletesNotify",
                        "MEM Task Completes, taskId",
                        MakeTraceSourceAccessor(&UbApiUrma::m_traceMemTaskCompletesNotify),
                        "ns3::UbApiUrma::MemTaskCompletesNotify")
        .AddTraceSource("WqeTaskStartsNotify",
                        "WQE Task Starts, taskId",
                        MakeTraceSourceAccessor(&UbApiUrma::m_traceWqeTaskStartsNotify),
                        "ns3::UbApiUrma::WqeTaskStartsNotify")
        .AddTraceSource("WqeTaskCompletesNotify",
                        "WQE Task Completes, taskId",
                        MakeTraceSourceAccessor(&UbApiUrma::m_traceWqeTaskCompletesNotify),
                        "ns3::UbApiUrma::WqeTaskCompletesNotify");
    return tid;
}

UbApiUrma::UbApiUrma()
{
}

UbApiUrma::~UbApiUrma()
{
}

void UbApiUrma::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void UbApiUrma::GetTpnConn(TpConnectionManager tpConnection)
{
    m_tpnConn = tpConnection;
}

TaOpcode UbApiUrma::StringToEnum(const std::string& str)
{
    auto it = TaOpcodeMap.find(str);
    if (it != TaOpcodeMap.end()) {
        return it->second;
    } else {
        NS_ASSERT_MSG (it != TaOpcodeMap.end(), "TaOpcode Not Exist");
        return TaOpcode::TA_OPCODE_MAX;
    }
}

void UbApiUrma::AddTask(uint32_t taskId, TrafficRecord record, const std::set<uint32_t> &dependencies)
{
    NS_LOG_FUNCTION(this << taskId << dependencies.size());
    if (m_tasks.find(taskId) != m_tasks.end()) {
        NS_LOG_ERROR("TaskId " << taskId << " already exists, cannot add duplicate task!");
        return;
    }
    m_tasks[taskId] = record;

    m_dependencies[taskId] = std::set<uint32_t>(dependencies.begin(), dependencies.end());

    // 设置初始状态
    if (dependencies.empty()) {
        m_taskStates[taskId] = TaskState::READY;
        m_readyTasks.insert(taskId);
    } else {
        m_taskStates[taskId] = TaskState::PENDING;
    }

    // 建立反向依赖映射
    for (uint32_t depId : dependencies) {
        m_dependents[depId].insert(taskId);
    }

    NS_LOG_DEBUG("Added task " << taskId << " with " << dependencies.size() << " dependencies");
}

void UbApiUrma::MarkTaskCompleted(uint32_t taskId)
{
    // 检查任务是否正在运行
    if (m_taskStates[taskId] != TaskState::RUNNING) {
        return;
    }

    // 更新状态
    m_taskStates[taskId] = TaskState::COMPLETED;
    NS_LOG_DEBUG("Task " << taskId << " completed");

    // 检查依赖此任务的任务
    auto dependentIt = m_dependents.find(taskId);
    if (dependentIt != m_dependents.end()) {
        for (uint32_t dependentId : dependentIt->second) {
            // 只处理等待状态的任务
            if (m_taskStates[dependentId] != TaskState::PENDING) {
                continue;
            }

            // 检查该任务的所有依赖是否都已完成
            bool allDepsCompleted = true;
            auto depIt = m_dependencies.find(dependentId);
            if (depIt != m_dependencies.end()) {
                for (uint32_t depId : depIt->second) {
                    if (m_taskStates[depId] != TaskState::COMPLETED) {
                        allDepsCompleted = false;
                        break;
                    }
                }
            }

            if (allDepsCompleted) {
                m_taskStates[dependentId] = TaskState::READY;
                m_readyTasks.insert(dependentId);
            }
        }
    }

    ScheduleNextTasks();

    // 检查是否全部完成
    if (IsCompleted()) {
        NS_LOG_DEBUG("[APPLICATION INFO] All tasks completed");
    }
}

bool UbApiUrma::IsCompleted() const
{
    for (const auto& statePair : m_taskStates) {
        if (statePair.second != TaskState::COMPLETED) {
            return false;
        }
    }
    return true;
}

void UbApiUrma::SetFinishCallback(Callback<void, uint32_t, uint32_t> cb, Ptr<UbJetty> jetty)
{
    jetty->SetClientCallback(cb);
}

void UbApiUrma::SetFinishCallback(Callback<void, uint32_t> cb, Ptr<UbApiLdst> ubApiLdst)
{
    ubApiLdst->SetClientCallback(cb);
}

void UbApiUrma::DoDispose(void)
{
    m_tasks.clear();
    m_dependencies.clear();
    m_taskStates.clear();
    m_dependents.clear();
    m_readyTasks.clear();
    Application::DoDispose();
}

void UbApiUrma::StartApplication(void)
{
    // 获取控制器
    if (m_node->GetObject<UbController>() == nullptr) {
        NS_LOG_ERROR("UbController not found on node");
        return;
    }

    // 开始调度第一批可执行任务
    ScheduleNextTasks();
}

void UbApiUrma::StopApplication(void)
{
    m_readyTasks.clear();
}

// 本用例所有原目的节点相同的wqe都绑在同一个jetty上
void UbApiUrma::SendTrafficForTest(TrafficRecord record)
{
    if (record.priority == 0) {
        NS_LOG_DEBUG("Task uses the highest priority, not recommended.");
    }
    std::vector<OrderType> type = {
        OrderType::ORDER_NO,
        OrderType::ORDER_RELAX,
        OrderType::ORDER_STRONG,
        OrderType::ORDER_NO,
        OrderType::ORDER_NO,
        OrderType::ORDER_RELAX,
        OrderType::ORDER_STRONG,
        OrderType::ORDER_NO};
    if (record.opType == "URMA_WRITE") {
        // URMA发送
        Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
        bool jettyExist = ubFunc->IsJettyExists(0);
        if (!jettyExist) {
            ubFunc->CreateJetty(record.sourceNode, record.destNode, 0);
        } else {
            m_jettyNum++;
        }
        vector<uint32_t> m_tpns = m_tpnConn.GetTpns(m_getTpnRule, m_useShortestPath, record.sourceNode,
                                                    record.destNode, UINT32_MAX, UINT32_MAX, record.priority);
        NS_ASSERT_MSG (m_tpns.empty() == false, "Tpns Not Exist");
        bool bindRst = ubFunc->jettyBindTp(record.sourceNode, record.destNode, 0, m_multiPathEnable, m_tpns);
        if (bindRst) {
            Ptr<UbJetty> curr_jetty = ubFunc->GetJetty(0);
            SetFinishCallback(MakeCallback(&UbApiUrma::OnTestTaskCompleted, this), curr_jetty);
            NS_LOG_INFO("WQE Starts, jettyNum: " << 0 << " taskId: " << record.taskId);
            WqeTaskStartsNotify(m_node->GetId(), 0, record.taskId);
            NS_LOG_INFO("[APPLICATION INFO] taskId:" << record.taskId << ",start time:" <<
                        Simulator::Now().GetNanoSeconds() << "ns");
            Ptr<UbWqe> wqe = ubFunc->CreateWqe(record.sourceNode, record.destNode, record.dataSize, record.taskId);

            wqe->SetOrderType(type[record.taskId]);
            ubFunc->PushWqeToJetty(wqe, 0);
        }
    } else {
        NS_ASSERT_MSG (0, "TaOpcode Not Exist");
    }
}

void UbApiUrma::SendTraffic(TrafficRecord record)
{
    if (record.priority == 0) {
        NS_LOG_DEBUG("Task uses the highest priority, not recommended.");
    }

    if (record.opType == "MEM_STORE" || record.opType == "MEM_LOAD") {
        // 内存语义发送
        UbMemOpearationType type = UbMemOpearationType::STORE;
        if (record.opType == "MEM_STORE") {
            type = UbMemOpearationType::STORE;
        } else if (record.opType == "MEM_LOAD") {
            type = UbMemOpearationType::LOAD;
        }
        Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
        Ptr<UbApiLdst> ldst = ubFunc->GetUbLdst();
        SetFinishCallback(MakeCallback(&UbApiUrma::OnMemTaskCompleted, this), ldst);
        NS_LOG_INFO("MEM Task Starts, taskId: " << record.taskId);
        MemTaskStartsNotify(m_node->GetId(), record.taskId);
        ubFunc->PushLdstTask(record.sourceNode, record.destNode, record.dataSize,
                             record.taskId, type, threadId);
        threadId++;
        if (threadId >= ldst->GetThreadNum()) {
            threadId = threadId % ldst->GetThreadNum();
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
        vector<uint32_t> m_tpns = m_tpnConn.GetTpns(m_getTpnRule, m_useShortestPath, record.sourceNode,
                                                    record.destNode, UINT32_MAX, UINT32_MAX, record.priority);
        NS_ASSERT_MSG (m_tpns.empty() == false, "Tpns Not Exist");
        bool bindRst = ubFunc->jettyBindTp(record.sourceNode, record.destNode, m_jettyNum, m_multiPathEnable, m_tpns);
        if (bindRst) {
            Ptr<UbJetty> curr_jetty = ubFunc->GetJetty(m_jettyNum);
            SetFinishCallback(MakeCallback(&UbApiUrma::OnTaskCompleted, this), curr_jetty);
            NS_LOG_INFO("WQE Starts, jettyNum: " << m_jettyNum << " taskId: " << record.taskId);
            WqeTaskStartsNotify(m_node->GetId(), m_jettyNum, record.taskId);
            NS_LOG_INFO("[APPLICATION INFO] taskId:" << record.taskId << ",start time:" <<
                        Simulator::Now().GetNanoSeconds() << "ns");
            Ptr<UbWqe> wqe = ubFunc->CreateWqe(record.sourceNode, record.destNode, record.dataSize, record.taskId);
            ubFunc->PushWqeToJetty(wqe, m_jettyNum);
        }
        m_jettyNum++; // m_jettyNum 在client里是唯一，不重复的
    } else {
        NS_ASSERT_MSG (0, "TaOpcode Not Exist");
    }
}

void UbApiUrma::ScheduleNextTasks()
{
    for (auto it = m_readyTasks.begin(); it != m_readyTasks.end();) {
        uint32_t taskId = *it;
        // 确认任务仍然是就绪状态
        if (m_taskStates[taskId] == TaskState::READY) {
            m_taskStates[taskId] = TaskState::RUNNING;

            auto taskIt = m_tasks.find(taskId);
            if (taskIt != m_tasks.end()) {
                SendTraffic(taskIt->second);
                NS_LOG_DEBUG("Scheduled task " << taskId);
            }
        }

        it = m_readyTasks.erase(it);
    }
}

void UbApiUrma::OnTaskCompleted(uint32_t taskId, uint32_t jettyNum)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("WQE Completes, jettyNum:" << jettyNum << " taskId:" << taskId);
    WqeTaskCompletesNotify(m_node->GetId(), jettyNum, taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId:" << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
    ubFunc->DestroyJetty(jettyNum);
    MarkTaskCompleted(taskId);
}

void UbApiUrma::OnTestTaskCompleted(uint32_t taskId, uint32_t jettyNum)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("WQE Completes, jettyNum:" << jettyNum << " taskId:" << taskId);
    WqeTaskCompletesNotify(m_node->GetId(), jettyNum, taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId:" << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    Ptr<UbFunction> ubFunc = m_node->GetObject<UbController>()->GetUbFunction();
    MarkTaskCompleted(taskId);
}

void UbApiUrma::OnMemTaskCompleted(uint32_t taskId)
{
    NS_LOG_FUNCTION(this << taskId);
    NS_LOG_INFO("MEM Task Completes, taskId: " << taskId);
    MemTaskCompletesNotify(m_node->GetId(), taskId);
    NS_LOG_INFO("[APPLICATION INFO] taskId:" << taskId << ",finish time:" << Simulator::Now().GetNanoSeconds() << "ns");
    MarkTaskCompleted(taskId);
}

void UbApiUrma::MemTaskStartsNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceMemTaskStartsNotify(nodeId, taskId);
}

void UbApiUrma::MemTaskCompletesNotify(uint32_t nodeId, uint32_t taskId)
{
    m_traceMemTaskCompletesNotify(nodeId, taskId);
}

void UbApiUrma::WqeTaskStartsNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId)
{
    m_traceWqeTaskStartsNotify(nodeId, jettyNum, taskId);
}

void UbApiUrma::WqeTaskCompletesNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId)
{
    m_traceWqeTaskCompletesNotify(nodeId, jettyNum, taskId);
}

} // namespace ns3
