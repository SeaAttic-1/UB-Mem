#ifndef HBM_STACK_H
#define HBM_STACK_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-pseudo-channel.h"
#include "hbm-macro.h"

namespace ns3 {

class HBMPseudoChannel;
struct MemoryRequest;

class HBMStack : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMStack();
  virtual ~HBMStack();

  void Initialize(uint32_t nodeId, uint32_t stackId, uint32_t numChannels = HBM_PC_PER_STACK);
  bool SendRequest(MemoryRequest request);

private:
  std::vector<Ptr<HBMPseudoChannel>> m_channels;
  uint32_t m_nodeId;
  uint32_t m_stackId;
};

} // namespace ns3

#endif

