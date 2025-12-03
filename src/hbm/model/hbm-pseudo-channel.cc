#include "hbm-pseudo-channel.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMPseudoChannel");
NS_OBJECT_ENSURE_REGISTERED(HBMPseudoChannel);

TypeId HBMPseudoChannel::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMPseudoChannel")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMPseudoChannel>();

  return tid;
}

HBMPseudoChannel::HBMPseudoChannel()
{
  NS_LOG_FUNCTION(this);
}

HBMPseudoChannel::~HBMPseudoChannel()
{
  NS_LOG_FUNCTION(this);
}

void HBMPseudoChannel::Initialize(uint32_t nodeId, uint32_t channelId, uint32_t numGroups)
{
    m_channelId = channelId;
    m_nodeId = nodeId;

    for(uint32_t i = 0; i < numGroups; i++) {
        Ptr<HBMBankGroup> new_bank_group = CreateObject<HBMBankGroup>();
        new_bank_group->Initialize(nodeId, i);
        m_bank_groups.push_back(new_bank_group);
    }
}

void HBMPseudoChannel::SendRequest(MemoryRequest request) {
    uint32_t groupId = EXTRACT_BANK_GROUP(request.address);
    this->m_bank_groups[groupId]->SendRequest(request);
}

} // namespace ns3
