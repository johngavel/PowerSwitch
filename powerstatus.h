#ifndef __GAVEL_POWERSTATUS_H
#define __GAVEL_POWERSTATUS_H

#include "powerdef.h"

#include <GavelInterfaces.h>

class PowerStatus : public IMemory {
public:
  PowerStatus() : IMemory("Power Status"){};

  // Virtual IMemory Methods
  typedef struct {
    bool status[NUM_DEVICES];
    byte spare[16 - sizeof(status)];
  } PowerSwitchData;

  static_assert(sizeof(PowerSwitchData) == 16, "ProgramMemory size unexpected - check packing/padding.");

  typedef union {
    PowerSwitchData data;
    unsigned char buffer[sizeof(PowerSwitchData)];
  } PowerSwitchUnion;
  PowerSwitchUnion memory;

  virtual const unsigned char& operator[](std::size_t index) const override { return memory.buffer[index]; };
  virtual unsigned char& operator[](std::size_t index) override { return memory.buffer[index]; };
  virtual std::size_t size() const noexcept override { return sizeof(PowerSwitchUnion); };
  virtual void initMemory() override {
    for (int i = 0; i < NUM_DEVICES; i++) memory.data.status[i] = false;
  };
  virtual void printData(OutputInterface* terminal) override {
    StringBuilder sb;
    for (byte i = 0; i < NUM_DEVICES; i++) {
      sb = "Device [";
      sb + i + "]: " + memory.data.status[i];
      terminal->println(INFO, sb.c_str());
    }
  }
  virtual void updateExternal() override { setInternal(true); };
  virtual JsonDocument createJson() override {
    StringBuilder sb;
    JsonDocument doc;
    doc["devicesstatus"] = NUM_DEVICES;
    for (byte i = 0; i < NUM_DEVICES; i++) {
      sb = "dev_power_";
      sb + i;
      doc[sb.c_str()] = memory.data.status[i];
    }
    return doc;
  };
  virtual bool parseJson(JsonDocument& doc) override {
    StringBuilder sb;
    for (byte i = 0; i < NUM_DEVICES; i++) {
      sb = "dev_power_";
      sb + i;
      if (!doc[sb.c_str()].isNull()) { command[i] = doc[sb.c_str()]; }
    }
    return true;
  };

  bool getStatus(unsigned int index) {
    if (index >= NUM_DEVICES) return false;
    return memory.data.status[index];
  }
  void setStatus(unsigned int index, bool value) {
    if (index >= NUM_DEVICES) return;
    memory.data.status[index] = value;
    setInternal(true);
  }
  bool getCommand(unsigned int index) {
    if (index >= NUM_DEVICES) return false;
    return command[index];
  }
  void setCommand(unsigned int index, bool value) {
    if (index >= NUM_DEVICES) return;
    command[index] = value;
  }

private:
  bool command[NUM_DEVICES];
};

#endif // __GAVEL_POWERCONFIG_H
