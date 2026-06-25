#ifndef _FANWITHSWING_H_
#define _FANWITHSWING_H_

#include <SinricProDevice.h>
#include <Capabilities/ToggleController.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/ModeController.h>

class Fanwithswing 
: public SinricProDevice
, public ToggleController<Fanwithswing>
, public PowerStateController<Fanwithswing>
, public ModeController<Fanwithswing> {
  friend class ToggleController<Fanwithswing>;
  friend class PowerStateController<Fanwithswing>;
  friend class ModeController<Fanwithswing>;
public:
  Fanwithswing(const String &deviceId) : SinricProDevice(deviceId, "Fanwithswing") {};
};

#endif
