#include "powerswitch.h"

#include "powermemory.h"

#include <ethernetmodule.h>
#include <gpio.h>
#include <screen.h>
#include <serialport.h>
#include <temperature.h>

static void minimalist();
void testOutput();

void PowerSwitch::setupTask() {
  PORT->addCmd("min", "", "Help for the minimalist http requests", minimalist);
  PORT->addCmd("test", "", "Tests all of the Relay Commands and Status GPIO", testOutput);

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

static void minimalist() {
  PORT->println();
  PORT->println(INFO, "For use in automation, there are minimalist HTTP "
                      "requests and responses.");
  PORT->println(INFO, "To make use of these, open a telnet session to the "
                      "power module on port 80.");
  PORT->println(INFO, "To turn on a switch - \"GET /on/[n]\" the response is \"on\".");
  PORT->println(INFO, "To turn off a switch - \"GET /off/[n]\" the response is \"off\".");
  PORT->println(INFO, "To check the status of a switch - \"GET /stat/[n]\" the "
                      "response is \"on\" or \"off\".");
  PORT->println(INFO, "[n] is the switch number between 1 and " + String(NUM_DEVICES));
  PORT->println();
  PORT->println(INFO, "The purpose of these minimalist http commands is to enable the coder");
  PORT->println(INFO, "to make use of the power switch without the overhead of http.");
  PORT->prompt();
}

void testOutput() {
  int numberOfDevices = POWER_DATA->getNumberOfDevices();
  bool firstState;
  bool secondState;
  bool thirdState;
  bool error = false;
  bool overall = false;
  PORT->println();
  PORT->println(INFO, "Testing Output Ports");
  PORT->print(INFO, "Output Ports: ");
  PORT->println(INFO, String(numberOfDevices));
  for (int i = 1; i <= numberOfDevices; i++) {
    error = false;
    rp2040.wdt_reset();
    PORT->print(INFO, "Testing Port: ");
    PORT->print(INFO, String(i));
    PORT->print(INFO, "; Name: ");
    PORT->println(INFO, String(POWER_DATA->getDeviceName(i)));
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
      PORT->print(ERROR, "Output Port ");
      PORT->print(WARNING, String(i));
      PORT->println(WARNING, " Failed");
      PORT->print(ERROR, "First State: ");
      PORT->println(WARNING, (firstState) ? "HIGH" : "LOW");
      PORT->print(ERROR, "Second State: ");
      PORT->println(WARNING, (secondState) ? "HIGH" : "LOW");
      PORT->print(ERROR, "Third State: ");
      PORT->println(WARNING, (thirdState) ? "HIGH" : "LOW");
    } else {
      PORT->print(INFO, "SUCCESS: Output Port ");
      PORT->print(INFO, String(i));
      PORT->println(INFO, " Passed");
    }
    overall |= error;
  }
  if (overall)
    PORT->println(FAILED, "ERROR: Output Port Test Failed");
  else
    PORT->println(PASSED, "SUCCESS: Output Port Test Passed");
  PORT->prompt();
}
