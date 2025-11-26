#ifndef HBM_BANK_SIMPLE_H
#define HBM_BANK_SIMPLE_H

#define HBM_BANK_ATOMIC_SIZE 64
#define HBM_BANK_PER_DIE 8
#define HBM_BUS_BANDWIDTH 2048 // in GB/s or B/ns, 2048 given by the latest HBM4 standard
#define HBM_BUS_BANK_BANDWIDTH HBM_BUS_BANDWIDTH / HBM_BANK_PER_DIE

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

struct SimpleMemoryRequest {
    uint32_t cuid;     // The CU that initiates this request
    uint64_t address;  // Memory address for the request
    uint32_t size;     // Size of the request (in bytes)  
    uint32_t bankId;   // The bank that the request is intended for
    bool isWrite;// Whether it's a write request or a read request
    uint32_t requestId; // An unused field
    Callback<void, void*> cb; // Callback function used to notify the receiver
    void* arg; // argument for the Callback func
};

class SimpleHBMBank : public Object
{
public:
  static TypeId GetTypeId(void);

  SimpleHBMBank();
  virtual ~SimpleHBMBank();

  void ReceiveRequest(SimpleMemoryRequest request);
  void SetNodeId(uint32_t nodeId);
  void SetBackgroundIntensity(uint32_t bg_intensity);

private:
  std::queue <SimpleMemoryRequest> request_q;
  uint32_t m_nominal_bandwidth = HBM_BUS_BANK_BANDWIDTH;
  uint32_t m_bg_intensity = 0; // Intra-node traffic
  uint32_t m_correction_coeff = 1; // Used for correction purpose
  EventId m_processEvent;
  bool m_busy = false;

  uint32_t m_nodeId;

  uint32_t CalculateLatency(uint32_t length);
  void FinishProcessing(SimpleMemoryRequest request);
};

} // namespace ns3

#endif // HBM_BANK_H
