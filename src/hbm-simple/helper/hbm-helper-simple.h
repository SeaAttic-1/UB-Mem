#ifndef HBM_HELPER_SIMPLE_H
#define HBM_HELPER_SIMPLE_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "../model/hbm-controller-simple.h"

namespace ns3 {

class SimpleHBMController;

class SimpleHBMHelper
{
public:
  SimpleHBMHelper();

  Ptr<SimpleHBMController> Create(uint32_t nodeId);
};

} // namespace ns3

#endif // HBM_HELPER_H
