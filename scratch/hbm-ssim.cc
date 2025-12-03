#include "ns3/core-module.h"
#include "ns3/hbm-helper.h"
#include "ns3/hbm-controller.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HBMSsimExample");

int
main(int argc, char** argv)
{
  LogComponentEnable("HBMController", LOG_LEVEL_INFO);
  LogComponentEnable("HBMBank", LOG_LEVEL_INFO);

  HBMHelper helper;
  Ptr<HBMController> controller = helper.Create(0);

  controller->SendRequest(0x1000, 128, true, false, [](void* p){}, nullptr);
  controller->SendRequest(0x1000, 256, false, false, [](void* p){}, nullptr);
  controller->SendRequest(0x2000, 64, true, false, [](void* p){}, nullptr);

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
