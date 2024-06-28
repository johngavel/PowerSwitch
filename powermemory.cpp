#include "powermemory.h"

#include <serialport.h>

static void configure();

void PowerMemory::setup() {
  PORT->addCmd("config", "...", "Configure Devices \"config ?\" for more", configure);
  String error = "ERROR";
  error.toCharArray(ErrorString, NAME_MAX_LENGTH);
}

void PowerMemory::initMemory() {
  randomSeed(rp2040.hwrand32());
  EEPROM_TAKE;
  memory.mem.numberOfDevices = NUM_DEVICES;
  memory.mem.isDHCP = false;
  memory.mem.macAddress[0] = 0xDE;
  memory.mem.macAddress[1] = 0xAD;
  memory.mem.macAddress[2] = 0xCC;
  memory.mem.macAddress[3] = random(256);
  memory.mem.macAddress[4] = random(256);
  memory.mem.macAddress[5] = random(256);
  memory.mem.ipAddress[0] = 0;
  memory.mem.ipAddress[1] = 0;
  memory.mem.ipAddress[2] = 0;
  memory.mem.ipAddress[3] = 0;
  memory.mem.dnsAddress[0] = 255;
  memory.mem.dnsAddress[1] = 255;
  memory.mem.dnsAddress[2] = 255;
  memory.mem.dnsAddress[3] = 255;
  memory.mem.subnetMask[0] = 255;
  memory.mem.subnetMask[1] = 255;
  memory.mem.subnetMask[2] = 255;
  memory.mem.subnetMask[3] = 0;
  memory.mem.gatewayAddress[0] = 255;
  memory.mem.gatewayAddress[1] = 255;
  memory.mem.gatewayAddress[2] = 255;
  memory.mem.gatewayAddress[3] = 255;
  String device = "Device";
  char deviceString[NAME_MAX_LENGTH];
  device.toCharArray(deviceString, NAME_MAX_LENGTH);
  for (int i = 0; i < memory.mem.numberOfDevices; i++) {
    memset(memory.mem.deviceName[i], 0, NAME_MAX_LENGTH + 1);
    strncpy(memory.mem.deviceName[i], deviceString, NAME_MAX_LENGTH - 1);
  }
  memory.mem.drift = 0;
  EEPROM_GIVE;
  EEPROM_FORCE;
}

void PowerMemory::printData() {
  EEPROM_TAKE;
  PORT->print(INFO, "MAC: ");
  PORT->print(INFO, String(memory.mem.macAddress[0], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[1], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[2], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[3], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[4], HEX) + ":");
  PORT->println(INFO, String(memory.mem.macAddress[5], HEX));
  PORT->println(INFO, "IP Address is " + String((memory.mem.isDHCP) ? "DHCP" : "Static"));
  PORT->print(INFO, "IP Address: ");
  PORT->print(INFO, String(memory.mem.ipAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.ipAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.ipAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.ipAddress[3]));
  PORT->print(INFO, "Subnet Mask: ");
  PORT->print(INFO, String(memory.mem.subnetMask[0]) + ".");
  PORT->print(INFO, String(memory.mem.subnetMask[1]) + ".");
  PORT->print(INFO, String(memory.mem.subnetMask[2]) + ".");
  PORT->println(INFO, String(memory.mem.subnetMask[3]));
  PORT->print(INFO, "Gateway: ");
  PORT->print(INFO, String(memory.mem.gatewayAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.gatewayAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.gatewayAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.gatewayAddress[3]));
  PORT->print(INFO, "DNS Address: ");
  PORT->print(INFO, String(memory.mem.dnsAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.dnsAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.dnsAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.dnsAddress[3]));
  PORT->print(INFO, "Num Dev: ");
  PORT->println(INFO, String(getNumberOfDevices()));
  for (int i = 0; i < getNumberOfDevices(); i++) {
    PORT->print(INFO, "Name ");
    PORT->print(INFO, String(i + 1));
    PORT->print(INFO, ": ");
    EEPROM_GIVE;
    PORT->println(INFO, String(getDeviceName(i)));
    EEPROM_TAKE;
  }
  PORT->print(INFO, "Temperature Drift: ");
  PORT->println(INFO, String(memory.mem.drift));
  EEPROM_GIVE;
}

unsigned char* PowerMemory::getData() {
  return memory.memoryArray;
}

unsigned long PowerMemory::getLength() {
  return sizeof(MemoryStruct);
}

void PowerMemory::setDeviceName(byte device, const char* name, int length) {
  EEPROM_TAKE;
  byte index = device;

  if (device < memory.mem.numberOfDevices) {
    if (length >= 0) {
      int nameLength = length;
      if (length > (NAME_MAX_LENGTH - 1)) nameLength = NAME_MAX_LENGTH - 1;
      memset(memory.mem.deviceName[index], 0, NAME_MAX_LENGTH);
      strncpy(memory.mem.deviceName[index], name, nameLength);
    }
  }
  EEPROM_GIVE;
}

char* PowerMemory::getDeviceName(byte device) {
  char* returnVal;
  EEPROM_TAKE;
  if (device >= NUM_DEVICES) returnVal = ErrorString;
  returnVal = memory.mem.deviceName[device];
  EEPROM_GIVE;
  return returnVal;
}

enum ConfigItem { None = 0, TempDrift, IpDHCP, IpAddress, IpDNS, IpSubnet, IpGW, Name };

static void configure() {
  char* value;
  ConfigItem item = None;
  unsigned long parameters[4];
  unsigned long count = 0;
  char* stringParameter = NULL;
  bool requiresStringParameter = false;
  bool commandComplete = true;

  PORT->println();
  value = PORT->readParameter();

  if (value == NULL) {
    PORT->println(WARNING, "Missing any parameters....");
    commandComplete = false;
    item = None;
  } else if (strncmp("temp", value, 4) == 0) {
    item = TempDrift;
    count = 1;
  } else if (strncmp("dhcp", value, 4) == 0) {
    item = IpDHCP;
    count = 1;
  } else if (strncmp("ip", value, 2) == 0) {
    item = IpAddress;
    count = 4;
  } else if (strncmp("dns", value, 3) == 0) {
    item = IpDNS;
    count = 4;
  } else if (strncmp("gw", value, 2) == 0) {
    item = IpGW;
    count = 4;
  } else if (strncmp("subnet", value, 6) == 0) {
    item = IpSubnet;
    count = 4;
  } else if (strncmp("name", value, 4) == 0) {
    item = Name;
    count = 1;
    requiresStringParameter = true;
  } else if ((strncmp("?", value, 1) == 0) || (strncmp("help", value, 1) == 0)) {
    item = None;
  } else {
    PORT->print(WARNING, "Invalid Config: <");
    PORT->print(WARNING, value);
    PORT->println(WARNING, ">");
    item = None;
    commandComplete = false;
  }
  for (unsigned long i = 0; i < count; i++) {
    value = PORT->readParameter();
    if (value == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      PORT->println(WARNING, "Missing Parameters in config");
      break;
    } else {
      parameters[i] = atoi(value);
    }
  }
  if (requiresStringParameter == true) {
    stringParameter = PORT->readParameter();
    if (stringParameter == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      PORT->println(WARNING, "Missing Parameters in config");
    }
  }
  switch (item) {
  case TempDrift:
    POWER_MEMORY.drift = parameters[0];
    EEPROM_FORCE;
    break;
  case IpDHCP:
    POWER_MEMORY.isDHCP = parameters[0];
    EEPROM_FORCE;
    break;
  case IpAddress:
    POWER_MEMORY.ipAddress[0] = parameters[0];
    POWER_MEMORY.ipAddress[1] = parameters[1];
    POWER_MEMORY.ipAddress[2] = parameters[2];
    POWER_MEMORY.ipAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpDNS:
    POWER_MEMORY.dnsAddress[0] = parameters[0];
    POWER_MEMORY.dnsAddress[1] = parameters[1];
    POWER_MEMORY.dnsAddress[2] = parameters[2];
    POWER_MEMORY.dnsAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpSubnet:
    POWER_MEMORY.subnetMask[0] = parameters[0];
    POWER_MEMORY.subnetMask[1] = parameters[1];
    POWER_MEMORY.subnetMask[2] = parameters[2];
    POWER_MEMORY.subnetMask[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpGW:
    POWER_MEMORY.gatewayAddress[0] = parameters[0];
    POWER_MEMORY.gatewayAddress[1] = parameters[1];
    POWER_MEMORY.gatewayAddress[2] = parameters[2];
    POWER_MEMORY.gatewayAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case Name:
    POWER_DATA->setDeviceName(parameters[0] - 1, stringParameter, strlen(stringParameter));
    EEPROM_FORCE;
    break;
  case None:
  default:
    PORT->println(HELP, "config name [n] [name] ", "- Sets device name");
    PORT->println(HELP, "config temp [n]        ", "- Set the drift for the temperature sensor");
    PORT->println(HELP, "config dhcp [0|1]      ", "- 0, turns off DHCP; 1, turns on DHCP");
    PORT->println(HELP, "config ip [n] [n] [n] [n]     ", "- Sets the IP address n.n.n.n");
    PORT->println(HELP, "config dns [n] [n] [n] [n]    ", "- Sets the DNS address n.n.n.n");
    PORT->println(HELP, "config gw [n] [n] [n] [n]     ", "- Sets the Gateway address n.n.n.n");
    PORT->println(HELP, "config subnet [n] [n] [n] [n] ", "- Sets the Subnet Mask n.n.n.n");
    PORT->println(HELP, "config help/?          ", "- Print config Help");
    PORT->println();
    PORT->println(HELP, "Note: Addresses use a space seperator, so "
                        "\"192.168.168.4\" is \"192 168 168 4\"");
    PORT->println(HELP, "      Must Reboot the system for some changes to take effect");
  }
  PORT->println((commandComplete) ? PASSED : FAILED, "Command Complete");
  PORT->prompt();
}
