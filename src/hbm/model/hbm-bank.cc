#include "hbm-bank.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"
#include "hbm-macro.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMBank");
NS_OBJECT_ENSURE_REGISTERED(HBMBank);

TypeId HBMBank::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMBank")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMBank>();
  return tid;
}

HBMBank::HBMBank(): m_busy(false), m_activeRow(HBM_ROW_PER_BANK)
{
  NS_LOG_FUNCTION(this);
}

HBMBank::~HBMBank()
{
  NS_LOG_FUNCTION(this);
}

void HBMBank::ReceiveRequest(MemoryRequest request) {

  if (request.cuid == 12345)
        NS_LOG_INFO("Remote request " << request.requestId << " accessing " << request.address << " in Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
      else
         NS_LOG_INFO("Request " << request.requestId << " accessing " << request.address << " in Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");

  uint32_t bus_delay = request.size / HBM_BUS_BANDWIDTH_PER_PC;
  bus_delay += HBM_CAS_LATENCY;

  if (!m_busy)
    {
      m_busy = true;
      uint32_t row_id = EXTRACT_ROW(request.address);
      if (row_id != m_activeRow) {
        NS_LOG_INFO("Row miss at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId);
        m_activeRow = row_id;
        bus_delay += HBM_ROW_MISS_PENALTY;
      }
      else
        NS_LOG_INFO("Row hit at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId);
      m_processEvent = Simulator::Schedule(NanoSeconds(bus_delay),
                                           &HBMBank::FinishProcessing,
                                           this, request);
    }
  else
    {
      request_q.push(request);
      NS_LOG_INFO("Congestion at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << ", Queue length " << request_q.size() );
    }
}

void
HBMBank::FinishProcessing(MemoryRequest request)
{
  NS_LOG_INFO("HBM Bank " <<  m_bankId << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId  << " processed request " << request.requestId << " by CU " << request.cuid
              << " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  request.cb(request.arg);
  if (!request_q.empty()) {
    MemoryRequest next_request = request_q.front();
    m_busy = true;
    
    uint32_t bus_delay = next_request.size / HBM_BUS_BANDWIDTH_PER_PC;
    bus_delay += HBM_CAS_LATENCY;
    uint32_t row_id = EXTRACT_ROW(next_request.address);

    if (row_id != m_activeRow) {
        m_activeRow = row_id;
        bus_delay += HBM_ROW_MISS_PENALTY;
        NS_LOG_INFO("Row miss at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(next_request.address) << " in channel " << EXTRACT_PC(next_request.address)
              << " in stack " << EXTRACT_STACK(next_request.address) << " on node " << m_nodeId);
    }
    else
        NS_LOG_INFO("Row hit at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(next_request.address) << " in channel " << EXTRACT_PC(next_request.address)
              << " in stack " << EXTRACT_STACK(next_request.address) << " on node " << m_nodeId);

    
    request_q.pop();
    Simulator::Schedule(NanoSeconds(bus_delay), &HBMBank::FinishProcessing, this, next_request);
  }
  	
}

void HBMBank::Initialize(uint32_t nodeId, uint32_t bankId) {
  m_nodeId = nodeId;
  m_bankId = bankId;
}

} // namespace ns3
