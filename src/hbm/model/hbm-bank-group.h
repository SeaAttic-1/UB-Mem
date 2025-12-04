#ifndef HBM_BANK_GROUP_H
#define HBM_BANK_GROUP_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-pseudo-channel.h"
#include "hbm-macro.h"

namespace ns3 {

class HBMBank;
class HBMPseudoChannel;
struct MemoryRequest;

class HBMBankGroup : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMBankGroup();
  virtual ~HBMBankGroup();

  void Initialize(uint32_t nodeId, uint32_t groupId, HBMPseudoChannel* pc_ptr, uint32_t numBanks = HBM_BANK_PER_BANK_GROUP);
  bool SendRequest(MemoryRequest request);

private:
  std::vector<Ptr<HBMBank>> m_banks;
  uint32_t m_nodeId;
  uint32_t m_groupId;
};

} // namespace ns3

#endif