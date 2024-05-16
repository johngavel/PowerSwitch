#ifndef __GAVEL_POWERSWITCH
#define __GAVEL_POWERSWITCH

#include <architecture.h>

class PowerSwitch : public Task {
public:
  void setup();
  void executeTask();

private:
  void monitorSwitch();
  void saveIPData();
  void monitorDevice();
};

#endif
