#ifndef HBM_CONTROLLER_SIMPLE_H
#define HBM_CONTROLLER_SIMPLE_H

#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/ptr.h"
#include <map>
#include <queue>

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

  void SendRequest(uint32_t size, Callback<void, void*> cb, void* arg);
  void SetBackgroundIntensity(float bg_intensity);
  void DoCallBack(Callback<void, void*> cb, void* arg);


private:

  struct MemoryRequest {
    uint32_t size;
    Callback<void, void*> cb;
    void* arg;
  };


  float m_bandwidth = 400.0; // 400 Gbps;
  const float m_nominal_bandwidth = 328.22; // 128 pkt size bandwidth, PS time resolution;
  bool m_busy = false;

  std::map<float, float> m_lut = {{0.0, 1.0}, {1.0, 1.0 / 1.333}, {2.0, 1.0 / 1.36}, {4.0 ,  1.0 / 1.401}, {8.0, 1.0 / 1.556}, {16.0,  1.0 / 1.999}};
  std::queue<MemoryRequest> m_requests;
};

} // namespace ns3

#endif // HBM_CONTROLLER_H
