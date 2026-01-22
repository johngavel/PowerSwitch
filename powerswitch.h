#ifndef __GAVEL_POWERSWITCH_H
#define __GAVEL_POWERSWITCH_H

#include "powerconfig.h"
#include "powerdef.h"
#include "powerstatus.h"

#include <GavelInterfaces.h>
#include <GavelTask.h>

class PowerSwitch : public Task {
public:
  PowerSwitch() : Task("PowerSwitch"){};

  // Virtual Task Methods
  virtual void addCmd(TerminalCommand* __termCmd) override;
  virtual void reservePins(BackendPinSetup* pinsetup) override;
  virtual bool setupTask(OutputInterface* __terminal) override;
  virtual bool executeTask() override;

  PowerConfig powerConfig;
  PowerStatus powerStatus;

private:
  void commandSwitch();
  void monitorSwitch();
  void monitorDevice();
  /* Terminal Commands */
  void powerOn(OutputInterface* terminal);
  void powerOff(OutputInterface* terminal);
  void powerStat(OutputInterface* terminal);
};

#endif // __GAVEL_POWERSWITCH_H
