#include "hbm-bank.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/singleton.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
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

bool HBMBank::ReceiveRequest(MemoryRequest request) {

  if (request.isRemote)
        NS_LOG_INFO("Remote request accessing " << request.address << " in Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");
      else
         NS_LOG_INFO("Request accessing " << request.address << " in Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << " queued at " << Simulator::Now().GetNanoSeconds() << " ns");

  uint32_t bus_delay = request.size / HBM_BUS_BANDWIDTH_PER_PC;
  NS_LOG_INFO("Request size first is " << request.size);
  NS_LOG_INFO("Process delay first is " << bus_delay);

  if (!m_busy)
    {
      m_busy = true;
      uint32_t row_id = EXTRACT_ROW(request.address);
      if (row_id != m_activeRow) {
        NS_LOG_INFO("Row miss at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId);
              if (m_activeRow != 32768) {
                bus_delay += HBM_PRECHARGE_LATENCY;
              }
        m_activeRow = row_id;
        bus_delay += HBM_ACTIVE_ROW_LATENCY;
      }

      else
        NS_LOG_INFO("Row hit at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId);
      
      bus_delay += HBM_CAS_LATENCY;

      #ifdef INCLUDE_OTHER_OVERHEAD
        bus_delay += OTHER_OVERHEAD;
      #endif

      for(uint32_t i = 0; i < request.cbs.size(); i++) {
        uint64_t end_address = request.end_addresses[i];
        uint64_t num_of_bursts = (end_address - request.address) / HBM_ATOMIC_SIZE;
        if (num_of_bursts < 2) num_of_bursts = 0;
        else num_of_bursts --;
        NS_LOG_INFO("num_of_bursts " << num_of_bursts);
        Simulator::Schedule(NanoSeconds(bus_delay + num_of_bursts * HBM_TCCDL_LATENCY), &HBMBank::InvokeCallback, this, request.cbs[i], request.args[i]);
      }

      uint32_t extra_col_accesses = request.size / HBM_COLUMN_SIZE;
      if(extra_col_accesses < 2) extra_col_accesses = 0;
      else extra_col_accesses --;
      bus_delay += extra_col_accesses * HBM_TCCDL_LATENCY;
      
    
      NS_LOG_INFO("Process delay is " << bus_delay);

      m_processEvent = Simulator::Schedule(NanoSeconds(bus_delay),
                                           &HBMBank::FinishProcessing,
                                           this, request);
      return true;
    }
  else
    {
      if (request_q.size() < HBM_MAX_BANK_QUEUE_SIZE) {
        request_q.push(request);
        NS_LOG_INFO("Congestion at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId << ", Queue length " << request_q.size() );
        return true;
      }
      NS_LOG_INFO("Queue filled at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId);
      return false;
    }
}

void
HBMBank::FinishProcessing(MemoryRequest request)
{
  NS_LOG_INFO("HBM Bank " <<  m_bankId << " in group " << EXTRACT_BANK_GROUP(request.address) << " in channel " << EXTRACT_PC(request.address)
              << " in stack " << EXTRACT_STACK(request.address) << " on node " << m_nodeId  << " processed request "<< 
             " at " << Simulator::Now().GetNanoSeconds() << " ns");
  m_busy = false;
  m_pc_ptr->NotifyComplete();
  NodeList::GetNode(m_nodeId)->GetObject<HBMController>()->NotifyComplete();
  /*
  for(uint32_t i = 0; i < request.cbs.size(); i++) {
    request.cbs[i](request.args[i]);
  }
  */
  

  if (!request_q.empty()) {
    MemoryRequest next_request = request_q.front();
    m_busy = true;
    
    uint32_t bus_delay = next_request.size / HBM_BUS_BANDWIDTH_PER_PC;
    uint32_t row_id = EXTRACT_ROW(next_request.address);
    if (row_id != m_activeRow) {
        NS_LOG_INFO("Row miss at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(next_request.address) << " in channel " << EXTRACT_PC(next_request.address)
              << " in stack " << EXTRACT_STACK(next_request.address) << " on node " << m_nodeId);
              if (m_activeRow != 32768) {
                bus_delay += HBM_PRECHARGE_LATENCY;
              }
        m_activeRow = row_id;
        bus_delay += HBM_ACTIVE_ROW_LATENCY;
      }

      else
        NS_LOG_INFO("Row hit at Bank " << m_bankId  << " in group " << EXTRACT_BANK_GROUP(next_request.address) << " in channel " << EXTRACT_PC(next_request.address)
              << " in stack " << EXTRACT_STACK(next_request.address) << " on node " << m_nodeId);
      
      bus_delay += HBM_CAS_LATENCY;

      #ifdef INCLUDE_OTHER_OVERHEAD
        bus_delay += OTHER_OVERHEAD;
      #endif
      
      for(uint32_t i = 0; i < next_request.cbs.size(); i++) {
        uint64_t end_address = next_request.end_addresses[i];
        uint64_t num_of_bursts = (end_address - next_request.address) / HBM_ATOMIC_SIZE;
        if (num_of_bursts < 2) num_of_bursts = 0;
        else num_of_bursts --;
        NS_LOG_INFO("num_of_bursts " << num_of_bursts);
        Simulator::Schedule(NanoSeconds(bus_delay + num_of_bursts * HBM_TCCDL_LATENCY), &HBMBank::InvokeCallback, this, next_request.cbs[i], next_request.args[i]);
      }

      uint32_t extra_col_accesses = request.size / HBM_COLUMN_SIZE;
      if(extra_col_accesses < 2) extra_col_accesses = 0;
      else extra_col_accesses --;
      bus_delay += extra_col_accesses * HBM_TCCDL_LATENCY;

    request_q.pop();
    Simulator::Schedule(NanoSeconds(bus_delay), &HBMBank::FinishProcessing, this, next_request);
  }
  	
}

void HBMBank::InvokeCallback(Callback<void, void*> cb, void* arg) {
  NS_LOG_INFO("Invoked cb at " << Simulator::Now().GetNanoSeconds());
  cb(arg);
}

void HBMBank::Initialize(uint32_t nodeId, uint32_t bankId, HBMPseudoChannel* pc_ptr) {
  m_nodeId = nodeId;
  m_bankId = bankId;
  m_pc_ptr = pc_ptr;
}

} // namespace ns3
