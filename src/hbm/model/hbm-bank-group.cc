#include "hbm-bank-group.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMBankGroup");
NS_OBJECT_ENSURE_REGISTERED(HBMBankGroup);

TypeId HBMBankGroup::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMBankGroup")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMBankGroup>();

  return tid;
}

HBMBankGroup::HBMBankGroup()
{
  NS_LOG_FUNCTION(this);
}

HBMBankGroup::~HBMBankGroup()
{
  NS_LOG_FUNCTION(this);
}

void HBMBankGroup::Initialize(uint32_t nodeId, uint32_t groupId, uint32_t numBanks)
{
    m_groupId = groupId;
    m_nodeId = nodeId;

    for(uint32_t i = 0; i < numBanks; i++) {
        Ptr<HBMBank> new_bank = CreateObject<HBMBank>();
        new_bank->Initialize(nodeId, i);
        m_banks.push_back(new_bank);
    }
}

void HBMBankGroup::SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, bool isWrite, Callback<void, void*> cb, void* arg) {
    MemoryRequest request = {cuid, requestId, address, size, isWrite, cb, arg};
    uint32_t bankId = EXTRACT_BANK(address);
    this->m_banks[bankId]->ReceiveRequest(request);
}

} // namespace ns3
