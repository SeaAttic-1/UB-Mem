#include "hbm-controller.h"
#include "hbm-bank.h"
#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HBMController");
NS_OBJECT_ENSURE_REGISTERED(HBMController);

TypeId HBMController::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::HBMController")
      .SetParent<Object>()
      .SetGroupName("HBM")
      .AddConstructor<HBMController>();
  return tid;
}

HBMController::HBMController()
{
  NS_LOG_FUNCTION(this);
}

HBMController::~HBMController()
{
  NS_LOG_FUNCTION(this);
}

void
HBMController::Initialize(uint32_t nodeId, uint32_t numStacks)
{
  m_nodeId = nodeId;
  for(uint32_t i = 0; i < numStacks; i++) {
    Ptr<HBMStack> new_stack = CreateObject<HBMStack>();
    new_stack->Initialize(nodeId, i);
    m_stacks.push_back(new_stack);
  }
  m_write_buffer.clear();
  Simulator::Schedule(NanoSeconds(HBM_CONTROOLER_WRITE_BUFFER_TIMEOUT), &HBMController::FlushWriteBuffer, this);
}


void HBMController::SendRequest(uint64_t address, uint32_t size, bool isWrite, bool isRemote, Callback<void, void*> cb, void* arg)
{
  // 
  /*
  uint64_t req_end = address + size;
  if (((req_end ^ address) & MAP_TO_SAME_ROW_BIT_MASK) != 0) {
    // If the request spans multiple rows
    // In this case, break this request into multiple smaller chunks
    // Assume that the start of address is page/row aligned
    NS_LOG_INFO("Detected request spanning more than one row starting at " << address << " of size " << size);
    for(uint32_t i = 0; i < size / HBM_ROW_SIZE; i++) {
      SendRequest(address + i * HBM_ROW_SIZE, size - i * HBM_ROW_SIZE, isWrite, cb, arg);
    }
    return;
  }
  */

  if(isWrite) {
    NS_LOG_INFO("Detected write request at " << address << " of size " << size);
    if (TryCoalesce(address, size, isRemote, cb, arg)) return;
    EnqueueWrite(address, size, isRemote, cb, arg);
  }
  // Read not implemented yet, but suffice for now
}

bool HBMController::TryCoalesce(uint64_t address, uint32_t size, bool isRemote, Callback<void, void*> cb, void* arg) {
  for(auto& i : m_write_buffer) {
    uint64_t entry_end = i.address + i.size;
    uint64_t req_end = address + size;

    if (((address ^ i.address) & MAP_TO_SAME_ROW_BIT_MASK) == 0 ) {
      NS_LOG_INFO("Detected requests writing to the same row with new request at " << address);
      bool overlap = !(req_end < i.address || entry_end < address);
      bool contiguous = (address == entry_end || req_end == i.address);

      if (overlap || contiguous) {
        i.address = std::min(address, i.address);
        uint64_t new_end = std::max(entry_end, req_end);
        i.size = new_end - i.address;

        if (arg != nullptr) {
          i.cbs.push_back(cb);
          i.args.push_back(arg);
        }
        
        return true;
      }
      // There is no check for if the request can span multiple pages. Check for this outside the HBM controller
    }
  }
  return false;
}

void HBMController::EnqueueWrite(uint64_t address, uint32_t size, bool isRemote, Callback<void, void*> cb, void* arg) {
  NS_LOG_INFO("Detected requests writing to the same row with new request at " << address);
  if (m_write_buffer.size() < HBM_CONTROLLER_WRITE_BUFFER_MAX_SIZE) {
    MemoryRequest new_request;
    new_request.address = address;
    new_request.size = size;
    new_request.isWrite = true;
    new_request.isRemote = isRemote;

    if (arg != nullptr) {
      new_request.cbs.push_back(cb);
      new_request.args.push_back(arg);
    }
    m_write_buffer.push_back(new_request);
  }
  else {
    for(auto& i : m_write_buffer) {
      uint32_t stack = EXTRACT_STACK(i.address);
      m_stacks[stack]->SendRequest(i);
    }

    m_write_buffer.clear();

    MemoryRequest new_request;
    new_request.address = address;
    new_request.size = size;
    new_request.isWrite = true;

    if (arg != nullptr) {
      new_request.cbs.push_back(cb);
      new_request.args.push_back(arg);
    }
    m_write_buffer.push_back(new_request);
  }
}

void HBMController::FlushWriteBuffer() {
  NS_LOG_INFO("Flush Write buffer on node " << m_nodeId);
  if (m_write_buffer.empty());
  else {
    for(auto& i : m_write_buffer) {
      
        uint32_t stack = EXTRACT_STACK(i.address);
        m_stacks[stack]->SendRequest(i);
      }
      m_write_buffer.clear();
  }
  Simulator::Schedule(NanoSeconds(HBM_CONTROOLER_WRITE_BUFFER_TIMEOUT), &HBMController::FlushWriteBuffer, this);
}

} // namespace ns3
