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
HBMBank::ReceiveRequest(MemoryRequest request, Callback<void> cb)
{
  NS_LOG_FUNCTION(this << request.requestId);

  if (!m_busy)
    {
      m_busy = true;
      m_processEvent = Simulator::Schedule(m_processDelay,
                                           &HBMBank::FinishProcessing,
                                           this, request, cb);
    }
  else
    {
      request_q.push(request);
      NS_LOG_INFO("Request " << request.requestId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
    }
}

void
HBMBank::FinishProcessing(MemoryRequest request, Callback<void> cb)
{
  NS_LOG_INFO("HBM Bank processed request " << request.requestId
              << " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  cb();
  if (!request_q.empty()) {
    MemoryRequest next_request = request_q.front();
    request_q.pop();
    Simulator::Schedule(m_processDelay, &HBMBank::FinishProcessing, this, next_request, cb);
  }
  	
}

} // namespace ns3
