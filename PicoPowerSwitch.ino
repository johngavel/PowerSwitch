// CTRL - SHIFT - 'P' ; Upload data directory to little file system
#include "powerdisplay.h"
#include "powermemory.h"
#include "powerserver.h"
#include "powerswitch.h"

#include <architecture.h>
#include <eeprom.h>
#include <ethernetmodule.h>
#include <files.h>
#include <gpio.h>
#include <onboardled.h>
#include <scan.h>
#include <serialport.h>
#include <servermodule.h>
#include <startup.h>
#include <temperature.h>
#include <watchdog.h>

#define DHT_11_PIN 28
#define TCA_ADDRESS 0x27

const char* ProgramInfo::AppName = "Power Switch";
const char* ProgramInfo::ShortName = "power";
const char* ProgramInfo::compileDate = __DATE__;
const char* ProgramInfo::compileTime = __TIME__;
const unsigned char ProgramInfo::ProgramNumber = 0x01;
const unsigned char ProgramInfo::MajorVersion = 0x01;
const unsigned char ProgramInfo::MinorVersion = 0x12;
const char* ProgramInfo::AuthorName = "John J. Gavel";
const HardwareWire ProgramInfo::hardwarewire = HardwareWire(&Wire, 4, 5);
const HardwareSerialPort ProgramInfo::hardwareserial = HardwareSerialPort(&Serial1, 0, 1);

PowerSwitch powerSwitch;
PowerMemory powerMemory;
PowerDisplay powerDisplay;

void setup() {
  setup0Start();

  GPIO->configureExpander(0, TCA_ADDRESS);
  PORT->setup();

  EEPROM->configure(I2C_DEVICESIZE_24LC16);
  EEPROM->setData(&powerMemory);
  EEPROM->setup();

  TEMPERATURE->configure(DHT_11_PIN, POWER_MEMORY.drift);
  TEMPERATURE->setup();

  BLINK->setup();
  SCAN->setup();
  SCREEN->setRefreshScreen(&powerDisplay, 1000);
  SCREEN->setup();
  FILES->setup();

  ETHERNET->configure(POWER_MEMORY.macAddress, POWER_MEMORY.isDHCP, POWER_MEMORY.ipAddress, POWER_MEMORY.dnsAddress, POWER_MEMORY.subnetMask,
                      POWER_MEMORY.gatewayAddress);
  ETHERNET->setup();
  SERVER->setup();
  setupServerModule();

  powerSwitch.setup();

  GPIO->setup();
  WATCHDOG->setup();
  setup0Complete();
}

void setup1() {
  setup1Start();
  setup1Complete();
}

void loop() {
  BLINK->loop();
  ETHERNET->loop();
  SERVER->loop();
  PORT->loop();
  TEMPERATURE->loop();
  EEPROM->loop();
  powerSwitch.loop();
  WATCHDOG->loop();
  delay(1);
}

void loop1() {
  SCREEN->loop();
  GPIO->loop();
  WATCHDOG->loop();
  delay(1);
}
