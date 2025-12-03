#ifndef HBM_BANK_H
#define HBM_BANK_H
// Usually bus transfer takes less than 1 nanoseconds, so doesn't quite matter.
// The major latencies are brought by mem row access 
// The macros are still defined if you need to tweak it though

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/ptr.h"
#include <queue>

namespace ns3 {

struct MemoryRequest {
    uint64_t address;  // Memory address for the request
    uint32_t size;     // Size of the request (in bytes)  
    bool isWrite;// Whether it's a write request or a read request
    bool isRemote;
    std::vector<Callback<void, void*>> cbs; // Callback function used to notify the receiver
    std::vector<void*> args; // argument for the Callback func
};

class HBMBank : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMBank();
  virtual ~HBMBank();

  void ReceiveRequest(MemoryRequest request);
  void ProcessNext();
  void Initialize(uint32_t nodeId, uint32_t bankId);

private:
  std::queue <MemoryRequest> request_q;
  bool m_busy;
  EventId m_processEvent;

  uint32_t m_nodeId;
  uint32_t m_bankId;
  uint32_t m_activeRow = 32768;

  void FinishProcessing(MemoryRequest request);
};

} // namespace ns3

#endif // HBM_BANK_H
