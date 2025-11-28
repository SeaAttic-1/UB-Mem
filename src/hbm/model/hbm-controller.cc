#include "hbm-controller.h"
#include "hbm-bank.h"
#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMController");
NS_OBJECT_ENSURE_REGISTERED(HBMController);

TypeId HBMController::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMController")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMController>();
  return tid;
}

HBMController::HBMController()
{
  NS_LOG_FUNCTION(this);
}

HBMController::~HBMController()
{
  NS_LOG_FUNCTION(this);
}

void
HBMController::Initialize(uint32_t nodeId, uint32_t numStacks)
{
  for(uint32_t i = 0; i < numStacks; i++) {
    Ptr<HBMStack> new_stack = CreateObject<HBMStack>();
    new_stack->Initialize(nodeId, i);
    m_stacks.push_back(new_stack);
  }
}

void HBMController::SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, bool isWrite, Callback<void, void*> cb, void* arg)
{
  NS_LOG_FUNCTION(this << requestId);

  uint32_t stackId = EXTRACT_STACK(address);
  m_stacks[stackId]->SendRequest(cuid, requestId, address, size, isWrite, cb, arg);
}

} // namespace ns3
