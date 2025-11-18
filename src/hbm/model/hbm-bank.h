#ifndef HBM_BANK_H
#define HBM_BANK_H

#define HBM_BANK_ATOMIC_SIZE 64

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include <queue>

namespace ns3 {

struct MemoryRequest {
    uint64_t address;  // Memory address for the request
    uint32_t size;     // Size of the request (in bytes)  
    uint32_t bankId;   // The bank that the request is intended for
    bool isWrite;// Whether it's a write request or a read request
    uint32_t requestId;
};

class HBMBank : public Object
{
public:
  static TypeId GetTypeId(void);

  HBMBank();
  virtual ~HBMBank();

  void ReceiveRequest(MemoryRequest request, Callback<void> cb);
  void ProcessNext();

private:
  std::queue <MemoryRequest> request_q;
  bool m_busy;
  EventId m_processEvent;
  Time m_processDelay;

  void FinishProcessing(MemoryRequest request, Callback<void> cb);
};

} // namespace ns3

#endif // HBM_BANK_H
