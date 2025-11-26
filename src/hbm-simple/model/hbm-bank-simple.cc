#include "hbm-bank-simple.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleHBMBank");
NS_OBJECT_ENSURE_REGISTERED(SimpleHBMBank);

TypeId SimpleHBMBank::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::SimpleHBMBank")
      .SetParent<Object>()
      .SetGroupName("Simple-HBM")
      .AddConstructor<SimpleHBMBank>()
      .AddAttribute("NominalBandwidth",
        "Nominal Bandwidth (in GB/s) ",
        UintegerValue(HBM_BUS_BANK_BANDWIDTH),
        MakeUintegerAccessor(&SimpleHBMBank::m_nominal_bandwidth),
        MakeUintegerChecker<uint32_t>())
      .AddAttribute("CorrectCoefficient",
      "Coefficient used to modify latency",
      UintegerValue(100),
      MakeUintegerAccessor(&SimpleHBMBank::m_correction_coeff),
      MakeUintegerChecker<uint32_t>());
      
  return tid;
}

SimpleHBMBank::SimpleHBMBank()
  : m_busy(false)
{
  NS_LOG_FUNCTION(this);
}

SimpleHBMBank::~SimpleHBMBank()
{
  NS_LOG_FUNCTION(this);
}

void
SimpleHBMBank::ReceiveRequest(SimpleMemoryRequest request)
{
  NS_LOG_FUNCTION(this << request.requestId);

  if (!m_busy)
    {
      m_busy = true;
      m_processEvent = Simulator::Schedule(NanoSeconds(CalculateLatency(request.size)),
                                           &SimpleHBMBank::FinishProcessing,
                                           this, request);
    }
  else
    {
      request_q.push(request);
      if (request.cuid == 12345)
        NS_LOG_INFO("Remote request " << request.requestId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
      else
        NS_LOG_INFO("Request " << request.requestId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
      NS_LOG_INFO("Congestion at Bank " << request.bankId << ", Queue length " << request_q.size() );
    }
}

void SimpleHBMBank::FinishProcessing(SimpleMemoryRequest request)
{
  NS_LOG_INFO("HBM Bank " <<  request.bankId << " on node " << m_nodeId  << " processed request " << request.requestId << " by CU " << request.cuid
              << " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  request.cb(request.arg);
  if (!request_q.empty()) {
    m_busy = true;
    SimpleMemoryRequest next_request = request_q.front();
    request_q.pop();

    Simulator::Schedule(NanoSeconds(CalculateLatency(next_request.size)), &SimpleHBMBank::FinishProcessing, this, next_request);
  }
  	
}

void SimpleHBMBank::SetNodeId(uint32_t nodeId) {
  m_nodeId = nodeId;
}

uint32_t SimpleHBMBank::CalculateLatency(uint32_t length) {
  // Calculate the real HBM traffic, then return the latency to process a request of "length" bytes 

  // First, get the total intensity, here a simple addition is used; 1 is there so no div by 0
  uint32_t total_intensity = m_bg_intensity + request_q.size() + 1;
  // Then, correct the bandwidth, here a division is used
  uint32_t corrected_bandwidth = m_nominal_bandwidth / total_intensity; 
  // Finally, calculate the latency. A correction coefficient is used to get realistic values
  uint32_t latency = length * m_correction_coeff / corrected_bandwidth;
  // Always use at least 1 nanoseconbd
  // Generally expect the value to be in [20, 80] ns
  return latency == 0 ? 1 : latency;
}

void SimpleHBMBank::SetBackgroundIntensity(uint32_t bg_intensity) {
  m_bg_intensity = bg_intensity;
}

} // namespace ns3
