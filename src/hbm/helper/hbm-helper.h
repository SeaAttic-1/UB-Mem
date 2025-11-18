#ifndef HBM_HELPER_H
#define HBM_HELPER_H

#include "ns3/object.h"
#include "ns3/ptr.h"

namespace ns3 {

class HBMController;

class HBMHelper
{
public:
  HBMHelper();

  Ptr<HBMController> Create(uint32_t numBanks);

private:
  uint32_t m_defaultBanks;
};

} // namespace ns3

#endif // HBM_HELPER_H
