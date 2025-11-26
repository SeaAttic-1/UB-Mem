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
      .SetGroupName("SimpleHBM")
      .AddConstructor<SimpleHBMBank>()
      .AddAttribute("ProcessDelay",
        "Delay (in nanoseconds) to process a request.",
        TimeValue(NanoSeconds(50)),
        MakeTimeAccessor(&SimpleHBMBank::m_processDelay),
        MakeTimeChecker());
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
      uint32_t bus_delay = request.size / HBM_BUS_BANK_BANDWIDTH;
      m_busy = true;
      m_processEvent = Simulator::Schedule(m_processDelay + NanoSeconds(bus_delay),
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

void
SimpleHBMBank::FinishProcessing(SimpleMemoryRequest request)
{
  NS_LOG_INFO("HBM Bank " <<  request.bankId << " on node " << m_nodeId  << " processed request " << request.requestId << " by CU " << request.cuid
              << " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  request.cb(request.arg);
  if (!request_q.empty()) {
    m_busy = true;
    SimpleMemoryRequest next_request = request_q.front();
    request_q.pop();
    uint32_t bus_delay = next_request.size / HBM_BUS_BANK_BANDWIDTH;
    Simulator::Schedule(m_processDelay + NanoSeconds(bus_delay), &SimpleHBMBank::FinishProcessing, this, next_request);
  }
  	
}

void SimpleHBMBank::SetNodeId(uint32_t nodeId) {
  m_nodeId = nodeId;
}

} // namespace ns3
