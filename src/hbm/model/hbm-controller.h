#ifndef HBM_CONTROLLER_H
#define HBM_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-stack.h"
#include "hbm-macro.h"

namespace ns3 {

struct MemoryRequest;
class HBMStack;

class HBMController : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMController();
  virtual ~HBMController();

  void Initialize(uint32_t nodeId, uint32_t numStacks = HBM_STACK_COUNT);
  bool SendRequest(uint64_t address, uint32_t size, bool isWrite, bool isRemote, Callback<void, void*> cb, void* arg);
  void NotifyComplete(void);


private:
  std::vector<Ptr<HBMStack>> m_stacks;
  std::vector<MemoryRequest> m_write_buffer;

  uint32_t m_nodeId;
  uint32_t m_outstanding = 0;

  std::vector<Callback<void, void*>> m_notify_callbacks;

  bool EnqueueWrite(uint64_t address, uint32_t size, bool isRemote, Callback<void, void*> cb, void* arg);
  bool TryCoalesce(uint64_t address, uint32_t size, bool isRemote, Callback<void, void*> cb, void* arg);
  void FlushWriteBuffer();
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
