#ifndef SimCTPPS_CTPPSPixelDigiProducer_RPix_HIT_CHARGE_CONVERTER_H
#define SimCTPPS_CTPPSPixelDigiProducer_RPix_HIT_CHARGE_CONVERTER_H

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimCTPPS/CTPPSPixelDigiProducer/interface/RPixLinearChargeCollectionDrifter.h"
#include "SimCTPPS/CTPPSPixelDigiProducer/interface/RPixLinearChargeDivider.h"
#include "SimCTPPS/CTPPSPixelDigiProducer/interface/RPixChargeShare.h"

class RPixHitChargeConverter
{
public:
  RPixHitChargeConverter(const edm::ParameterSet &params_, CLHEP::HepRandomEngine& eng, uint32_t det_id);
  ~RPixHitChargeConverter();
    
  std::map<unsigned short, double, std::less<unsigned short> > processHit(const PSimHit &hit);
private:
  const edm::ParameterSet &params_;
  const uint32_t det_id_;
    
  RPixLinearChargeDivider* theRPixChargeDivider;
  RPixLinearChargeCollectionDrifter* theRPixChargeCollectionDrifter;
  RPixChargeShare* theRPixChargeShare;
  int verbosity_;
};

#endif
