// SPDX-License-Identifier: GPL-2.0-only
#include <chrono>
#include "ns3/ub-utils.h"

using namespace utils;

ApplicationContainer appCon;
void CheckExampleProcess(unordered_map<int, Ptr<UbApp>> client_map)
{
    PrintTimestamp("Check Example Process.");
    for (auto it = client_map.begin(); it != client_map.end(); it++) {
        Ptr<UbApp> client = it->second;
        if (!client->IsCompleted()) {
            Simulator::Schedule(MicroSeconds(100), &CheckExampleProcess, client_map);
            return ;
        }
    }
    Simulator::Stop();
    return ;
}

// 根据配置文件路径执行用例
void RunCase(const string& configPath)
{
    RngSeedManager::SetSeed(10);
    string LoadConfigFilePath = configPath + "/network_attribute.txt";
    SetComponentsAttribute(LoadConfigFilePath);
    CreateTraceDir();
    string NodeConfigFile = configPath + "/node.csv";
    CreateNode(NodeConfigFile);
    string TopoConfigFile = configPath + "/topology.csv";
    CreateTopo(TopoConfigFile);
    string RouterConfigFile = configPath + "/routing_table.csv";
    AddRoutingTable(RouterConfigFile);
    string TpConfigFile = configPath + "/transport_channel.csv";
    TpConnectionManager retConnectionManager = CreateTp(TpConfigFile);
    TopoTraceConnect();
    string TrafficConfigFile = configPath + "/traffic.csv";
    auto trafficData = ReadTrafficCSV(TrafficConfigFile);
    BooleanValue gFaultEnable;
    g_fault_enable.GetValue(gFaultEnable);
    if (gFaultEnable.Get()) {
        string FaultConfigFile = configPath + "/fault.csv";
        InitFaultMoudle(FaultConfigFile);
    }
    // 遍历Traffic数据，并启动client
    PrintTimestamp("Start Client.");
    for (auto& record : trafficData) {
        auto it = client_map.find(record.sourceNode);
        if (it == client_map.end()) {
            Ptr<UbApp> client = CreateObject<UbApp> ();
            client_map[record.sourceNode] = client;
            ClientTraceConnect(record.sourceNode);

            Ptr<Node> sN = node_map[record.sourceNode];
            sN->AddApplication (client_map[record.sourceNode]);
            appCon.Add(client_map[record.sourceNode]);
        }

        client_map[record.sourceNode]->SetNode(node_map[record.sourceNode]);
        client_map[record.sourceNode]->AddTask(record.taskId, record,
                                               GetDependsToTaskId(record.dependOnPhases, record.sourceNode));
        client_map[record.sourceNode]->GetTpnConn(retConnectionManager.GetConnectionManagerByNode(record.sourceNode));
    }
    appCon.Start(Time(0));

    CheckExampleProcess(client_map);
}

int main(int argc, char* argv[])
{
    if (QueryAttributeInfor(argc, argv))
        return 0;
    // 开始计时
    auto start = std::chrono::high_resolution_clock::now();
    Time::SetResolution(Time::NS);
    // 日志中添加时间前缀
    // ns3::LogComponentEnableAll(ns3::LOG_PREFIX_TIME);

    // 示例：设置指定组件日志级别，设置指定组件打印时间前缀
    // LogComponentEnable("UbApp", LOG_LEVEL_INFO);
    // LogComponentEnable("UbApp", LOG_PREFIX_TIME);

    // 激活日志
    // LogComponentEnable("UbSwitchAllocator", LOG_LEVEL_ALL);
    // LogComponentEnable("UbQueueManager", LOG_LEVEL_ALL);
    // LogComponentEnable("UbCaqm", LOG_LEVEL_ALL);
    // LogComponentEnable("UbApp", LOG_LEVEL_ALL);
    // LogComponentEnable("UbCongestionControl", LOG_LEVEL_ALL);
    // LogComponentEnable("UbController", LOG_LEVEL_ALL);
    // LogComponentEnable("UbDataLink", LOG_LEVEL_ALL);
    // LogComponentEnable("UbFlowControl", LOG_LEVEL_ALL);
    // LogComponentEnable("UbHeader", LOG_LEVEL_ALL);
    // LogComponentEnable("UbLink", LOG_LEVEL_ALL);
    // LogComponentEnable("UbApiLdstThread", LOG_LEVEL_ALL);
    // LogComponentEnable("UbApiLdst", LOG_LEVEL_ALL);
    // LogComponentEnable("UbPort", LOG_LEVEL_ALL);
    // LogComponentEnable("UbRoutingProcess", LOG_LEVEL_ALL);
    // LogComponentEnable("UbSwitch", LOG_LEVEL_ALL);
    // LogComponentEnable("UbFunction", LOG_LEVEL_ALL);
    // LogComponentEnable("UbTransportChannel", LOG_LEVEL_ALL);
    // LogComponentEnable("UbFault", LOG_LEVEL_ALL);
    // LogComponentEnable("UbTransaction", LOG_LEVEL_ALL);

    // 配置文件路径
    string congfigPath = "src/unified-bus/examples/case/urma_clos";
    if (argc > 1) {
        congfigPath = string(argv[1]);
    }

    // 读取配置文件并执行用例
    string runCase = "Run case: " + congfigPath;
    PrintTimestamp(runCase);
    RunCase(congfigPath);

    Simulator::Run();
    Simulator::Destroy();
    PrintTimestamp("Simulator finished.");
    Destroy(appCon);
    ParseTrace();
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    PrintTimestamp("Program finished.");
    string runTime = "程序运行时间: " + to_string(duration.count()) + " 毫秒";
    PrintTimestamp(runTime);
    return 0;
}
