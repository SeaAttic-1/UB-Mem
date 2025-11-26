#include "hbm-controller-simple.h"
#include "hbm-bank-simple.h"
#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleHBMController");
NS_OBJECT_ENSURE_REGISTERED(SimpleHBMController);

TypeId SimpleHBMController::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::SimpleHBMController")
      .SetParent<Object>()
      .SetGroupName("SimpleHBM")
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

void
SimpleHBMController::InitializeBanks(uint32_t nodeId, uint32_t numBanks)
{
  NS_LOG_FUNCTION(this << numBanks);

  m_banks.clear();
  for (uint32_t i = 0; i < numBanks; i++)
    {
      auto bank = CreateObject<SimpleHBMBank>();
      bank->SetNodeId(nodeId);
      m_banks.push_back(bank);
    }
}

void SimpleHBMController::SendRequest(uint32_t cuid, uint32_t requestId, uint64_t address, uint32_t size, uint32_t bankId, bool isWrite, Callback<void, void*> cb, void* arg)
{
  NS_LOG_FUNCTION(this << requestId);

  if (m_banks.empty())
    {
      NS_LOG_ERROR("HBMController has no banks initialized!");
      return;
    }
  if (bankId >= m_banks.size()) {
      NS_LOG_ERROR("Attempt to access bank" << bankId << "but HBM has only" << m_banks.size() << "banks" );
      return;
  }
  SimpleMemoryRequest request = {cuid, address, size, bankId, isWrite, requestId, cb, arg};
  m_banks[request.bankId]->ReceiveRequest(request);
}

} // namespace ns3
