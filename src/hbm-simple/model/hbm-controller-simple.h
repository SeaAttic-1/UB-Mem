#ifndef HBM_CONTROLLER_H
#define HBM_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>

namespace ns3 {

class SimpleHBMBank;

class SimpleHBMController : public Object
{
public:
  static TypeId GetTypeId(void);

  SimpleHBMController();
  virtual ~SimpleHBMController();

  void InitializeBanks(uint32_t nodeId, uint32_t numBanks);
  void SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, uint32_t bankId, bool isWrite, Callback<void, void*> cb, void* arg);

private:
  std::vector<Ptr<SimpleHBMBank>> m_banks;
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
