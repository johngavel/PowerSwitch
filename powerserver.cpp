#include "powerserver.h"

#include "powermemory.h"

#include <gpio.h>
#include <serialport.h>
#include <servermodule.h>
#include <temperature.h>
#include <watchdog.h>

static void sendPageBegin(HTMLBuilder* html, bool autoRefresh = false, int seconds = 10) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  html->println("HTTP/1.1 200 OK");
  html->println("Content-type:text/html");
  html->println("Connection: close");
  html->println();

  // Display the HTML web page
  html->println("<!DOCTYPE html><html>");
  html->print("<head>");
  if (autoRefresh) {
    html->print("<meta http-equiv=\"refresh\" content=\"");
    html->print(seconds);
    html->print("; url=http://");
    html->print(ETHERNET->getIPAddress().toString());
    html->println("/\"; name=\"viewport\" content=\"width=device-width, "
                  "initial-scale=1\" >");
  }
  // CSS to style the on/off buttons
  html->println("<style>html { font-family: courier; font-size: 24px; display: "
                "inline-block; margin: 0px auto; text-align: center;}");
  html->println(".button { background-color: red; border: none; color: white; "
                "padding: 16px 40px;");
  html->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  html->println(".button2 {background-color: black;}");
  html->println(".button3 {background-color: grey;}");
  html->println(".button4 {background-color: green;}");
  html->println(".center {margin-left: auto; margin-right: auto;}");
  html->println("footer {");
  html->println("text-align: center;font-size: 16px;");
  html->println("padding: 3px;");
  html->println("background-color: MediumSeaGreen;");
  html->println("color: white;");
  html->println("}</style>");
  html->print("<title>");
  html->print(ProgramInfo::AppName);
  html->println("</title></head>");
  html->print("<body><h1>");
  html->print(ProgramInfo::AppName);
  html->println("</h1>");
}

static void sendPageEnd(HTMLBuilder* html) {
  String versionString = "Ver. " + String(ProgramInfo::ProgramNumber) + String(".") +
                         String(ProgramInfo::MajorVersion) + String(".") + String(ProgramInfo::MinorVersion);

  html->println("<footer><p>");
  html->println(ProgramInfo::AppName);
  html->println("<br>" + versionString);
  html->println("<br>Build Date: " + String(ProgramInfo::compileDate) + " Time: " + String(ProgramInfo::compileTime));
  html->println("<br>Author: John J. Gavel<br>");
  html->println("</p></footer>");
  html->println("</body></html>");
  html->println();
}

class TemplatePage : public BasicPage {
public:
  TemplatePage() { setPageName("template"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    html->println("<h2>Template<h2/>");
    sendPageEnd(html);
    return html;
  }
} templatePage;

class ErrorPage : public BasicPage {
public:
  ErrorPage() { setPageName("error"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html, true, 5);
    html->println("<section>");
    html->println("  <div class=\"container\">");
    html->println("   <div><img class=\"image\" src=\"/errorimg.jpg\" alt=\"\"></div>");
    html->println("  </div>");
    html->println("  </div>");
    html->println("</section>");
    sendPageEnd(html);
    return html;
  }
} errorPage;

class RootPage : public BasicPage {
public:
  virtual void conductAction() = 0;
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    conductAction();
    sendPageBegin(html, true, refresh);
    if (TEMPERATURE->validTemperature()) {
      html->print("<p>Temperature ");
      html->print(TEMPERATURE->getTemperature());
      html->println(" F</p>");
    }

    html->print("<table class=\"center\">");
    for (byte i = 0; i < NUM_DEVICES; i++) {
      html->print("<tr><td><p>Power ");
      html->print(i + 1);
      html->print(". </td><td>");
      html->print(POWER_DATA->getDeviceName(i));
      html->print("</td></p></td><td><a href=\"/");
      html->print(i + 1);
      if (POWER_DATA->getCurrentRelayStatus(i) == HIGH) {
        html->println("/off\"><button class=\"button\">ON</button></a></td></tr></p>");
      } else {
        html->println("/on\"><button class=\"button2 "
                      "button\">OFF</button></a></td></tr>");
      }
    }
    html->println("</table>");
    html->println("<table class=\"center\"><tr><td><a "
                  "href=\"/configname\">Configure Devices</a></td>");
    html->println("<td><a href=\"/server\">Server Control</a></td></tr></table>");
    sendPageEnd(html);
    return html;
  }
  int refresh;
};

class IndexPage : public RootPage {
public:
  IndexPage() {
    setPageName("index");
    refresh = 10;
  };
  void conductAction(){};
} indexPage;

class ButtonMinPage : public BasicPage {
public:
  ButtonMinPage(int __device, bool __status) : device(__device), status(__status){};
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    if (GPIO->getPin(GPIO_INPUT, device)->getCurrentStatus() != status)
      GPIO->getPin(GPIO_PULSE, device)->setCurrentStatus(true);
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

class ButtonPage : public RootPage {
public:
  ButtonPage(int __device, bool __status) : device(__device), status(__status) { refresh = 1; };
  void conductAction() {
    if (GPIO->getPin(GPIO_INPUT, device)->getCurrentStatus() != status)
      GPIO->getPin(GPIO_PULSE, device)->setCurrentStatus(true);
  };
  int device;
  bool status;
};

class ServerPage : public BasicPage {
public:
  ServerPage() { setPageName("server"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    String versionString = "Ver. " + String(ProgramInfo::ProgramNumber) + String(".") +
                           String(ProgramInfo::MajorVersion) + String(".") + String(ProgramInfo::MinorVersion);

    html->println("<h2>");
    html->println(ProgramInfo::AppName);
    html->println("</h2><table class=\"center\"> <tr><td>" + versionString + "</td></tr>");
    html->println("<tr><td>Build Date: " + String(ProgramInfo::compileDate) +
                  " Time: " + String(ProgramInfo::compileTime) + "</td></tr>");
    html->println("<tr><td>Author: John J. Gavel</td></tr></table>");
    html->println();

    html->println("<h2>MAC Address</h2><table class=\"center\"> <tr><td>MAC "
                  "Address:</td><td>" +
                  String(POWER_MEMORY.macAddress[0], HEX) + String(":") + String(POWER_MEMORY.macAddress[1], HEX) +
                  String(":") + String(POWER_MEMORY.macAddress[2], HEX) + String(":") +
                  String(POWER_MEMORY.macAddress[3], HEX) + String(":") + String(POWER_MEMORY.macAddress[4], HEX) +
                  String(":") + String(POWER_MEMORY.macAddress[5], HEX) + "</td></tr></table>");
    html->println("<h2>IP Configuration</h2><table class=\"center\">");
    html->print("<tr><td>IP Configuration:</td><td>" + String((POWER_MEMORY.isDHCP) ? "DHCP" : "Static") +
                "</td></tr>");
    IPAddress ipAddress = ETHERNET->getIPAddress();
    html->println("<tr><td>IP Address:</td><td>" + String(ipAddress[0]) + String(".") + String(ipAddress[1]) +
                  String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]) + "</td></tr>");
    ipAddress = ETHERNET->getDNS();
    html->println("<tr><td>DNS Server:</td><td>" + String(ipAddress[0]) + String(".") + String(ipAddress[1]) +
                  String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]) + "</td></tr>");
    ipAddress = ETHERNET->getSubnetMask();
    html->println("<tr><td>Subnet Mask:</td><td>" + String(ipAddress[0]) + String(".") + String(ipAddress[1]) +
                  String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]) + "</td></tr>");
    ipAddress = ETHERNET->getGateway();
    html->println("<tr><td>Gateway:</td><td>" + String(ipAddress[0]) + String(".") + String(ipAddress[1]) +
                  String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]) + "</td></tr></table>");

    html->println("<h2>Power Status</h2><table class=\"center\">");
    for (int i = 0; i < POWER_DATA->getNumberOfDevices(); i++) {
      html->print("<tr><td>Power </td><td>");
      html->print(String(i + 1));
      html->print(" </td><td>");
      html->print(String(POWER_DATA->getDeviceName(i)));
      html->println("</td>");
      if (POWER_DATA->getCurrentRelayStatus(i) == HIGH) {
        html->println("<td>ON</td></tr>");
      } else {
        html->println("<td>OFF</td></tr>");
      }
    }
    html->println("</table>");
    html->println("<h2>Hardware Status</h2><table class=\"center\">");
    html->println("<tr><td>Pico</td><td>" + String((POWER_DATA->getOnline(HWC_COMPUTER)) ? "ON" : "OFF") + "</td>");
    html->println("<tr><td>Display</td><td>" + String((POWER_DATA->getOnline(HWC_DISPLAY)) ? "ON" : "OFF") + "</td>");
    html->println("<tr><td>GPIO Ex</td><td>" + String((POWER_DATA->getOnline(HWC_GPIO)) ? "ON" : "OFF") + "</td>");
    html->println("<tr><td>Temperature</td><td>" + String((POWER_DATA->getOnline(HWC_TEMPERATURE)) ? "ON" : "OFF") +
                  "</td>");
    html->println("<tr><td>Ethernet</td><td>" + String((POWER_DATA->getOnline(HWC_ETHERNET)) ? "ON" : "OFF") + "</td>");
    html->println("<tr><td>EEPROM</td><td>" + String((POWER_DATA->getOnline(HWC_EEPROM)) ? "ON" : "OFF") + "</td>");
    html->println("</table>");
    html->println("<tr><a href=\"/ipconfig\">Configure IP Addresses</a></tr>");
    html->println("<br><tr><a href=\"/upgrade\">Upgrade the Power Switch</a></tr>");
    html->println("<table class=\"center\">");
    html->println("<td><a href=\"/\"><button type=\"button\" class=\"button2 "
                  "button\">Cancel</button></a></td></tr>");
    html->println("</table>");
    html->println("<table class=\"center\"><a href=\"/reboot\"><button "
                  "class=\"button\">REBOOT</button></a></table>");
    sendPageEnd(html);
    return html;
  }
} serverPage;

class UploadPage : public BasicPage {
public:
  UploadPage() { setPageName("upload"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    html->println("<h2>File Upload<h2/>");
    html->println("<form method=\"post\" enctype=\"multipart/form-data\">");
    html->println("<label for=\"file\">File</label>");
    html->println("<input id=\"file\" name=\"file\" type=\"file\" />");
    html->println("<button>Upload</button>");
    html->println("</form><br>");
    html->println("<td><a href=\"/server\"><button type=\"button\" "
                  "class=\"button2 button\">Cancel</button></a></td></tr><br>");
    sendPageEnd(html);
    return html;
  }
} uploadPage;

class UpgradePage : public BasicPage {
public:
  UpgradePage() { setPageName("upgrade"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    html->println("<h2>OTA Upgrade<h2/>");
    html->println("<form method=\"post\" enctype=\"multipart/form-data\">");
    html->println("<label for=\"file\">File</label>");
    html->println("<input id=\"file\" name=\"file\" type=\"file\" />");
    html->println("<button>Upload</button>");
    html->println("</form><br>");
    html->println("<td><a href=\"/server\"><button type=\"button\" "
                  "class=\"button2 button\">Cancel</button></a></td></tr><br>");
    sendPageEnd(html);
    return html;
  }
} upgradePage;

class RebootPage : public BasicPage {
public:
  RebootPage() { setPageName("reboot"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html, true, 15);
    html->println("<p>Rebooting.....</p>");
    sendPageEnd(html);
    PORT->println(WARNING, "Reboot in progress.....");
    WATCHDOG->reboot();
    return html;
  }
} rebootPage;

class ConfigNamePage : public ProcessPage {
public:
  ConfigNamePage() { setPageName("configname"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->println("<p>Configuration Saved</p>");
    html->println("<form action=\"/configname\" method=\"GET\">");
    html->print("<table class=\"center\">");
    for (byte i = 0; i < NUM_DEVICES; i++) {
      html->print("<tr><td><p>Power ");
      html->print(i + 1);
      html->print(". ");
      html->print(POWER_DATA->getDeviceName(i));
      html->print("</td><td>");
      html->print("<input type=\"text\" maxlength=\"15\" pattern=\"[^\\s]+\" value=\"");
      html->print(POWER_DATA->getDeviceName(i));
      html->print("\" name=\"");
      html->print(i + 1);
      html->print("\">");
      html->println("</td></tr>");
    }
    html->print("<tr><td><button type=\"submit\" class=\"button4 "
                "button\">Submit</button></td>");
    html->println("<td><button type=\"reset\" class=\"button\">Reset</button></td>");
    html->println("<td><a href=\"/\"><button type=\"button\" class=\"button2 "
                  "button\">Cancel</button></a></td></tr>");
    html->println("</table>");
    html->println("<p>(No whitespace, max length is 15 characters)</p>");
    html->println("</form>");
    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  }

  void processParameterList() {
    for (int i = 0; i < list.parameterCount; i++) {
      int deviceNumber = list.parameters[i].parameter.toInt() - 1;
      POWER_DATA->setDeviceName(deviceNumber, list.parameters[i].value.c_str(), list.parameters[i].value.length());
    }
    parametersProcessed = true;
    EEPROM->breakSeal();
  }
} configNamePage;

class ConfigIPPage : public ProcessPage {
public:
  ConfigIPPage() { setPageName("ipconfig"); };

  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->println("<p>Configuration Saved</p>");
    html->println("<form action=\"/ipconfig\" method=\"GET\">");

    bool isDhcp = POWER_MEMORY.isDHCP;
    html->println("<fieldset><legend>Select IP Address Source</legend>");
    html->print("<input type=\"radio\" id=\"dhcp0\" name=\"dhcp\" value=\"0\"");
    html->print((!isDhcp) ? " checked />" : " />");
    html->println("<label for=\"dhcp0\">Static IP</label>");
    html->print("<input type=\"radio\" id=\"dhcp1\" name=\"dhcp\" value=\"1\"");
    html->print((isDhcp) ? " checked />" : " />");
    html->println("<label for=\"dhcp1\">DHCP</label>");
    html->println("</fieldset>");

    html->print("<table class=\"center\">");

    byte* address = POWER_MEMORY.ipAddress;
    html->print("<tr><td>IP Address</td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[0]) + "\" name=\"ip0\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[1]) + "\" name=\"ip1\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[2]) + "\" name=\"ip2\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[3]) + "\" name=\"ip3\"\"></td></tr>");

    address = POWER_MEMORY.subnetMask;
    html->print("<tr><td>Subnet Mask</td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[0]) + "\" name=\"sm0\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[1]) + "\" name=\"sm1\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[2]) + "\" name=\"sm2\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[3]) + "\" name=\"sm3\"\"></td></tr>");

    address = POWER_MEMORY.gatewayAddress;
    html->print("<tr><td>Gateway Address</td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[0]) + "\" name=\"ga0\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[1]) + "\" name=\"ga1\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[2]) + "\" name=\"ga2\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[3]) + "\" name=\"ga3\"\"></td></tr>");

    address = POWER_MEMORY.dnsAddress;
    html->print("<tr><td>DNS Address</td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[0]) + "\" name=\"da0\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[1]) + "\" name=\"da1\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[2]) + "\" name=\"da2\"\"></td>");
    html->print("<td><input type=\"text\" maxlength=\"3\" size=\"3\" "
                "pattern=\"[^\\s]+\" value=\"" +
                String(address[3]) + "\" name=\"da3\"\"></td></tr>");

    html->print("</table><table class=\"center\"><tr><td><button type=\"submit\" "
                "class=\"button4 button\">Submit</button></td>");
    html->println("<td><button type=\"reset\" class=\"button\">Reset</button></td>");
    html->println("<td><a href=\"/\"><button type=\"button\" class=\"button2 "
                  "button\">Cancel</button></a></td></tr>");
    html->println("</table>");
    html->println("</form>");
    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  }

  void processParameterList() {
    for (int i = 0; i < list.parameterCount; i++) {
      if (list.parameters[i].parameter.equals("dhcp"))
        POWER_MEMORY.isDHCP = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip0"))
        POWER_MEMORY.ipAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip1"))
        POWER_MEMORY.ipAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip2"))
        POWER_MEMORY.ipAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip3"))
        POWER_MEMORY.ipAddress[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm0"))
        POWER_MEMORY.subnetMask[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm1"))
        POWER_MEMORY.subnetMask[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm2"))
        POWER_MEMORY.subnetMask[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm3"))
        POWER_MEMORY.subnetMask[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga0"))
        POWER_MEMORY.gatewayAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga1"))
        POWER_MEMORY.gatewayAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga2"))
        POWER_MEMORY.gatewayAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga3"))
        POWER_MEMORY.gatewayAddress[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da0"))
        POWER_MEMORY.dnsAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da1"))
        POWER_MEMORY.dnsAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da2"))
        POWER_MEMORY.dnsAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da3"))
        POWER_MEMORY.dnsAddress[3] = list.parameters[i].value.toInt();
      else
        PORT->println(ERROR, "Unknown parameter when processing IP Configuration Page.");
    }
    parametersProcessed = true;
    EEPROM->breakSeal();
  }
} configIPPage;

class UpgradeProcessingFilePage : public FilePage {
public:
  UpgradeProcessingFilePage() { setPageName("upgrade"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html, true, (success) ? 20 : 5);
    html->print("<p>");
    if (success)
      html->print("Processing File Upgrade....");
    else
      html->print("Processing File Upgrade FAILED....");
    html->println("</p>");
    sendPageEnd(html);
    return html;
  }
} upgradeProcessingFilePage;

class UploadProcessingFilePage : public FilePage {
public:
  UploadProcessingFilePage() { setPageName("upload"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html, true, 5);
    html->print("<p>");
    if (success)
      html->print("Processing File Upload....");
    else
      html->print("Processing File Upload FAILED....");
    html->println("</p>");
    sendPageEnd(html);
    return html;
  }
} uploadProcessingFilePage;

void setupServerModule() {
  SERVER->setRootPage(&indexPage);
  SERVER->setUpgradePage(&upgradeProcessingFilePage);
  SERVER->setUploadPage(&uploadProcessingFilePage);
  SERVER->setErrorPage(&errorPage);
  SERVER->setPage(&indexPage);
  SERVER->setPage(&serverPage);
  SERVER->setPage(&upgradePage);
  SERVER->setPage(&uploadPage);
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
