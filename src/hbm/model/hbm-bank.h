#ifndef HBM_BANK_H
#define HBM_BANK_H

#define HBM_BANK_ATOMIC_SIZE 32
#define HBM_BANK_PER_DIE 8
#define HBM_BUS_BANDWIDTH_BITS 256 * 1000 * 1000 * 1000
#define HBM_BUS_BANDWIDTH HBM_BUS_BANDWIDTH_BITS / 8
#define HBM_BUS_BANK_BANDWIDTH HBM_BUS_BANDWIDTH / HBM_BANK_PER_DIE
// Usually bus transfer takes less than 1 nanoseconds, so doesn't quite matter.
// The major latencies are brought by mem row access 
// The macros are still defined if you need to tweak it though

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/callback.h"
#include <queue>

namespace ns3 {

struct MemoryRequest {
    uint64_t address;  // Memory address for the request
    uint32_t size;     // Size of the request (in bytes)  
    uint32_t bankId;   // The bank that the request is intended for
    bool isWrite;// Whether it's a write request or a read request
    uint32_t requestId; // An unused field
    Callback<void, void*> cb; // Callback function used to notify the receiver
    void* arg; // argument for the Callback func
};

class HBMBank : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMBank();
  virtual ~HBMBank();

  void ReceiveRequest(MemoryRequest request);
  void ProcessNext();

private:
  std::queue <MemoryRequest> request_q;
  bool m_busy;
  EventId m_processEvent;
  Time m_processDelay;

  void FinishProcessing(MemoryRequest request);
};

} // namespace ns3

#endif // HBM_BANK_H
