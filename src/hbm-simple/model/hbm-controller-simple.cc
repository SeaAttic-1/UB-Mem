#include "hbm-controller-simple.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/callback.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleHBMController");
NS_OBJECT_ENSURE_REGISTERED(SimpleHBMController);

TypeId SimpleHBMController::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::SimpleHBMController")
      .SetParent<Object>()
      .SetGroupName("Simple-HBM")
      .AddConstructor<SimpleHBMController>();
  return tid;
}

SimpleHBMController::SimpleHBMController()
{
  NS_LOG_FUNCTION(this);
}

SimpleHBMController::~SimpleHBMController()
{
  NS_LOG_FUNCTION(this);
}

void SimpleHBMController::SendRequest(uint32_t size, Callback<void, void*> cb, void* arg)
{

  if (!m_busy) {
    m_busy = true;
    float delay = size / m_bandwidth * 8000.0;
    Simulator::Schedule(PicoSeconds(delay), &SimpleHBMController::DoCallBack, this, cb, arg);
  }

  else {
    m_requests.push({size, cb, arg});
  }

}

void SimpleHBMController::SetBackgroundIntensity(float bg_intensity) {

  for(auto& i : m_lut) {
    if(i.first == bg_intensity) {
      m_bandwidth = m_nominal_bandwidth * i.second;
      NS_LOG_INFO("bandwidth is " << m_bandwidth);
      NS_LOG_INFO("delay is " << 128.0 * 8.0 / m_bandwidth);
    }
  }
}

void SimpleHBMController::DoCallBack(Callback<void, void*> cb, void* arg) {
  cb(arg);
  m_busy = false;

  if(!m_requests.empty()) {
    m_busy = true;
    MemoryRequest new_request = m_requests.front();
    m_requests.pop();

    float delay = new_request.size / m_bandwidth * 8000.0;
    Simulator::Schedule(PicoSeconds(delay), &SimpleHBMController::DoCallBack, this, new_request.cb, new_request.arg);
  }

}

}
 // namespace ns3
