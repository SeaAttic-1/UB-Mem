#include "ns3/ub-utils.h"
#include <chrono>
#include <iomanip>

using namespace utils;

ApplicationContainer appCon;
static int checkCount = 0;

void CheckExampleProcess(unordered_map<int, Ptr<UbApiUrma>> client_map)
{
    // 使用 \r 让输出在同一行更新
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&nowTime);
    
    // 获取仿真时间（微秒）
    double simTimeUs = Simulator::Now().GetMicroSeconds();
    
    if (checkCount != 0) {
        std::cout << "\r[" << std::put_time(&localTime, "%H:%M:%S") 
              << "]:Check Example Process... (" << ++checkCount 
              << ") | Sim Time: " << std::fixed << std::setprecision(2) << simTimeUs << " us" 
              << std::flush;
    } else {
        ++checkCount;
    }
    
    for (auto it = client_map.begin(); it != client_map.end(); it++) {
        Ptr<UbApiUrma> client = it->second;
        if (!client->IsCompleted()) {
            Simulator::Schedule(MicroSeconds(100), &CheckExampleProcess, client_map);
            return ;
        }
    }
    std::cout << std::endl; // 完成后换行
    Simulator::Stop();
    return ;
}

// 根据配置文件路径执行用例
void RunCase(const string& configPath)
{
    RngSeedManager::SetSeed(10);
    string LoadConfigFilePath = configPath + "/network_attribute.txt";
    SetComponetsAttribute(LoadConfigFilePath);
    CeateTraceDir();
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
            Ptr<UbApiUrma> client = CreateObject<UbApiUrma> ();
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

void PrintHelp()
{
    std::cout << "\n使用方法: ./ns3 run 'scratch/ub-quick-example [选项]'\n\n";
    std::cout << "选项:\n";
    std::cout << "  --case-dir <路径>      指定用例文件夹路径 (默认: scratch/test_CLOS)\n";
    std::cout << "  --trace-dir <路径>     指定ParseTrace使用的输出文件夹 (默认: 与用例文件夹相同)\n";
    std::cout << "  --help                 显示此帮助信息\n";
    std::cout << "  --ClassName <类名>     查询指定类的属性信息\n";
    std::cout << "  --AttributeName <属性> 查询指定属性的详细信息 (需配合--ClassName使用)\n";
    std::cout << "\n示例:\n";
    std::cout << "  ./ns3 run 'scratch/ub-quick-example --case-dir scratch/test_CLOS'\n";
    std::cout << "  ./ns3 run 'scratch/ub-quick-example --case-dir scratch/test_CLOS --trace-dir /tmp/trace'\n";
    std::cout << "  ./ns3 run 'scratch/ub-quick-example --help'\n";
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    // 开始计时
    auto start = std::chrono::high_resolution_clock::now();
    Time::SetResolution(Time::NS);
    
    // 配置文件路径
    string configPath = "scratch/test_CLOS";
    string customTraceDir = "";
    string className = "";
    string attributeName = "";
    
    // 使用 CommandLine 解析参数
    CommandLine cmd;
    cmd.AddValue("case-dir", "指定用例文件夹路径", configPath);
    cmd.AddValue("trace-dir", "指定ParseTrace使用的输出文件夹", customTraceDir);
    cmd.AddValue("ClassName", "查询指定类的属性信息", className);
    cmd.AddValue("AttributeName", "查询指定属性的详细信息", attributeName);
    cmd.Parse(argc, argv);
    
    // 处理帮助信息
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintHelp();
            return 0;
        }
    }
    
    // 处理属性查询
    if (!className.empty()) {
        TypeId tid;
        try {
            tid = TypeId::LookupByName(className);
        } catch (const std::exception& e) {
            NS_LOG_UNCOND("错误: 找不到类 '" << className << "'");
            return 1;
        }
        
        if (!attributeName.empty()) {
            struct TypeId::AttributeInformation info;
            if (tid.LookupAttributeByName(attributeName, &info)) {
                NS_LOG_UNCOND("Attribute: " << info.name << "\n"
                            << "Description: " << info.help << "\n"
                            << "DataType: " << info.checker->GetValueTypeName() << "\n"
                            << "Default: " << info.initialValue->SerializeToString(info.checker));
            } else {
                NS_LOG_UNCOND("错误: 在类 '" << className << "' 中找不到属性 '" << attributeName << "'");
            }
        } else {
            NS_LOG_UNCOND("类 '" << className << "' 的所有属性:\n");
            for (uint32_t i = 0; i < tid.GetAttributeN(); ++i) {
                TypeId::AttributeInformation info = tid.GetAttribute(i);
                NS_LOG_UNCOND("Attribute: " << info.name << "\n"
                            << "Description: " << info.help << "\n"
                            << "DataType: " << info.checker->GetValueTypeName() << "\n"
                            << "Default: " << info.initialValue->SerializeToString(info.checker) << "\n");
            }
        }
        return 0;
    }
    
    // 日志中添加时间前缀
    // ns3::LogComponentEnableAll(ns3::LOG_PREFIX_TIME);

    // 示例：设置指定组件日志级别，设置指定组件打印时间前缀
    // LogComponentEnable("UbApiUrma", LOG_LEVEL_INFO);
    // LogComponentEnable("UbApiUrma", LOG_PREFIX_TIME);

    // 激活日志
    // LogComponentEnable("UbSwitchAllocator", LOG_LEVEL_ALL);
    // LogComponentEnable("UbQueueManager", LOG_LEVEL_ALL);
    // LogComponentEnable("UbCaqm", LOG_LEVEL_ALL);
    // LogComponentEnable("UbApiUrma", LOG_LEVEL_ALL);
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

    // 读取配置文件并执行用例
    string runCase = "Run case: " + configPath;
    PrintTimestamp(runCase);
    RunCase(configPath);
    
    PrintTimestamp("Simulator Start!");
    Simulator::Run();
    Simulator::Destroy();
    PrintTimestamp("Simulator finished!");
    Destroy(appCon);
    ParseTrace(customTraceDir);
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    PrintTimestamp("Program finished.");
    string runTime = "程序运行时间: " + to_string(duration.count()) + " 毫秒";
    PrintTimestamp(runTime);
    return 0;
}

