#ifndef UB_API_URMA_H
#define UB_API_URMA_H

#include <vector>
#include <unordered_map>
#include <set>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/ub-datatype.h"
#include "ns3/ub-controller.h"
#include "ns3/ub-api-ldst.h"
#include "../ub-tp-connection-manager.h"

using namespace utils;
namespace ns3 {

/**
 * @brief traffic-config.csv 中定义的数据结构
 */
struct TrafficRecord {
    int taskId;
    int sourceNode;
    int destNode;
    int dataSize;
    string opType;
    int priority;
    string delay;
    int phaseId;
    vector<uint32_t> dependOnPhases;
};

/**
 * @brief 任务图应用，管理多个WQE任务及其依赖关系
 */
class UbApiUrma : public Application {
public:
    static TypeId GetTypeId(void);

    UbApiUrma();
    virtual ~UbApiUrma();

    void AddTask(uint32_t taskId, TrafficRecord record, const std::set<uint32_t>& dependencies = {});

    void MarkTaskCompleted(uint32_t taskId);

    bool IsCompleted() const;

    void SetNode(Ptr<Node> node); // 设置当前节点

    void GetTpnConn(TpConnectionManager tpConnection); // 获取tpnconnection

    void SetGetTpnRule(GetTpnRuleT type)
    {
        m_getTpnRule = type;
    }

    void SetUseShortestPath(bool useShortestPath)
    {
        m_useShortestPath = useShortestPath;
    }

    // ========== 回调函数 ==========
    void SetFinishCallback(Callback<void, uint32_t, uint32_t> cb, Ptr<UbJetty> jetty);
    void SetFinishCallback(Callback<void, uint32_t> cb, Ptr<UbApiLdst> UbApiLdst);

protected:
    void DoDispose(void) override;

private:

    TracedCallback<uint32_t, uint32_t> m_traceMemTaskStartsNotify;
    TracedCallback<uint32_t, uint32_t> m_traceMemTaskCompletesNotify;
    TracedCallback<uint32_t, uint32_t, uint32_t> m_traceWqeTaskStartsNotify;
    TracedCallback<uint32_t, uint32_t, uint32_t> m_traceWqeTaskCompletesNotify;

    void MemTaskStartsNotify(uint32_t nodeId, uint32_t taskId);
    void MemTaskCompletesNotify(uint32_t nodeId, uint32_t taskId);
    void WqeTaskStartsNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId);
    void WqeTaskCompletesNotify(uint32_t nodeId, uint32_t jettyNum, uint32_t taskId);

    // ========== Application接口实现 ==========
    void StartApplication(void) override;
    void StopApplication(void) override;

    // ========== DAG调度逻辑 ==========
    /**
     * @brief 调度下一批可执行的任务
     */
    void ScheduleNextTasks();

    void SendTraffic(TrafficRecord record);
    void SendTrafficForTest(TrafficRecord record);

    /**
     * @brief 任务完成回调
     */
    void OnTaskCompleted(uint32_t taskId, uint32_t jettyNum);

    void OnTestTaskCompleted(uint32_t taskId, uint32_t jettyNum);
    void OnMemTaskCompleted(uint32_t taskId);

    TaOpcode StringToEnum(const std::string& str);

    // ========== 数据成员 ==========
    // 任务状态枚举
    enum class TaskState {
        PENDING,    // 等待依赖完成
        READY,      // 就绪，可以调度
        RUNNING,    // 正在执行
        COMPLETED   // 已完成
    };

    map<std::string, TaOpcode> TaOpcodeMap = {
        {"URMA_WRITE", TaOpcode::TA_OPCODE_WRITE},
        {"MEM_STORE", TaOpcode::TA_OPCODE_WRITE},
        {"MEM_LOAD", TaOpcode::TA_OPCODE_READ}
    };
    // DAG结构
    std::unordered_map<uint32_t, TrafficRecord> m_tasks{};
    std::unordered_map<uint32_t, std::set<uint32_t>> m_dependencies{}; // 每个任务ID对应其依赖的任务ID集合（即该任务必须等待这些任务完成）
    std::unordered_map<uint32_t, std::set<uint32_t>> m_dependents{};   // 每个任务ID对应所有依赖它的任务ID集合（即这些任务等待该任务完成
    std::unordered_map<uint32_t, TaskState> m_taskStates{};
    std::set<uint32_t> m_readyTasks{};

    // 控制器

    // 任务ID计数器
    uint32_t m_nextTaskId = 0;

    bool m_multiPathEnable;

    GetTpnRuleT m_getTpnRule = GetTpnRuleT::BY_PEERNODE_PRIORITY; // 获取Tpn的方式
    bool m_useShortestPath = true; // 是否根据跳数寻找最短路径

    Ptr<Node> m_node;               // 当前节点
    TpConnectionManager m_tpnConn;  // 当前节点维护的tpnConn
    uint32_t  m_jettyNum = 0;       // 当前节点维护的jettynum,不会重复
    uint32_t  threadId = 0;         // 当前节点维护的threadId,不会重复
};

} // namespace ns3

#endif /* UB_CLIENT_DAG_H */
