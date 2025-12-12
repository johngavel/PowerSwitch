#include "powermemory.h"

#include <export.h>
#include <serialport.h>

static void configure(OutputInterface* terminal);
static void importMemory(OutputInterface* terminal);
static void exportMemory(OutputInterface* terminal);

void PowerMemory::setup() {
  TERM_CMD->addCmd("config", "...", "Configure Devices \"config ?\" for more", configure);
  TERM_CMD->addCmd("export", "", "Export Configuration to File System.", exportMemory);
  TERM_CMD->addCmd("import", "", "Import Configuration to File System.", importMemory);
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

void PowerMemory::printData(OutputInterface* terminal) {
  EEPROM_TAKE;
  terminal->print(INFO, "MAC: ");
  terminal->print(INFO, String(memory.mem.macAddress[0], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[1], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[2], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[3], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[4], HEX) + ":");
  terminal->println(INFO, String(memory.mem.macAddress[5], HEX));
  terminal->println(INFO, "IP Address is " + String((memory.mem.isDHCP) ? "DHCP" : "Static"));
  terminal->print(INFO, "IP Address: ");
  terminal->print(INFO, String(memory.mem.ipAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.ipAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.ipAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.ipAddress[3]));
  terminal->print(INFO, "Subnet Mask: ");
  terminal->print(INFO, String(memory.mem.subnetMask[0]) + ".");
  terminal->print(INFO, String(memory.mem.subnetMask[1]) + ".");
  terminal->print(INFO, String(memory.mem.subnetMask[2]) + ".");
  terminal->println(INFO, String(memory.mem.subnetMask[3]));
  terminal->print(INFO, "Gateway: ");
  terminal->print(INFO, String(memory.mem.gatewayAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.gatewayAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.gatewayAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.gatewayAddress[3]));
  terminal->print(INFO, "DNS Address: ");
  terminal->print(INFO, String(memory.mem.dnsAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.dnsAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.dnsAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.dnsAddress[3]));
  terminal->print(INFO, "Num Dev: ");
  terminal->println(INFO, String(getNumberOfDevices()));
  for (int i = 0; i < getNumberOfDevices(); i++) {
    terminal->print(INFO, "Name ");
    terminal->print(INFO, String(i + 1));
    terminal->print(INFO, ": ");
    EEPROM_GIVE;
    terminal->println(INFO, String(getDeviceName(i)));
    EEPROM_TAKE;
  }
  terminal->print(INFO, "Temperature Drift: ");
  terminal->println(INFO, String(memory.mem.drift));
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
  if (device >= NUM_DEVICES)
    returnVal = ErrorString;
  else
    returnVal = memory.mem.deviceName[device];
  EEPROM_GIVE;
  return returnVal;
}

enum ConfigItem { None = 0, TempDrift, IpDHCP, IpAddress, IpDNS, IpSubnet, IpGW, Name };

static void configure(OutputInterface* terminal) {
  char* value;
  ConfigItem item = None;
  unsigned long parameters[4];
  unsigned long count = 0;
  char* stringParameter = NULL;
  bool requiresStringParameter = false;
  bool commandComplete = true;

  value = terminal->readParameter();

  if (value == NULL) {
    terminal->println(WARNING, "Missing any parameters....");
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
    terminal->print(WARNING, "Invalid Config: <");
    terminal->print(WARNING, value);
    terminal->println(INFO, ">");
    item = None;
    commandComplete = false;
  }
  for (unsigned long i = 0; i < count; i++) {
    value = terminal->readParameter();
    if (value == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      terminal->println(WARNING, "Missing Parameters in config");
      break;
    } else {
      parameters[i] = atoi(value);
    }
  }
  if (requiresStringParameter == true) {
    stringParameter = terminal->readParameter();
    if (stringParameter == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      terminal->println(WARNING, "Missing Parameters in config");
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
    terminal->println(HELP, "config name [n] [name] ", "- Sets device name");
    terminal->println(HELP, "config temp [n]        ", "- Set the drift for the temperature sensor");
    terminal->println(HELP, "config dhcp [0|1]      ", "- 0, turns off DHCP; 1, turns on DHCP");
    terminal->println(HELP, "config ip [n] [n] [n] [n]     ", "- Sets the IP address n.n.n.n");
    terminal->println(HELP, "config dns [n] [n] [n] [n]    ", "- Sets the DNS address n.n.n.n");
    terminal->println(HELP, "config gw [n] [n] [n] [n]     ", "- Sets the Gateway address n.n.n.n");
    terminal->println(HELP, "config subnet [n] [n] [n] [n] ", "- Sets the Subnet Mask n.n.n.n");
    terminal->println(HELP, "config help/?          ", "- Print config Help");
    terminal->println();
    terminal->println(HELP, "Note: Addresses use a space seperator, so "
                            "\"192.168.168.4\" is \"192 168 168 4\"");
    terminal->println(HELP, "      Must Reboot the system for some changes to take effect");
  }
  terminal->println((commandComplete) ? PASSED : FAILED, "Command Complete");
  terminal->prompt();
}

void exportMemory(OutputInterface* terminal) {
  POWER_DATA->exportMem();
  terminal->println(PASSED, "Export Complete.");
  terminal->prompt();
}

void PowerMemory::exportMem() {
  Export exportMem(MEMORY_CONFIG_FILE);
  exportMem.exportData("mac", POWER_MEMORY.macAddress, 6);
  exportMem.exportData("dhcp", POWER_MEMORY.isDHCP);
  exportMem.exportData("ip", POWER_MEMORY.ipAddress, 4);
  exportMem.exportData("dnsAddress", POWER_MEMORY.dnsAddress, 4);
  exportMem.exportData("subnetMask", POWER_MEMORY.subnetMask, 4);
  exportMem.exportData("gatewayAddress", POWER_MEMORY.gatewayAddress, 4);
  exportMem.exportData("numberOfDevices", POWER_MEMORY.numberOfDevices);
  exportMem.exportData("drift", POWER_MEMORY.drift);
  for (int i = 0; i < NUM_DEVICES; i++) {
    exportMem.exportData("deviceName" + String(i), String(POWER_MEMORY.deviceName[i]));
  }
  exportMem.close();
}

void importMemory(OutputInterface* terminal) {
  POWER_DATA->importMem();
  terminal->println(PASSED, "Import Complete.");
  terminal->prompt();
}

void PowerMemory::importMem() {
  Import importMem(MEMORY_CONFIG_FILE);
  String parameter;
  String value;
  while (importMem.importParameter(&parameter)) {
    if (parameter == "mac")
      importMem.importData(POWER_MEMORY.macAddress, 6);
    else if (parameter == "dhcp")
      importMem.importData(&POWER_MEMORY.isDHCP);
    else if (parameter == "ip")
      importMem.importData(POWER_MEMORY.ipAddress, 4);
    else if (parameter == "dnsAddress")
      importMem.importData(POWER_MEMORY.dnsAddress, 4);
    else if (parameter == "subnetMask")
      importMem.importData(POWER_MEMORY.subnetMask, 4);
    else if (parameter == "gatewayAddress")
      importMem.importData(POWER_MEMORY.gatewayAddress, 4);
    else if (parameter == "numberOfDevices")
      importMem.importData(&POWER_MEMORY.numberOfDevices);
    else if (parameter == "drift")
      importMem.importData(&POWER_MEMORY.drift);
    else if (parameter.startsWith("deviceName")) {
      int nameIndex = parameter.substring(String("deviceName").length()).toInt();
      importMem.importData(&value);
      strncpy(POWER_MEMORY.deviceName[nameIndex], value.c_str(), NAME_MAX_LENGTH);
    } else {
      importMem.importData(&value);
      CONSOLE->println(ERROR, "Unknown Parameter in File: " + parameter + " - " + value);
    }
  }

  EEPROM_FORCE;
}
