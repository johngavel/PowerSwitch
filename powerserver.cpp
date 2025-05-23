#include "powerserver.h"

#include "powermemory.h"

#include <commonhtml.h>
#include <gpio.h>
#include <serialport.h>
#include <servermodule.h>
#include <stringutils.h>
#include <temperature.h>
#include <watchdog.h>

static ErrorPage errorPage;
static CodePage codePage("PowerSwitch", "https://github.com/johngavel/PowerSwitch");
static UploadPage uploadPage;
static ExportPage exportPage;
static ImportPage importPage;
static UpgradePage upgradePage;
static RebootPage rebootPage;
static UpgradeProcessingFilePage upgradeProcessingFilePage;
static UploadProcessingFilePage uploadProcessingFilePage;
static ImportProcessingFilePage importProcessingFilePage;

class FrontPage : public BasicPage {
public:
  FrontPage() {
    setPageName("index");
    refresh = 10;
  };
  virtual void conductAction() = 0;
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    conductAction();
    sendPageBegin(html, true, refresh);
    if (TEMPERATURE->validTemperature()) {
      html->brTag();
      html->print("Temperature ");
      html->print(TEMPERATURE->getTemperature());
      html->println(" F");
      html->brTag();
    }

    html->openTag("table", "class=\"center\"");
    for (byte i = 0; i < NUM_DEVICES; i++) {
      html->openTrTag()->openTdTag();
      html->print("Power ");
      html->print(i + 1);
      html->print(". ");
      html->closeTag();
      html->openTdTag()->print(POWER_DATA->getDeviceName(i));
      html->closeTag();
      html->openTdTag();
      if (POWER_DATA->getCurrentRelayStatus(i) == HIGH) {
        String off = "href=\"/" + String(i + 1) + "/off\"";
        html->openTag("a", off);
        html->openTag("button", "class=\"button\"");
        html->print("ON")->closeTag()->closeTag();
      } else {
        String on = "href=\"/" + String(i + 1) + "/on\"";
        html->openTag("a", on);
        html->openTag("button", "class=\"button2 button\"");
        html->print("OFF")->closeTag()->closeTag();
      }
      html->closeTag()->closeTag();
    }
    html->closeTag()->println();
    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()->openTag("a", "href=\"/configname\"")->print("Configure Devices")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/server\"")->print("Server Control")->closeTag()->closeTag()->closeTag()->closeTag()->println();

    sendPageEnd(html);
    return html;
  }
  int refresh;
};

class RootPage : public FrontPage {
  void conductAction() {};
} indexPage;

class ButtonMinPage : public BasicPage {
public:
  ButtonMinPage(int __device, bool __status) : device(__device), status(__status){};
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    if (GPIO->getPin(GPIO_INPUT, device)->getCurrentStatus() != status) GPIO->getPin(GPIO_PULSE, device)->setCurrentStatus(true);
    html->println((status == true) ? "on" : "off");
    return html;
  };
  int device;
  bool status;
};

class StatMinPage : public BasicPage {
public:
  StatMinPage(int __device) : device(__device){};
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    html->println((GPIO->getPin(GPIO_INPUT, device)->getCurrentStatus()) ? "on" : "off");
    return html;
  };
  int device;
};

class ButtonPage : public FrontPage {
public:
  ButtonPage(int __device, bool __status) : device(__device), status(__status) { refresh = 1; };
  void conductAction() {
    if (GPIO->getPin(GPIO_INPUT, device)->getCurrentStatus() != status) GPIO->getPin(GPIO_PULSE, device)->setCurrentStatus(true);
  };
  int device;
  bool status;
};

class ServerPage : public BasicPage {
public:
  ServerPage() { setPageName("server"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    String versionString =
        "Ver. " + String(ProgramInfo::MajorVersion) + String(".") + String(ProgramInfo::MinorVersion) + String(".") + String(ProgramInfo::BuildVersion);
    String buildString = "Build Date: " + String(ProgramInfo::compileDate) + " Time: " + String(ProgramInfo::compileTime);
    String authorString = "Author: " + String(ProgramInfo::AuthorName);

    html->openTag("h2")->print(ProgramInfo::AppName)->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()->tdTag(versionString)->closeTag();
    html->openTrTag()->tdTag(buildString)->closeTag();
    html->openTrTag()->tdTag(authorString)->closeTag()->closeTag();
    html->brTag()->println();

    html->openTag("h2")->print("MAC Address")->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()->tdTag("MAC Address:")->tdTag(getMacString(POWER_MEMORY.macAddress))->closeTag();
    html->closeTag();
    html->brTag()->println();

    html->openTag("h2")->print("IP Configuration")->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()->tdTag("IP Configuration:")->tdTag(String((POWER_MEMORY.isDHCP) ? "DHCP" : "Static"))->closeTag();
    IPAddress ipAddress = NIC->getIPAddress();
    html->openTrTag()->tdTag("IP Address:")->tdTag(ipAddress.toString())->closeTag()->println();
    ipAddress = NIC->getDNS();
    html->openTrTag()->tdTag("DNS Server:")->tdTag(ipAddress.toString())->closeTag()->println();
    ipAddress = NIC->getSubnetMask();
    html->openTrTag()->tdTag("Subnet Mask:")->tdTag(ipAddress.toString())->closeTag()->println();
    ipAddress = NIC->getGateway();
    html->openTrTag()->tdTag("Gateway:")->tdTag(ipAddress.toString())->closeTag()->println();
    html->closeTag();
    html->brTag()->println();

    html->openTag("h2")->print("Power Status")->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    for (int i = 0; i < POWER_DATA->getNumberOfDevices(); i++) {
      html->openTrTag();
      html->tdTag("Power ")->tdTag(String(i + 1))->tdTag(String(POWER_DATA->getDeviceName(i)));

      if (POWER_DATA->getCurrentRelayStatus(i) == HIGH) {
        html->tdTag("ON");
      } else {
        html->tdTag("OFF");
      }
      html->closeTag();
    }
    html->closeTag();
    html->brTag()->println();

    html->openTag("h2")->print("Hardware Status")->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()->tdTag("Pico")->tdTag(String((POWER_DATA->getOnline(HWC_COMPUTER)) ? "ON" : "OFF"))->closeTag();
    html->openTrTag()->tdTag("Display")->tdTag(String((POWER_DATA->getOnline(HWC_DISPLAY)) ? "ON" : "OFF"))->closeTag();
    html->openTrTag()->tdTag("GPIO Ex")->tdTag(String((POWER_DATA->getOnline(HWC_GPIO)) ? "ON" : "OFF"))->closeTag();
    html->openTrTag()->tdTag("Temperature")->tdTag(String((POWER_DATA->getOnline(HWC_TEMPERATURE)) ? "ON" : "OFF"))->closeTag();
    html->openTrTag()->tdTag("Ethernet")->tdTag(String((POWER_DATA->getOnline(HWC_ETHERNET)) ? "ON" : "OFF"))->closeTag();
    html->openTrTag()->tdTag("EEPROM")->tdTag(String((POWER_DATA->getOnline(HWC_EEPROM)) ? "ON" : "OFF"))->closeTag();
    html->closeTag();
    html->brTag()->println();

    html->openTag("table", "class=\"center\"");
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/ipconfig\"")->print("Configure IP Addresses")->closeTag()->closeTag()->closeTag();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/import\"")->print("Import Switch Configuration")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/export\"")->print("Export Switch Configuration")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/upgrade\"")->print("Upgrade the Power Switch")->closeTag()->closeTag()->closeTag();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/code\"")->print("Source Code of the Pico Power Switch")->closeTag()->closeTag()->closeTag();
    html->closeTag();
    html->brTag()->println();

    html->brTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->openTdTag()
        ->openTag("a", "href=\"/reboot\"")
        ->openTag("button", "type=\"button\" class=\"button\"")
        ->print("REBOOT")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    sendPageEnd(html);
    return html;
  }
} serverPage;

class ConfigNamePage : public ProcessPage {
public:
  ConfigNamePage() { setPageName("configname"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->brTag()->print("Configuration Saved")->brTag()->brTag();
    html->openTag("form", "action=\"/configname\" method=\"GET\"")->println();
    html->openTag("table", "class=\"center\"")->println();
    for (byte i = 0; i < NUM_DEVICES; i++) {
      html->openTrTag()->tdTag("Power " + String(i + 1) + ". ");
      html->tdTag(POWER_DATA->getDeviceName(i));

      String inputOption = "type=\"text\" maxlength=\"15\" pattern=\"[^\\s]+\" value=\"";
      inputOption += POWER_DATA->getDeviceName(i);
      inputOption += "\" name=\"";
      inputOption += String(i + 1) + "\"";
      html->openTdTag()->closeTag("input", inputOption);
      html->closeTag()->closeTag();
    }
    html->closeTag();

    html->brTag();
    html->print("(No whitespace, max length is 15 characters)")->brTag()->brTag()->println();

    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()
        ->openTag("button", "type=\"submit\" class=\"button4 button\"")
        ->print("Submit")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("button", "type=\"reset\" class=\"button\"")
        ->print("Reset")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    html->closeTag();
    html->closeTag();

    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  }

  void processParameterList() {
    for (int i = 0; i < LIST->getCount(); i++) {
      int deviceNumber = LIST->getParameter(i).toInt() - 1;
      POWER_DATA->setDeviceName(deviceNumber, LIST->getValue(i).c_str(), LIST->getValue(i).length());
    }
    parametersProcessed = true;
    EEPROM_FORCE;
  }
} configNamePage;

class ConfigIPPage : public ProcessPage {
public:
  ConfigIPPage() { setPageName("ipconfig"); };

  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->brTag()->print("Configuration Saved")->brTag()->brTag();
    html->openTag("form", "action=\"/ipconfig\" method=\"GET\"")->println();
    html->openTag("table", "class=\"center\"");

    bool isDhcp = POWER_MEMORY.isDHCP;
    html->openTag("fieldset");
    html->openTag("legend")->print("Select IP Address Source")->closeTag()->println();
    html->closeTag("input", "type=\"radio\" id=\"dhcp0\" name=\"dhcp\" value=\"0\"" + String((!isDhcp) ? " checked" : ""));
    html->openTag("label", "for=\"dhcp0\"")->print("Static IP")->closeTag();
    html->closeTag("input", "type=\"radio\" id=\"dhcp1\" name=\"dhcp\" value=\"1\"" + String((isDhcp) ? " checked" : ""));
    html->openTag("label", "for=\"dhcp1\"")->print("DHCP")->closeTag();
    html->closeTag();

    html->print("<table class=\"center\">");

    byte* address = POWER_MEMORY.ipAddress;
    html->openTrTag()->tdTag("IPAddress");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"ip0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"ip1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"ip2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"ip3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = POWER_MEMORY.subnetMask;
    html->openTrTag()->tdTag("Subnet Mask");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"sm0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"sm1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"sm2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"sm3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = POWER_MEMORY.gatewayAddress;
    html->openTrTag()->tdTag("Gateway Address");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"ga0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"ga1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"ga2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"ga3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = POWER_MEMORY.dnsAddress;
    html->openTrTag()->tdTag("DNS Address");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"da0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"da1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"da2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"da3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()
        ->openTag("button", "type=\"submit\" class=\"button4 button\"")
        ->print("Submit")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("button", "type=\"reset\" class=\"button\"")
        ->print("Reset")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    html->closeTag();
    html->closeTag();
    html->closeTag();
    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  }

  void processParameterList() {
    for (int i = 0; i < LIST->getCount(); i++) {
      if (LIST->getParameter(i).equals("dhcp"))
        POWER_MEMORY.isDHCP = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip0"))
        POWER_MEMORY.ipAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip1"))
        POWER_MEMORY.ipAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip2"))
        POWER_MEMORY.ipAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip3"))
        POWER_MEMORY.ipAddress[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm0"))
        POWER_MEMORY.subnetMask[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm1"))
        POWER_MEMORY.subnetMask[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm2"))
        POWER_MEMORY.subnetMask[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm3"))
        POWER_MEMORY.subnetMask[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga0"))
        POWER_MEMORY.gatewayAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga1"))
        POWER_MEMORY.gatewayAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga2"))
        POWER_MEMORY.gatewayAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga3"))
        POWER_MEMORY.gatewayAddress[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da0"))
        POWER_MEMORY.dnsAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da1"))
        POWER_MEMORY.dnsAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da2"))
        POWER_MEMORY.dnsAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da3"))
        POWER_MEMORY.dnsAddress[3] = LIST->getValue(i).toInt();
      else
        CONSOLE->println(ERROR, "Unknown parameter when processing IP Configuration Page.");
    }
    parametersProcessed = true;
    EEPROM_FORCE;
  }
} configIPPage;

void setupServerModule() {
  SERVER->setRootPage(&indexPage);
  SERVER->setUpgradePage(&upgradeProcessingFilePage);
  SERVER->setUploadPage(&uploadProcessingFilePage);
  SERVER->setUploadPage(&importProcessingFilePage);
  SERVER->setErrorPage(&errorPage);
  SERVER->setPage(&indexPage);
  SERVER->setPage(&serverPage);
  SERVER->setPage(&codePage);
  SERVER->setPage(&upgradePage);
  SERVER->setPage(&uploadPage);
  SERVER->setPage(&exportPage);
  SERVER->setPage(&importPage);
  SERVER->setPage(&rebootPage);
  SERVER->setPage(&configNamePage);
  SERVER->setPage(&configIPPage);
  SERVER->setFormProcessingPage(&configIPPage);
  SERVER->setFormProcessingPage(&configNamePage);
  for (int i = 1; i <= NUM_DEVICES; i++) {
    ButtonPage* page = new ButtonPage(i, true);
    page->setPageName(String(String(i) + "/on").c_str());
    SERVER->setPage(page);
    page = new ButtonPage(i, false);
    page->setPageName(String(String(i) + "/off").c_str());
    SERVER->setPage(page);
    ButtonMinPage* page2 = new ButtonMinPage(i, true);
    page2->setPageName(String("on/" + String(i)).c_str());
    SERVER->setPage(page2);
    page2 = new ButtonMinPage(i, false);
    page2->setPageName(String("off/" + String(i)).c_str());
    SERVER->setPage(page2);
    StatMinPage* page3 = new StatMinPage(i);
    page3->setPageName(String("stat/" + String(i)).c_str());
    SERVER->setPage(page3);
  }
}
