#ifndef HBM_STACK_H
#define HBM_STACK_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-pseudo-channel.h"
#include "hbm-macro.h"

namespace ns3 {

class HBMPseudoChannel;

class HBMStack : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMStack();
  virtual ~HBMStack();

  void Initialize(uint32_t nodeId, uint32_t stackId, uint32_t numChannels = HBM_PC_PER_STACK);
  void SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, bool isWrite, Callback<void, void*> cb, void* arg);

private:
  std::vector<Ptr<HBMPseudoChannel>> m_channels;
  uint32_t m_nodeId;
  uint32_t m_stackId;
};

} // namespace ns3

#endif

