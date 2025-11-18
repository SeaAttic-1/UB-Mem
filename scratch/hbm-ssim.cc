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
  Ptr<HBMController> controller = helper.Create(4);

  controller->SendRequest(1, 0x1000, 128, 1, true, [](){});
  controller->SendRequest(2, 0x1000, 256, 1, false, [](){});
  controller->SendRequest(3, 0x2000, 64, 2, true, [](){});

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
