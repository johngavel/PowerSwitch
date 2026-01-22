#ifndef __GAVEL_POWERCONFIG_H
#define __GAVEL_POWERCONFIG_H

#include "powerdef.h"

#include <GavelInterfaces.h>

class PowerConfig : public IMemory {
public:
  PowerConfig() : IMemory("Power Config"){};

  // Virtual IMemory Methods
  typedef struct {
    byte numberOfDevices;
    char deviceName[NUM_DEVICES][NAME_STRING_LENGTH];
    byte spare[96 - sizeof(numberOfDevices) - sizeof(deviceName)];
  } PowerSwitchData;

  static_assert(sizeof(PowerSwitchData) == 96, "ProgramMemory size unexpected - check packing/padding.");

  typedef union {
    PowerSwitchData data;
    unsigned char buffer[sizeof(PowerSwitchData)];
  } PowerSwitchUnion;
  PowerSwitchUnion memory;

  virtual const unsigned char& operator[](std::size_t index) const override { return memory.buffer[index]; };
  virtual unsigned char& operator[](std::size_t index) override { return memory.buffer[index]; };
  virtual std::size_t size() const noexcept override { return sizeof(PowerSwitchUnion); };
  virtual void initMemory() override {
    memory.data.numberOfDevices = NUM_DEVICES;
    memset(memory.data.deviceName, 0, (NUM_DEVICES * NAME_STRING_LENGTH));
    for (int i = 0; i < NUM_DEVICES; i++) { snprintf(memory.data.deviceName[i], NAME_STRING_LENGTH, "Device_%d", i); }
  };
  virtual void printData(OutputInterface* terminal) override {
    StringBuilder sb;
    sb = "Number of Devices: ";
    sb + memory.data.numberOfDevices;
    terminal->println(INFO, sb.c_str());
    for (byte i = 0; i < memory.data.numberOfDevices; i++) {
      sb = "Device [";
      sb + i + "]: " + memory.data.deviceName[i];
      terminal->println(INFO, sb.c_str());
    }
  }
  virtual void updateExternal() override { setInternal(true); };
  virtual JsonDocument createJson() override {
    JsonDocument doc;
    doc["devicesconfig"] = memory.data.numberOfDevices;
    doc["namelength"] = NAME_MAX_LENGTH;
    JsonArray data = doc["names"].to<JsonArray>();
    for (byte i = 0; i < memory.data.numberOfDevices; i++) { data[i] = memory.data.deviceName[i]; }
    return doc;
  };
  virtual bool parseJson(JsonDocument& doc) override {
    if (!doc["names"].isNull()) {
      JsonArray data = doc["names"].as<JsonArray>();
      for (byte i = 0; i < memory.data.numberOfDevices; i++) {
        if (!data[i].isNull()) {
          const char* name = data[i];
          setName(i, name);
        }
      }
    }
    setInternal(true);
    return true;
  };
  const char* getName(unsigned int index) {
    if (index >= NUM_DEVICES) return "";
    return memory.data.deviceName[index];
  }

  void setName(unsigned int index, const char* name) {
    if (index >= NUM_DEVICES) return;
    memset(memory.data.deviceName[index], 0, NAME_MAX_LENGTH);
    strncpy(memory.data.deviceName[index], name, NAME_STRING_LENGTH);
    setInternal(true);
  }

private:
};

#endif // __GAVEL_POWERCONFIG_H
