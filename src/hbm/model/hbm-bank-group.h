#ifndef HBM_BANK_GROUP_H
#define HBM_BANK_GROUP_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-bank.h"
#include "hbm-macro.h"

namespace ns3 {

class HBMBank;

class HBMBankGroup : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMBankGroup();
  virtual ~HBMBankGroup();

  void Initialize(uint32_t nodeId, uint32_t groupId, uint32_t numBanks = HBM_BANK_PER_BANK_GROUP);
  void SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, bool isWrite, Callback<void, void*> cb, void* arg);

private:
  std::vector<Ptr<HBMBank>> m_banks;
  uint32_t m_nodeId;
  uint32_t m_groupId;
};

} // namespace ns3

#endif