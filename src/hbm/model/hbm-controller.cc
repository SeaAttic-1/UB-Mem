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
  Simulator::Schedule(NanoSeconds(HBM_CONTROLLER_WRITE_BUFFER_TIME_OUT), &HBMController::FlushWriteBuffer, this);
}


bool HBMController::SendRequest(uint64_t address, uint32_t size, bool isWrite, bool isRemote, Callback<void, void*> cb, void* arg)
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
  if (m_outstanding > HBM_MAX_MC_OTSD_LIMITS) {
    Simulator::Schedule(NanoSeconds(5), &HBMController::SendRequest, this, address, size, isWrite, isRemote, cb, arg);
    NS_LOG_INFO("MC Limit reached");
    return false;
  }

  if(isWrite) {
    if(isRemote)
      NS_LOG_INFO("Detected remote write request at " << address << " of size " << size);
    else
      NS_LOG_INFO("Detected write request at " << address << " of size " << size);
    if (TryCoalesce(address, size, isRemote, cb, arg)) return true;
    if (EnqueueWrite(address, size, isRemote, cb, arg)) return true;

    if(isRemote)
      Simulator::Schedule(NanoSeconds(5), &HBMController::SendRequest, this, address, size, isWrite, isRemote, cb, arg);
    else
      m_notify_callbacks.push_back(cb); // For intra-node background traffic, store this cb, which will be used to resume the cu's access
    return false;
  }

  return false; 
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
          i.end_addresses.push_back(address + size);
        }
        
        return true;
      }
      // There is no check for if the request can span multiple pages. Check for this outside the HBM controller
    }
  }
  return false;
}

bool HBMController::EnqueueWrite(uint64_t address, uint32_t size, bool isRemote, Callback<void, void*> cb, void* arg) {
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
      new_request.end_addresses.push_back(address + size);
    }
    m_write_buffer.push_back(new_request);
    // m_outstanding ++; // only increment for directly enqueued requests; merged requests do not count
    return true;
  }
  else {
    for(auto it = m_write_buffer.begin(); it != m_write_buffer.end();) {
      
        uint32_t stack = EXTRACT_STACK(it->address);
        if (m_stacks[stack]->SendRequest(*it)) {
          it = m_write_buffer.erase(it);
          m_outstanding ++;
        }
        else it ++;
    }

    if (m_write_buffer.size() < HBM_CONTROLLER_WRITE_BUFFER_MAX_SIZE) {

      MemoryRequest new_request;
      new_request.address = address;
      new_request.size = size;
      new_request.isWrite = true;

      if (arg != nullptr) {
        new_request.cbs.push_back(cb);
        new_request.args.push_back(arg);
        new_request.end_addresses.push_back(address + size);
      }
      m_write_buffer.push_back(new_request);
      // m_outstanding ++;
      return true;
    }
    return false;
  }
}

void HBMController::FlushWriteBuffer() {
  NS_LOG_INFO("Flush Write buffer on node " << m_nodeId);
  if (m_write_buffer.empty());
  else {
    for(auto it = m_write_buffer.begin(); it != m_write_buffer.end();) {
      
        uint32_t stack = EXTRACT_STACK(it->address);
        if (m_stacks[stack]->SendRequest(*it)) {
          it = m_write_buffer.erase(it);
          m_outstanding ++;
        }
        else it ++;
      }
  }
  Simulator::Schedule(NanoSeconds(HBM_CONTROLLER_WRITE_BUFFER_TIME_OUT), &HBMController::FlushWriteBuffer, this);
}

void HBMController::NotifyComplete(void) {
  m_outstanding --;
  for(auto cb: m_notify_callbacks) {
    cb(nullptr);
  }
}

} // namespace ns3
