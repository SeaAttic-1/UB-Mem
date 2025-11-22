#include "hbm-bank.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMBank");
NS_OBJECT_ENSURE_REGISTERED(HBMBank);

TypeId HBMBank::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMBank")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMBank>()
      .AddAttribute("ProcessDelay",
        "Delay (in nanoseconds) to process a request.",
        TimeValue(NanoSeconds(50)),
        MakeTimeAccessor(&HBMBank::m_processDelay),
        MakeTimeChecker());
  return tid;
}

HBMBank::HBMBank()
  : m_busy(false)
{
  NS_LOG_FUNCTION(this);
}

HBMBank::~HBMBank()
{
  NS_LOG_FUNCTION(this);
}

void
HBMBank::ReceiveRequest(MemoryRequest request)
{
  NS_LOG_FUNCTION(this << request.requestId);

  if (!m_busy)
    {
      uint32_t bus_delay = request.size / HBM_BUS_BANK_BANDWIDTH;
      m_busy = true;
      m_processEvent = Simulator::Schedule(m_processDelay + bus_delay,
                                           &HBMBank::FinishProcessing,
                                           this, request);
    }
  else
    {
      request_q.push(request);
      NS_LOG_INFO("Request " << request.requestId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
      NS_LOG_INFO("Congestion at Bank " << request.bankId << ", Queue length " << request_q.size() );
    }
}

void
HBMBank::FinishProcessing(MemoryRequest request)
{
  NS_LOG_INFO("HBM Bank " << request.bankId << " processed request " << request.requestId
              << " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  request.cb(request.arg);
  if (!request_q.empty()) {
    MemoryRequest next_request = request_q.front();
    request_q.pop();
    uint32_t bus_delay = request.size / HBM_BUS_BANK_BANDWIDTH;
    Simulator::Schedule(m_processDelay + bus_delay, &HBMBank::FinishProcessing, this, next_request);
  }
  	
}

} // namespace ns3
