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
// const HW_TYPES ProgramInfo::hw_type = HW_RP2040_ZERO;
const HW_TYPES ProgramInfo::hw_type = HW_RASPBERRYPI_PICO;
const unsigned char ProgramInfo::ProgramNumber = 0x01;
const unsigned char ProgramInfo::MajorVersion = 0x01;
const unsigned char ProgramInfo::MinorVersion = 0x10;

PowerSwitch powerSwitch;
PowerMemory powerMemory;
PowerDisplay powerDisplay;

void setup() {
  setup0Start();

  GPIO->configureHW(ProgramInfo::hw_type);
  GPIO->configureExpander(0, TCA_ADDRESS);
  PORT->setup();
  EEPROM->setData(&powerMemory);
  EEPROM->setup();

  TEMPERATURE->configure(DHT_11_PIN, POWER_MEMORY.drift);
  TEMPERATURE->setup();

  BLINK->setHW(ProgramInfo::hw_type);
  BLINK->setup();
  SCAN->setup();
  SCREEN->setRefreshScreen(&powerDisplay, 1000);
  SCREEN->setup();
  FILES->setup();

  ETHERNET->configure(POWER_MEMORY.macAddress, POWER_MEMORY.isDHCP, POWER_MEMORY.ipAddress, POWER_MEMORY.dnsAddress,
                      POWER_MEMORY.subnetMask, POWER_MEMORY.gatewayAddress);
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
  WATCHDOG->loop();
  delay(1);
}

void loop1() {
  powerSwitch.loop();
  PORT->loop();
  SCREEN->loop();
  GPIO->loop();
  EEPROM->loop();
  TEMPERATURE->loop();
  WATCHDOG->loop();
  delay(1);
}
