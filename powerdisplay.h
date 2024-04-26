#ifndef __POWER_DISPLAY
#define __POWER_DISPLAY

#include <screen.h>

class PowerDisplay : public RefreshScreen {
  void loop();
};

#endif