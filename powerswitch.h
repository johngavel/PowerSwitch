#ifndef __GAVEL_POWERSWITCH
#define __GAVEL_POWERSWITCH

#include <architecture.h>

class PowerSwitch : public Task {
public:
  PowerSwitch() : Task("PowerSwitch"){};
  void setupTask();
  void executeTask();

private:
  void monitorSwitch();
  void saveIPData();
  void monitorDevice();
};

#endif
