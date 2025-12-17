#include "hbm-helper-simple.h"
#include "ns3/hbm-controller-simple.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleHBMHelper");

Ptr<SimpleHBMController>
SimpleHBMHelper::Create(uint32_t nodeId)
{
  Ptr<SimpleHBMController> ctrl = CreateObject<SimpleHBMController>();
  return ctrl;
}
} // namespace ns3
