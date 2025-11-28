#ifndef HBM_CONTROLLER_H
#define HBM_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-stack.h"
#include "hbm-macro.h"

namespace ns3 {

class HBMStack;

class HBMController : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMController();
  virtual ~HBMController();

  void Initialize(uint32_t nodeId, uint32_t numStacks = HBM_STACK_COUNT);
  void SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, bool isWrite, Callback<void, void*> cb, void* arg);

private:
  std::vector<Ptr<HBMStack>> m_stacks;
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
