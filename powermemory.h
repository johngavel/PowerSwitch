#ifndef __POWER_MEMORY
#define __POWER_MEMORY
#include <eeprom.h>

#define NUM_DEVICES 5
#define DEBUG_LEDS 6
#define NAME_MAX_LENGTH 15
#define POWER_DATA ((PowerMemory*) EEPROM->getData())
#define POWER_MEMORY POWER_DATA->memory.mem

typedef enum { HWC_COMPUTER, HWC_DISPLAY, HWC_GPIO, HWC_TEMPERATURE, HWC_ETHERNET, HWC_EEPROM, HWC_MAX } HWC_COMPONENTS;

class PowerMemory : public Data {
public:
  struct MemoryStruct {
    byte macAddress[6];
    bool isDHCP;
    byte ipAddress[4];
    byte dnsAddress[4];
    byte subnetMask[4];
    byte gatewayAddress[4];
    byte numberOfDevices;
    char deviceName[NUM_DEVICES][NAME_MAX_LENGTH + 1];
    int drift;
  };

  typedef union {
    MemoryStruct mem;
    byte memoryArray[sizeof(MemoryStruct)];
  } MemoryUnion;

  MemoryUnion memory;
  void setup();
  void initMemory();
  void printData();
  unsigned char* getData();
  unsigned long getLength();

  byte getNumberOfDevices() { return memory.mem.numberOfDevices; };
  void setDeviceName(byte device, const char* name, int length);
  char* getDeviceName(byte device);

  bool getCurrentRelayStatus(int i) { return currentRelayStatus[i]; };
  void setCurrentRelayStatus(int i, bool status) { currentRelayStatus[i] = status; };
  bool getOnline(HWC_COMPONENTS i) { return onlineStatus[i]; };
  void setOnline(HWC_COMPONENTS i, bool status) { onlineStatus[i] = status; };

private:
  char ErrorString[NAME_MAX_LENGTH];
  bool currentRelayStatus[NUM_DEVICES];
  bool onlineStatus[HWC_MAX];
};

#endif