#include "powerswitch.h"

#include <GavelPicoStandard.h>
#include <GavelScreen.h>

extern Screen screen;

void PowerSwitch::addCmd(TerminalCommand* __termCmd) {
  __termCmd->addCmd("on", "[n]", "Powers on device 1 - 5.",
                    [this](TerminalLibrary::OutputInterface* terminal) { powerOn(terminal); });
  __termCmd->addCmd("off", "[n]", "Powers off device 1 - 5.",
                    [this](TerminalLibrary::OutputInterface* terminal) { powerOff(terminal); });
  __termCmd->addCmd("stat", "[n]", "Status of device 1 - 5.",
                    [this](TerminalLibrary::OutputInterface* terminal) { powerStat(terminal); });
};

void PowerSwitch::reservePins(BackendPinSetup* pinsetup) {
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 6, 0, GpioType::Led, "Debug LED 1", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 7, 1, GpioType::Led, "Debug LED 2", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 8, 2, GpioType::Led, "Debug LED 3", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 9, 3, GpioType::Led, "Debug LED 4", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 10, 4, GpioType::Led, "Debug LED 5", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_CPU_BOARD, 11, 5, GpioType::Led, "Debug LED 6", Polarity::Sink);

  gpioManager.addPin(GPIO_DEVICE_TCA9555, 0, 0, GpioType::Input, "Relay Status 1");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 1, 1, GpioType::Input, "Relay Status 2");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 2, 2, GpioType::Input, "Relay Status 3");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 3, 3, GpioType::Input, "Relay Status 4");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 4, 4, GpioType::Input, "Relay Status 5");

  gpioManager.addPin(GPIO_DEVICE_TCA9555, 5, 6, GpioType::Led, "Monitor LED 1", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 6, 7, GpioType::Led, "Monitor LED 2", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 7, 8, GpioType::Led, "Monitor LED 3", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 8, 9, GpioType::Led, "Monitor LED 4", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 9, 10, GpioType::Led, "Monitor LED 5", Polarity::Sink);
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 10, 11, GpioType::Led, "Monitor LED 6", Polarity::Sink);

  gpioManager.addPin(GPIO_DEVICE_TCA9555, 11, 0, GpioType::Pulse, "Relay Command 1");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 12, 1, GpioType::Pulse, "Relay Command 2");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 13, 2, GpioType::Pulse, "Relay Command 3");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 14, 3, GpioType::Pulse, "Relay Command 4");
  gpioManager.addPin(GPIO_DEVICE_TCA9555, 15, 4, GpioType::Pulse, "Relay Command 5");
};

bool PowerSwitch::setupTask(OutputInterface* __terminal) {
  return true;
};

bool PowerSwitch::executeTask() {
  commandSwitch();
  monitorSwitch();
  monitorDevice();
  return true;
};

static bool firstPass = true;
void PowerSwitch::monitorSwitch() {
  for (int i = 0; i < NUM_DEVICES; i++) {
    bool currentStatus = gpioManager.find(GpioType::Input, i)->get();
    if (currentStatus != powerStatus.getStatus(i)) {
      if (firstPass == false)
        screen.setScreen(LIGHT, String(String(powerConfig.getName(i)) + String((currentStatus) ? " ON" : " OFF")));

      powerStatus.setStatus(i, currentStatus);
      gpioManager.find(GpioType::Led, i + 1)->set(currentStatus);
    }
  }
  firstPass = false;
}

void PowerSwitch::monitorDevice() {
  bool overallStatus = true;
  for (unsigned int i = 0; i < hardwareList.size(); i++) {
    bool status = hardwareList[i]->isWorking();
    overallStatus &= status;
    gpioManager.find(GpioType::Led, i + 6)->set(status);
  }
  gpioManager.find(GpioType::Led, 0)->set(overallStatus);
};

void PowerSwitch::commandSwitch() {
  for (int i = 0; i < NUM_DEVICES; i++) {
    if (powerStatus.getCommand(i)) {
      gpioManager.find(GpioType::Pulse, i)->set(true);
      powerStatus.setCommand(i, false);
    }
  }
}

static int validateParameter(OutputInterface* terminal) {
  char* value = terminal->readParameter();
  if (!value) {
    terminal->invalidParameter();
    terminal->println();
    return -1;
  }
  int index = atoi(value) - 1;
  if (index > NUM_DEVICES) {
    terminal->invalidParameter();
    terminal->println();
    return -1;
  }
  return index;
}

void PowerSwitch::powerOn(OutputInterface* terminal) {
  int value = validateParameter(terminal);
  if (value >= 0) {
    if (powerStatus.getStatus(value) == false) powerStatus.setCommand(value, true);
  }
  terminal->prompt();
}

void PowerSwitch::powerOff(OutputInterface* terminal) {
  int value = validateParameter(terminal);
  if (value >= 0) {
    if (powerStatus.getStatus(value) == true) powerStatus.setCommand(value, true);
  }
  terminal->prompt();
}

void PowerSwitch::powerStat(OutputInterface* terminal) {
  int value = validateParameter(terminal);
  if (value >= 0) { terminal->println(INFO, (powerStatus.getStatus(value)) ? "ON" : "OFF"); }
  terminal->prompt();
}