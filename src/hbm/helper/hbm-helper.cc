#include "hbm-helper.h"
#include "ns3/hbm-controller.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMHelper");

HBMHelper::HBMHelper()
  : m_defaultBanks(8)
{
}

Ptr<HBMController>
HBMHelper::Create(uint32_t nodeId)
{
  Ptr<HBMController> ctrl = CreateObject<HBMController>();
  ctrl->Initialize(nodeId);
  return ctrl;
}

} // namespace ns3
