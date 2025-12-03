#ifndef HBM_PSEUDO_CHANNEL_H
#define HBM_PSEUDO_CHANNEL_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "hbm-bank-group.h"
#include <vector>
#include "hbm-macro.h"

namespace ns3 {

class HBMPseudoChannel;

class HBMPseudoChannel : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMPseudoChannel();
  virtual ~HBMPseudoChannel();

  void Initialize(uint32_t nodeId, uint32_t channelId, uint32_t numGroups = HBM_BANK_GROUP_PER_PC);
  void SendRequest(MemoryRequest request);

private:
  std::vector<Ptr<HBMBankGroup>> m_bank_groups;
  uint32_t m_nodeId;
  uint32_t m_channelId;
};

} // namespace ns3

#endif

