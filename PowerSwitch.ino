#include "files/webpage_all.h"
#include "powerswitch.h"

#include <GavelEEProm.h>
#include <GavelEthernet.h>
#include <GavelI2CWire.h>
#include <GavelPico.h>
#include <GavelSPIWire.h>
#include <GavelScan.h>
#include <GavelScreen.h>
#include <GavelServer.h>
#include <GavelServerStandard.h>
#include <GavelTCA9555.h>
#include <GavelTelnet.h>
#include <GavelTemperature.h>

#define DHT_11_PIN 28
#define TCA_ADDRESS 0x27

const char* ProgramInfo::AppName = "Power Switch";
const char* ProgramInfo::ShortName = "power";
const unsigned char ProgramInfo::ProgramNumber = 0x01;
const unsigned char ProgramInfo::MajorVersion = 0x02;
const unsigned char ProgramInfo::MinorVersion = 0x00;
const char* ProgramInfo::AuthorName = "John J. Gavel";

Scan scan;
EthernetModule ethernetModule;
EEpromMemory memory;
TelnetModule telnet;
ServerModule server;
Temperature temperature;
Tca9555Backend tca9555(TCA_ADDRESS);
Screen screen;
PowerSwitch powerswitch;

#define SCREEN_TAB "   "
class WiredScreen : public RefreshScreen {
public:
  WiredScreen(VirtualNetwork* __network) : network(__network){};
  virtual void screen() {
    String ipString = "0.0.0.0";
    String appString = String("Ver. ") + String(ProgramInfo::MajorVersion) + String(".") +
                       String(ProgramInfo::MinorVersion) + String(".") + String(ProgramInfo::BuildVersion);
    String temperatureString = "";
    if (temperature.validTemperature())
      temperatureString = "Temperature: " + (String(temperature.getTemperature())) + " F";
    if (network) { ipString = network->getIPAddress().toString(); }
    String indexString = "";
    for (byte i = 0; i < NUM_DEVICES; i++) {
      indexString += String(i + 1);
      indexString += SCREEN_TAB;
    }
    String statusString = "";
    for (byte i = 0; i < NUM_DEVICES; i++) {
      if (powerswitch.powerStatus.getStatus(i) == false)
        statusString += " ";
      else
        statusString += "*";
      statusString += SCREEN_TAB;
    }
    getScreen()->setScreen(String(ProgramInfo::AppName), appString, temperatureString, ipString, "", indexString, "",
                           statusString);
  };

private:
  VirtualNetwork* network = nullptr;
};

void setupPowerSwitch() {
  ArrayDirectory* dir;

  // Only one of these is needed for Scan!
  // scan.addCmd(TERM_CMD);
  taskManager.add(&scan);

  memory.setData(&powerswitch.powerConfig);
  memory.setData(&powerswitch.powerStatus);
  taskManager.add(&powerswitch);
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www"));
  // dir->addFile(new StaticFile(dhcpconfightml_string, dhcpconfightml, dhcpconfightml_len));
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www/api"));
  dir->addFile(new JsonFile(&powerswitch.powerConfig, "power-config.json", READ_WRITE));
  dir->addFile(new JsonFile(&powerswitch.powerStatus, "power-status.json", READ_WRITE));
  dir->addFile(new JsonFile(&temperature, "temperature.json", READ_WRITE));

  // dir = static_cast<ArrayDirectory*>(fileSystem.open("/www/js"));
  // dir->addFile(new StaticFile(dhcptablejs_string, dhcptablejs, dhcptablejs_len));
  // dir->addFile(new StaticFile(dhcpconfigjs_string, dhcpconfigjs, dhcpconfigjs_len));
  license.addLibrary(DHT_SENSOR_LIBRARY_INDEX);
  license.addLibrary(TCA9555_INDEX);
  license.addLibrary(ADAFRUIT_BUSIO_INDEX);
  license.addLibrary(ADAFRUIT_GFX_LIBRARY_INDEX);
  license.addLibrary(ADAFRUIT_SSD1306_INDEX);
  license.addLibrary(ADAFRUIT_UNIFIED_SENSOR_INDEX);

  screen.setRefreshScreen(new WiredScreen(&ethernetModule), 500);
  screen.setSplashScreen(JAXSON, String(ProgramInfo::AppName) + " v" + String(ProgramInfo::MajorVersion) + String(".") +
                                     String(ProgramInfo::MinorVersion));

  taskManager.add(&screen);
  hardwareList.add(&screen);
}

// ---------- Setup / Loop ----------
void setup() {
  setup0Start(TERM_CMD);

  setup0SerialPort(0, 1);
  i2cWire.begin(4, 5);
  spiWire.begin(18, 19, 16, 17);

  gpioManager.addDevice(&tca9555);
  temperature.configure(DHT_11_PIN);

  ethernetModule.configure();
  ethernetModule.allowDHCP(true);
  license.addLibrary(ETHERNET_INDEX);

  memory.configure(I2C_DEVICESIZE_24LC16);
  pico.rebootCallBacks.addCallback([&]() { (void) memory.forceWrite(); });

  memory.setData(&programMem);
  memory.setData(ethernetModule.getMemory());
  memory.setData(&temperature);

  StringBuilder sb = ProgramInfo::ShortName;
  sb + ":\\> ";
  telnet.configure(ethernetModule.getServer(TELNET_PORT), sb.c_str(), banner);

  loadServerStandard(&programMem, &hardwareList, &license, ethernetModule.getMemory(), &server, &fileSystem,
                     &taskManager, &memory, &pico);
  ArrayDirectory* dir = static_cast<ArrayDirectory*>(fileSystem.open("/www"));
  dir->addFile(new StaticFile("favicon.ico", faviconblueico, faviconblueico_len));
  dir->addFile(new StaticFile(indexhtml_string, indexhtml, indexhtml_len));
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www/js"));
  dir->addFile(new StaticFile(powerjs_string, powerjs, powerjs_len));

  server.configure(ethernetModule.getServer(HTTP_PORT), &fileSystem);

  hardwareList.add(&memory);
  hardwareList.add(&ethernetModule);
  hardwareList.add(&temperature);
  hardwareList.add(&tca9555);

  taskManager.add(&memory);
  taskManager.add(&ethernetModule);
  taskManager.add(&server);
  taskManager.add(&telnet);
  taskManager.add(&temperature);

  // gpioManager.setCore(1);

  setupPowerSwitch();

  setup0Complete();
}

void setup1() {
  setup1Start();
  setup1Complete();
}

void loop() {
  loop_0();
}

void loop1() {
  loop_1();
}