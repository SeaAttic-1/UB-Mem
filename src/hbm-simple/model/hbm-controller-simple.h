#ifndef HBM_CONTROLLER_SIMPLE_H
#define HBM_CONTROLLER_SIMPLE_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <vector>
#include "hbm-bank-simple.h"

namespace ns3 {

// It is worth noting that this hbm implementation calculates the latency per request in a unique way:
// It groups all the latency into a whole called "access latency"
// The access latency is calculated by dividing the request size by the HBM bandwidth
// The HBM bandwidth is not a fixed value. It depends on the network traffic plus the background traffic intensity.
// Do note that this is not quite right, because bandwidth has nothing to do with latency
// Therefore, the bandwidth must be further corrected, i.e., multiplied with a coefficient, to give more realistic latency number

class SimpleHBMController : public Object
{
public:
  static TypeId GetTypeId(void);

  SimpleHBMController();
  virtual ~SimpleHBMController();

  void InitializeBanks(uint32_t nodeId, uint32_t numBanks);
  void SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, uint32_t bankId, bool isWrite, Callback<void, void*> cb, void* arg);
  void SetBackgroundIntensity(uint32_t bg_intensity);


private:
  std::vector<Ptr<SimpleHBMBank>> m_banks;
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
