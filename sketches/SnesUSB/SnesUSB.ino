/*******************************************************************************
 * Snes controller input library.
 * https://github.com/sonik-br/SnesLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * This sketch is ready to use on a Leonardo/Pro Micro but may work on any
 * arduino with the correct number of pins and proper setup.
 * 
 * Works with common snes pad and multitap.
 * Not tested with NES pad and NTT pad but should work.
 * DO NOT connect lightguns and other unsuported controllers
 * 
 * By using the multitap it's possible to connect up to 4 controllers.
 * multitap is detected only when powering the arduino.
 * 
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
*/

/*******************************************************************************
 * Multitap or Multiport config
 * Comment both to use single port without multitap support.
 * Define SNES_ENABLE_MULTITAP to use single port with multitap support.
 * Define SNES_MULTI_CONNECTION to use multiple ports without multitap support.
 * Don't define both. Multitap and MultiConnection will not work at the same time.
 * SNES_MULTI_CONNECTION can be set to 2, 3, or 4.
*/

//#define SNES_ENABLE_MULTITAP
//#define SNES_MULTI_CONNECTION 2

#include <SnesLib.h>
#include <Joystick.h>

//#define ENABLE_SERIAL_DEBUG

//Snes pins
#define SNESPIN_CLOCK  A3
#define SNESPIN_LATCH  A2
#define SNESPIN_DATA1  A1

#ifdef SNES_ENABLE_MULTITAP
  #define SNESPIN_DATA2  A0 //DATA for the second controller
  #define SNESPIN_SELECT 14
#else //DATA pins for additional controllers
  #define SNESPIN_DATA2  2
  #define SNESPIN_DATA3  3
  #define SNESPIN_DATA4  4
#endif


#ifdef SNES_ENABLE_MULTITAP //single port with multitap support
  SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2, SNESPIN_SELECT> snes;
#else
  #ifdef SNES_MULTI_CONNECTION //multiple port without multitap support
    #if SNES_MULTI_CONNECTION == 2
      SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2> snes;
    #elif SNES_MULTI_CONNECTION == 3
      SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2, SNESPIN_DATA3> snes;
    #elif SNES_MULTI_CONNECTION == 4
      SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2, SNESPIN_DATA3, SNESPIN_DATA4> snes;
    #endif
  #else //single port without multitap support
    SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1> snes;
  #endif
#endif

//Limit to 4 controllers. Can also used SNES_MAX_CTRL that's auto defined in the lib.
//#define MAX_USB_STICKS 4

#define USB_BUTTON_COUNT 10

#ifdef ENABLE_SERIAL_DEBUG
#define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
#define debug(...) Serial.print (__VA_ARGS__)
#define debugln(...) Serial.println (__VA_ARGS__)
#else
#define dstart(...)
#define debug(...)
#define debugln(...)
#endif

Joystick_* usbStick[SNES_MAX_CTRL];

uint8_t totalUsb = 1;
uint16_t sleepTime = 500; //In micro seconds

//dpad to hat table angles. RLDU
const int16_t hatTable[] = {
  0,0,0,0,0, //not used
  135, //0101
  45,  //0110
  90,  //0111
  0, //not used
  225, //1010
  315, //1010
  270, //1011
  0, //not used
  180, //1101
  0,   //1110
  JOYSTICK_HATSWITCH_RELEASE, //1111
};

void blinkLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

void resetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;

  for (uint8_t x = 0; x < USB_BUTTON_COUNT; x++)
    usbStick[i]->releaseButton(x);
}

void setup() {
  //Init onboard led pin
  pinMode(LED_BUILTIN, OUTPUT);

  //Init the class
  snes.begin();

  delayMicroseconds(10);

  //Multitap is connected?
  const uint8_t tap = snes.getMultitapPorts();
  if (tap == 0){ //No multitap connected during boot
    totalUsb = SNES_MAX_CTRL;
  }
  else { //Multitap connected
    totalUsb = min(tap, SNES_MAX_CTRL);
    sleepTime = 1000; //use longer interval between reads for multitap
  }

  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joystick_ (
      JOYSTICK_DEFAULT_REPORT_ID + i,
      JOYSTICK_TYPE_GAMEPAD,
      USB_BUTTON_COUNT,
      1,      // hatSwitchCount (0-2)
      false,   // includeXAxis
      false,   // includeYAxis
      false,  // includeZAxis
      false,  // includeRxAxis
      false,  // includeRyAxis
      false,  // includeRzAxis
      false,  // includeRudder
      false,   // includeThrottle
      false,  // includeAccelerator
      false,   // includeBrake
      false   // includeSteering
    );
  }

  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i]->begin(false); //Disable automatic sendState
    resetJoyValues(i);
  }
  
  delay(50);

  dstart (115200);
  //debugln (F("Powered on!"));
}


void loop() {
  static uint8_t lastControllerCount = 0;
  unsigned long start = micros();

  //Read snes port
  //It's not required to disable interrupts but it will gain some performance
  noInterrupts();
  snes.update();
  interrupts();

  //Get the number of connected controllers
  const uint8_t joyCount = snes.getControllerCount();

  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;

    //Get the data for the specific controller
    const SnesController& sc = snes.getSnesController(i);

    //Only process data if state changed from previous read
    if(sc.stateChanged()) {
      //Controller just connected.
      if (sc.deviceJustChanged())
        resetJoyValues(i);

      uint8_t hatData = sc.hat();

      if (sc.deviceType() == SNES_DEVICE_NES) {
        usbStick[i]->setButton(1, sc.digitalPressed(SNES_Y));
        usbStick[i]->setButton(2, sc.digitalPressed(SNES_B));
      } else {
        usbStick[i]->setButton(0, sc.digitalPressed(SNES_Y));
        usbStick[i]->setButton(1, sc.digitalPressed(SNES_B));
        usbStick[i]->setButton(2, sc.digitalPressed(SNES_A));
        usbStick[i]->setButton(3, sc.digitalPressed(SNES_X));
        usbStick[i]->setButton(4, sc.digitalPressed(SNES_L));
        usbStick[i]->setButton(5, sc.digitalPressed(SNES_R));        
      }
      
      usbStick[i]->setButton(8, sc.digitalPressed(SNES_SELECT));
      usbStick[i]->setButton(9, sc.digitalPressed(SNES_START));

      //Get angle from hatTable and pass to joystick class
      usbStick[i]->setHatSwitch(0, hatTable[hatData]);

      usbStick[i]->sendState();
    }
  }
    

  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      resetJoyValues(i);
    }
  }

  //Keep count for next read
  lastControllerCount = joyCount;


  //Sleep if total loop time was less than sleepTime
  unsigned long delta = micros() - start;
  if (delta < sleepTime) {
    debug(delta);
    debug(F("\t"));
    delta = sleepTime - delta;
    debugln(delta);
    delayMicroseconds(delta);
  }

  //Blink led while no controller connected
  if (joyCount == 0)
    blinkLed();
}
