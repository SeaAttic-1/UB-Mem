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

  controller->SendRequest(1);
  controller->SendRequest(2);
  controller->SendRequest(3);

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
