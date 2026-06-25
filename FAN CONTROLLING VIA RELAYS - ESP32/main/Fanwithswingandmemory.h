#ifndef _FANWITHSWINGANDMEMORY_H_
#define _FANWITHSWINGANDMEMORY_H_

#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/RangeController.h>
#include <Capabilities/ToggleController.h>

class Fanwithswingandmemory 
: public SinricProDevice
, public PowerStateController<Fanwithswingandmemory>
, public RangeController<Fanwithswingandmemory>
, public ToggleController<Fanwithswingandmemory> {
  friend class PowerStateController<Fanwithswingandmemory>;
  friend class RangeController<Fanwithswingandmemory>;
  friend class ToggleController<Fanwithswingandmemory>;
public:
  Fanwithswingandmemory(const String &deviceId) : SinricProDevice(deviceId, "Fanwithswingandmemory") {};
};

#endif
