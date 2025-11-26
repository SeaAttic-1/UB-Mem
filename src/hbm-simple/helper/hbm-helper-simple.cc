#include "hbm-helper-simple.h"
#include "ns3/hbm-controller-simple.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleHBMHelper");

SimpleHBMHelper::SimpleHBMHelper()
  : m_defaultBanks(8)
{
}

Ptr<SimpleHBMController>
SimpleHBMHelper::Create(uint32_t nodeId, uint32_t numBanks)
{
  Ptr<SimpleHBMController> ctrl = CreateObject<SimpleHBMController>();
  ctrl->InitializeBanks(nodeId, numBanks);
  return ctrl;
}

} // namespace ns3
