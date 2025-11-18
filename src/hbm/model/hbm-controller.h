#ifndef HBM_CONTROLLER_H
#define HBM_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>

namespace ns3 {

class HBMBank;

class HBMController : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMController();
  virtual ~HBMController();

  void InitializeBanks(uint32_t numBanks);
  void SendRequest(uint32_t requestId, uint64_t address, uint32_t size, uint32_t bankId, bool isWrite, Callback<void, void*> cb, void* arg);

private:
  std::vector<Ptr<HBMBank>> m_banks;
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
