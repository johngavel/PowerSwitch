#include "powerswitch.h"

#include "powermemory.h"

#include <ethernetmodule.h>
#include <gpio.h>
#include <screen.h>
#include <serialport.h>
#include <temperature.h>

static void minimalist(OutputInterface* terminal);
static void testOutput(OutputInterface* terminal);
static void onCommand(OutputInterface* terminal);
static void offCommand(OutputInterface* terminal);

void PowerSwitch::setupTask() {
  TERM_CMD->addCmd("min", "", "Help for the minimalist http requests", minimalist);
  TERM_CMD->addCmd("test", "", "Tests all of the Relay Commands and Status GPIO", testOutput);
  TERM_CMD->addCmd("on", "[n]", "Turn on [n] switch", onCommand);
  TERM_CMD->addCmd("off", "[n]", "Turn off [n] switch", offCommand);

  GPIO->configurePinLED(GPIO_INTERNAL, 6, GPIO_SINK, 0, "Debug LED 1");
  GPIO->configurePinLED(GPIO_INTERNAL, 7, GPIO_SINK, 1, "Debug LED 2");
  GPIO->configurePinLED(GPIO_INTERNAL, 8, GPIO_SINK, 2, "Debug LED 3");
  GPIO->configurePinLED(GPIO_INTERNAL, 9, GPIO_SINK, 3, "Debug LED 4");
  GPIO->configurePinLED(GPIO_INTERNAL, 10, GPIO_SINK, 4, "Debug LED 5");
  GPIO->configurePinLED(GPIO_INTERNAL, 11, GPIO_SINK, 5, "Debug LED 6");

  GPIO->configurePinIO(GPIO_INPUT, GPIO_EXTERNAL_EXPANDER_1, 0, 1, "Relay Status 1");
  GPIO->configurePinIO(GPIO_INPUT, GPIO_EXTERNAL_EXPANDER_1, 1, 2, "Relay Status 2");
  GPIO->configurePinIO(GPIO_INPUT, GPIO_EXTERNAL_EXPANDER_1, 2, 3, "Relay Status 3");
  GPIO->configurePinIO(GPIO_INPUT, GPIO_EXTERNAL_EXPANDER_1, 3, 4, "Relay Status 4");
  GPIO->configurePinIO(GPIO_INPUT, GPIO_EXTERNAL_EXPANDER_1, 4, 5, "Relay Status 5");

  GPIO->configurePinIO(GPIO_PULSE, GPIO_EXTERNAL_EXPANDER_1, 11, 1, "Relay Command 1");
  GPIO->configurePinIO(GPIO_PULSE, GPIO_EXTERNAL_EXPANDER_1, 12, 2, "Relay Command 2");
  GPIO->configurePinIO(GPIO_PULSE, GPIO_EXTERNAL_EXPANDER_1, 13, 3, "Relay Command 3");
  GPIO->configurePinIO(GPIO_PULSE, GPIO_EXTERNAL_EXPANDER_1, 14, 4, "Relay Command 4");
  GPIO->configurePinIO(GPIO_PULSE, GPIO_EXTERNAL_EXPANDER_1, 15, 5, "Relay Command 5");

  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 5, GPIO_SINK, 6, "Monitor LED 1");
  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 6, GPIO_SINK, 7, "Monitor LED 2");
  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 7, GPIO_SINK, 8, "Monitor LED 3");
  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 8, GPIO_SINK, 9, "Monitor LED 4");
  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 9, GPIO_SINK, 10, "Monitor LED 5");
  GPIO->configurePinLED(GPIO_EXTERNAL_EXPANDER_1, 10, GPIO_SINK, 11, "Monitor LED 6");
  setRefreshMilli(100);
}

void PowerSwitch::executeTask() {
  monitorSwitch();
  monitorDevice();
  saveIPData();
}

static bool firstPass = true;
void PowerSwitch::monitorSwitch() {
  for (int i = 1; i <= NUM_DEVICES; i++) {
    bool currentStatus = GPIO->getPin(GPIO_INPUT, i)->getCurrentStatus();
    if (currentStatus != POWER_DATA->getCurrentRelayStatus(i - 1)) {
      if (firstPass == false) SCREEN->setScreen(LIGHT, String(POWER_DATA->getDeviceName(i - 1)) + String((currentStatus) ? " ON" : " OFF"));

      POWER_DATA->setCurrentRelayStatus(i - 1, currentStatus);
      GPIO->getPin(GPIO_LED, i)->setCurrentStatus(currentStatus);
    }
  }
  firstPass = false;
}

void PowerSwitch::monitorDevice() {
  bool status = true;
  bool overallStatus = true;

  POWER_DATA->setOnline(HWC_COMPUTER, status);
  GPIO->getPin(GPIO_LED, 6)->setCurrentStatus(status);
  overallStatus &= status;

  status = SCREEN->getTimerRun();
  POWER_DATA->setOnline(HWC_DISPLAY, status);
  GPIO->getPin(GPIO_LED, 7)->setCurrentStatus(status);
  overallStatus &= status;

  status = GPIO->getTimerRun();
  POWER_DATA->setOnline(HWC_GPIO, status);
  GPIO->getPin(GPIO_LED, 8)->setCurrentStatus(status);
  overallStatus &= status;

  status = TEMPERATURE->validTemperature();
  POWER_DATA->setOnline(HWC_TEMPERATURE, status);
  GPIO->getPin(GPIO_LED, 9)->setCurrentStatus(status);
  overallStatus &= status;

  status = ETHERNET->getTimerRun();
  POWER_DATA->setOnline(HWC_ETHERNET, status);
  GPIO->getPin(GPIO_LED, 10)->setCurrentStatus(status);
  overallStatus &= status;

  status = EEPROM->getTimerRun();
  POWER_DATA->setOnline(HWC_EEPROM, status);
  GPIO->getPin(GPIO_LED, 11)->setCurrentStatus(status);
  overallStatus &= status;

  GPIO->getPin(GPIO_LED, 0)->setCurrentStatus(overallStatus);
  ;
}

void PowerSwitch::saveIPData() {
  if (ETHERNET->ipChanged == true) {
    IPAddress address;

    address = ETHERNET->getIPAddress();
    POWER_MEMORY.ipAddress[0] = address[0];
    POWER_MEMORY.ipAddress[1] = address[1];
    POWER_MEMORY.ipAddress[2] = address[2];
    POWER_MEMORY.ipAddress[3] = address[3];
    address = ETHERNET->getDNS();
    POWER_MEMORY.dnsAddress[0] = address[0];
    POWER_MEMORY.dnsAddress[1] = address[1];
    POWER_MEMORY.dnsAddress[2] = address[2];
    POWER_MEMORY.dnsAddress[3] = address[3];
    address = ETHERNET->getGateway();
    POWER_MEMORY.gatewayAddress[0] = address[0];
    POWER_MEMORY.gatewayAddress[1] = address[1];
    POWER_MEMORY.gatewayAddress[2] = address[2];
    POWER_MEMORY.gatewayAddress[3] = address[3];
    address = ETHERNET->getSubnetMask();
    POWER_MEMORY.subnetMask[0] = address[0];
    POWER_MEMORY.subnetMask[1] = address[1];
    POWER_MEMORY.subnetMask[2] = address[2];
    POWER_MEMORY.subnetMask[3] = address[3];
    EEPROM_FORCE;
  }
  ETHERNET->ipChanged = false;
}

void minimalist(OutputInterface* terminal) {
  terminal->println(INFO, "For use in automation, there are minimalist HTTP "
                          "requests and responses.");
  terminal->println(INFO, "To make use of these, open a telnet session to the "
                          "power module on port 80.");
  terminal->println(INFO, "To turn on a switch - \"GET /on/[n]\" the response is \"on\".");
  terminal->println(INFO, "To turn off a switch - \"GET /off/[n]\" the response is \"off\".");
  terminal->println(INFO, "To check the status of a switch - \"GET /stat/[n]\" the "
                          "response is \"on\" or \"off\".");
  terminal->println(INFO, "[n] is the switch number between 1 and " + String(NUM_DEVICES));
  terminal->println();
  terminal->println(INFO, "The purpose of these minimalist http commands is to enable the coder");
  terminal->println(INFO, "to make use of the power switch without the overhead of http.");
  terminal->prompt();
}

void testOutput(OutputInterface* terminal) {
  int numberOfDevices = POWER_DATA->getNumberOfDevices();
  bool firstState;
  bool secondState;
  bool thirdState;
  bool error = false;
  bool overall = false;
  terminal->println(INFO, "Testing Output Ports");
  terminal->print(INFO, "Output Ports: ");
  terminal->println(INFO, String(numberOfDevices));
  for (int i = 1; i <= numberOfDevices; i++) {
    error = false;
    rp2040.wdt_reset();
    terminal->print(INFO, "Testing Port: ");
    terminal->print(INFO, String(i));
    terminal->print(INFO, "; Name: ");
    terminal->println(INFO, String(POWER_DATA->getDeviceName(i)));
    firstState = GPIO->getPin(GPIO_INPUT, i)->getCurrentStatus();
    GPIO->getPin(GPIO_PULSE, i)->setCurrentStatus(true);
    delay(1000);
    secondState = GPIO->getPin(GPIO_INPUT, i)->getCurrentStatus();
    GPIO->getPin(GPIO_PULSE, i)->setCurrentStatus(true);
    delay(1000);
    thirdState = GPIO->getPin(GPIO_INPUT, i)->getCurrentStatus();

    if (firstState == secondState) error = true;
    if (thirdState == secondState) error = true;
    if (thirdState != firstState) error = true;

    if (error) {
      terminal->print(ERROR, "Output Port ");
      terminal->print(WARNING, String(i));
      terminal->println(WARNING, " Failed");
      terminal->print(ERROR, "First State: ");
      terminal->println(WARNING, (firstState) ? "HIGH" : "LOW");
      terminal->print(ERROR, "Second State: ");
      terminal->println(WARNING, (secondState) ? "HIGH" : "LOW");
      terminal->print(ERROR, "Third State: ");
      terminal->println(WARNING, (thirdState) ? "HIGH" : "LOW");
    } else {
      terminal->print(INFO, "SUCCESS: Output Port ");
      terminal->print(INFO, String(i));
      terminal->println(INFO, " Passed");
    }
    overall |= error;
  }
  if (overall)
    terminal->println(FAILED, "ERROR: Output Port Test Failed");
  else
    terminal->println(PASSED, "SUCCESS: Output Port Test Passed");
  terminal->prompt();
}

void onCommand(OutputInterface* terminal) {
  unsigned long index;
  GPIO_DESCRIPTION* gpio;
  char* value;
  value = terminal->readParameter();
  if (value != NULL) {
    index = (unsigned long) atoi(value);
    gpio = GPIO->getPin(GPIO_PULSE, index);
    if (gpio != nullptr) {
      if (GPIO->getPin(GPIO_INPUT, index)->getCurrentStatus() == false) GPIO->getPin(GPIO_PULSE, index)->setCurrentStatus(true);
    } else {
      terminal->println(ERROR, "Cannot find Switch Index: " + String(index));
    }
  } else {
    terminal->invalidParameter();
  }
  terminal->prompt();
}

void offCommand(OutputInterface* terminal) {
  unsigned long index;
  GPIO_DESCRIPTION* gpio;
  char* value;
  value = terminal->readParameter();
  if (value != NULL) {
    index = (unsigned long) atoi(value);
    gpio = GPIO->getPin(GPIO_PULSE, index);
    if (gpio != nullptr) {
      if (GPIO->getPin(GPIO_INPUT, index)->getCurrentStatus() == true) GPIO->getPin(GPIO_PULSE, index)->setCurrentStatus(true);
    } else {
      terminal->println(ERROR, "Cannot find Switch Index: " + String(index));
    }
  } else {
    terminal->invalidParameter();
  }
  terminal->prompt();
}