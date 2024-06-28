#include "powerdisplay.h"

#include "powermemory.h"

#include <ethernetmodule.h>
#include <temperature.h>

#define SCREEN_TAB "   "

void PowerDisplay::screen() {
  int lineIndex = 0;
  String displayLines[8];
  String buildString;
  refresh = 1000;
  displayLines[lineIndex++] = ProgramInfo::AppName;
  buildString = "Ver. " + String(ProgramInfo::MajorVersion) + String(".") + String(ProgramInfo::MinorVersion);
  displayLines[lineIndex++] = buildString;

  if (TEMPERATURE->validTemperature()) {
    buildString = "Temperature: ";
    buildString += (String(TEMPERATURE->getTemperature()));
    buildString += " F";
    displayLines[lineIndex++] = buildString;
  } else {
    displayLines[lineIndex++] = "";
  }
  if (ETHERNET->linkStatus()) {
    buildString = String(POWER_MEMORY.ipAddress[0]);
    buildString += ".";
    buildString += String(POWER_MEMORY.ipAddress[1]);
    buildString += ".";
    buildString += String(POWER_MEMORY.ipAddress[2]);
    buildString += ".";
    buildString += String(POWER_MEMORY.ipAddress[3]);
    displayLines[lineIndex++] = buildString;
  } else {
    displayLines[lineIndex++] = "";
  }
  displayLines[lineIndex++] = "";
  buildString = "";
  for (byte i = 0; i < NUM_DEVICES; i++) {
    buildString += String(i + 1);
    buildString += SCREEN_TAB;
  }
  displayLines[lineIndex++] = buildString;
  buildString = "";
  for (byte i = 0; i < NUM_DEVICES; i++) {
    if (POWER_DATA->getCurrentRelayStatus(i) == false)
      buildString += " ";
    else
      buildString += "*";
    buildString += SCREEN_TAB;
  }
  displayLines[lineIndex++] = buildString;
  SCREEN->setScreen(displayLines[0], displayLines[1], displayLines[2], displayLines[3], displayLines[4], displayLines[5], displayLines[6], displayLines[7]);
}