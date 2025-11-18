#include "hbm-controller.h"
#include "hbm-bank.h"
#include "ns3/log.h"

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
HBMController::InitializeBanks(uint32_t numBanks)
{
  NS_LOG_FUNCTION(this << numBanks);

  m_banks.clear();
  for (uint32_t i = 0; i < numBanks; i++)
    {
      m_banks.push_back(CreateObject<HBMBank>());
    }
}

void HBMController::SendRequest(uint32_t requestId, uint64_t address, uint32_t size, uint32_t bankId, bool isWrite, Callback<void, void*> cb, void* arg)
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
  MemoryRequest request = {address, size, bankId, isWrite, requestId, cb, arg};
  m_banks[request.bankId]->ReceiveRequest(request);
}

} // namespace ns3
