// SPDX-License-Identifier: GPL-2.0-only
#include "ns3/ub-utils.h"
#include <chrono>
#include "ns3/test.h"
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace utils;
using namespace ns3;

/**
* @brief unified-bus test class
*/
class ubTest : public TestCase
{
public:
    ubTest (string testPath);
    /**
     * @brief Run the test
     */
    void DoRun() override;

private:
    void CheckExampleProcess(unordered_map<int, Ptr<UbApp>> client_map);

    void RunCase(const string& configPath);

    void IsPathExist(const std::string &path);

    void VerifyGenFile(const string& configPath);

    string testPath;

    void DoTeardown() override;
    virtual void DoSetUp (void);
    bool compareCSVFiles(const std::string& file1, const std::string& file2);

    ApplicationContainer appCon;
};

void ubTest::DoSetUp (void)
{
    // 重置全局状态
    Config::Reset ();
}

void ubTest::DoTeardown()
{
    Simulator::Destroy();
}

ubTest::ubTest(string testPath)
    : TestCase("ubTest"),
      testPath(testPath)
{
}

void ubTest::VerifyGenFile(const string& configPath)
{
    // 验证生成的task_statistics.csv文件
    string taskFile1 = configPath + "/output/task_statistics.csv";
    string taskFile2 = configPath + "/test/task_statistics.csv";
    // 逐行读取并比较
    NS_TEST_ASSERT_MSG_EQ(compareCSVFiles(taskFile1, taskFile2), true, "task_statistics.csv not same.");

    // 验证生成的throughput.csv文件
    string throughputFile1 = configPath + "/output/throughput.csv";
    string throughputFile2 = configPath + "/test/throughput.csv";
    // 逐行读取并比较
    NS_TEST_ASSERT_MSG_EQ(compareCSVFiles(throughputFile1, throughputFile2), true, "throughput.csv not same.");
}

bool ubTest::compareCSVFiles(const std::string& file1, const std::string& file2)
{
    std::ifstream file1Stream(file1);
    std::ifstream file2Stream(file2);

    if (!file1Stream.is_open() || !file2Stream.is_open()) {
        return false;
    }

    std::string line1;
    std::string line2;
    while (std::getline(file1Stream, line1) && std::getline(file2Stream, line2)) {
        if (line1 != line2) {
            return false;
        }
        line1.clear();
        line2.clear();
    }

    if (line1 != line2) {
        return false;
    }

    // 检查是否有文件还有剩余行
    if (file1Stream.peek() != EOF || file2Stream.peek() != EOF) {
        return false;
    }

    return true;
}

void ubTest::DoRun (void)
{
    string runCase = "Run case: " + testPath;
    PrintTimestamp(runCase);
    // 读取配置文件并执行用例
    RunCase(testPath);

    Simulator::Run();
    // Simulator::Destroy();

    Destroy(appCon);
    ParseTrace(true);
    VerifyGenFile(testPath);
}

void ubTest::IsPathExist(const std::string &path)
{
    NS_TEST_ASSERT_MSG_EQ(std::filesystem::exists(path), true, "test path not exist");
}

void ubTest::CheckExampleProcess(unordered_map<int, Ptr<UbApp>> client_map)
{
    PrintTimestamp("Check Example Process.");
    for (auto it = client_map.begin(); it != client_map.end(); it++) {
        Ptr<UbApp> client = it->second;
        if (!client->IsCompleted()) {
            Simulator::Schedule(MicroSeconds(10), &ubTest::CheckExampleProcess, this, client_map);
            return ;
        }
    }
    Simulator::Stop();
    return ;
}

// 根据配置文件路径执行用例
void ubTest::RunCase(const string& configPath)
{
    IsPathExist(configPath);
    RngSeedManager::SetSeed(10);
    string LoadConfigFilePath = configPath + "/network_attribute.txt";
    SetComponentsAttribute(LoadConfigFilePath);
    CreateTraceDir();
    string NodeConfigFile = configPath + "/node.csv";
    CreateNode(NodeConfigFile);
    string TopoConfigFile = configPath + "/topology.csv";
    CreateTopo(TopoConfigFile);
    string RouterConfigFile = configPath + "/router_table.csv";
    AddRoutingTable(RouterConfigFile);
    string TpConfigFile = configPath + "/transport_channel.csv";
    TpConnectionManager retConnectionManager = CreateTp(TpConfigFile);
    TopoTraceConnect();
    string TrafficConfigFile = configPath + "/traffic.csv";
    auto trafficData = ReadTrafficCSV(TrafficConfigFile);
    // ApplicationContainer appCon;
    // 遍历Traffic数据，并启动client
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

/**
* @brief unified-bus testsuite class
*/
class ubTestSuite : public TestSuite
{
  public:
    /**
     * @brief Constructor
     */
    ubTestSuite();
};

ubTestSuite::ubTestSuite()
    : TestSuite("ubTest", Type::UNIT)
{
    AddTestCase(new ubTest("scratch/test_CLOS"), TestCase::Duration::QUICK);
    AddTestCase(new ubTest("scratch/test_CCFC"), TestCase::Duration::QUICK);
}

// 注册测试用例
static ubTestSuite ubTestSuite;