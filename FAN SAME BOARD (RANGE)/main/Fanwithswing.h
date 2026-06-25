#ifndef _FANWITHSWING_H_
#define _FANWITHSWING_H_

#include <SinricProDevice.h>
#include <Capabilities/ToggleController.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/RangeController.h>

class Fanwithswing 
: public SinricProDevice
, public ToggleController<Fanwithswing>
, public PowerStateController<Fanwithswing>
, public RangeController<Fanwithswing> {
  friend class ToggleController<Fanwithswing>;
  friend class PowerStateController<Fanwithswing>;
  friend class RangeController<Fanwithswing>;
public:
  Fanwithswing(const String &deviceId) : SinricProDevice(deviceId, "Fanwithswing") {};
};

#endif
