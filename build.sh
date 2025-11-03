#!/bin/bash

# Source helper scripts
if ! source common.sh 2> /dev/null; then
  echo "Error: common.sh not found. Please ensure it's in the same directory." >&2
  exit 1
fi

BUILD="$1"
CURRENT_DIR="$2"

case "$BUILD" in
  --clean)
    Delete "$CURRENT_DIR"/favicon.h
    create_libraries_used.sh --clean "$LIBRARY_GAVEL/$LIBRARYV1"/GavelGPIO
    create_libraries_used.sh --clean "$LIBRARY_GAVEL/$LIBRARYV1"/GavelLicense
    ;;
  --pre)
    createfileheader.sh "$CURRENT_DIR"/assets/favicon_blue.ico "$CURRENT_DIR"/favicon.h favicon
    create_libraries_used.sh --build "$LIBRARY_GAVEL/$LIBRARYV1"/GavelGPIO TCA9555_GPIO
    create_libraries_used.sh --build "$LIBRARY_GAVEL/$LIBRARYV1"/GavelLicense
    #create_libraries_used.sh --build "$LIBRARY_GAVEL/$LIBRARYV1"/GavelLicense ETHERNET_USED TERMINAL_USED TCA9555_USED I2C_EEPROM_USED DHT_SENSOR_LIBRARY_USED ADAFRUIT_SSD1306_USED ADAFRUIT_GFX_LIBRARY_USED ADAFRUIT_BUSIO_USED ADAFRUIT_UNIFIED_SENSOR_USED
    ;;
  --post)
    create_libraries_used.sh --clean "$LIBRARY_GAVEL/$LIBRARYV1"/GavelGPIO
    create_libraries_used.sh --clean "$LIBRARY_GAVEL/$LIBRARYV1"/GavelLicense
    ;;
  --build)
    createfileheader.sh "$CURRENT_DIR"/assets/favicon_blue.ico "$CURRENT_DIR"/favicon.h favicon
    ;;
  *)
    log_failed "Invalid Command Argument: $BUILD"
    exit 1
    ;;
esac

exit 0
