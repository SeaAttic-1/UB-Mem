#include "hbm-stack.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"
#include "hbm-bank.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMStack");
NS_OBJECT_ENSURE_REGISTERED(HBMStack);

TypeId HBMStack::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMStack")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMStack>();

  return tid;
}

HBMStack::HBMStack()
{
  NS_LOG_FUNCTION(this);
}

HBMStack::~HBMStack()
{
  NS_LOG_FUNCTION(this);
}

void HBMStack::Initialize(uint32_t nodeId, uint32_t stackId, uint32_t numChannels)
{
    m_stackId = stackId;
    m_nodeId = nodeId;

    for(uint32_t i = 0; i < numChannels; i++) {
        Ptr<HBMPseudoChannel> new_channel = CreateObject<HBMPseudoChannel>();
        new_channel->Initialize(nodeId, i);
        m_channels.push_back(new_channel);
    }
}

bool HBMStack::SendRequest(MemoryRequest request) {
    uint32_t channelId = EXTRACT_PC(request.address);
    return this->m_channels[channelId]->SendRequest(request);
}

} // namespace ns3
