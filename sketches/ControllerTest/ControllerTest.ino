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
*/

#include <SnesLib.h>

#define ENABLE_SERIAL_DEBUG

#define SNESPIN_CLOCK  A3
#define SNESPIN_LATCH  A2
#define SNESPIN_DATA1  A1
#define SNESPIN_DATA2  A0
#define SNESPIN_SELECT 14

SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1, SNESPIN_DATA2, SNESPIN_SELECT> snes;
//to use less pins, also comment SNES_ENABLE_MULTITAP at SnesLib.h
//SnesPort<SNESPIN_CLOCK, SNESPIN_LATCH, SNESPIN_DATA1> snes;

#define ENABLE_SERIAL_DEBUG

#ifdef ENABLE_SERIAL_DEBUG
#define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
#define debug(...) Serial.print (__VA_ARGS__)
#define debugln(...) Serial.println (__VA_ARGS__)
#else
#define dstart(...)
#define debug(...)
#define debugln(...)
#endif

#define DIGITALSTATE(D) \
if(sc.digitalJustPressed(D)) { \
  debugln(F("Digital pressed: " #D)); \
} else if(sc.digitalJustReleased(D)) {\
  debugln(F("Digital released: " #D)); \
}

#define DEVICE(A, B) \
if(A == B) {\
  debug(#B); \
}

void printDeviceType (const SnesDeviceType_Enum d){
  DEVICE(d, SNES_DEVICE_NONE)
  DEVICE(d, SNES_DEVICE_NOTSUPPORTED)
  DEVICE(d, SNES_DEVICE_NES)
  DEVICE(d, SNES_DEVICE_PAD)
  DEVICE(d, SNES_DEVICE_NTT)
}

void printButtons(const SnesController& sc) {
  DIGITALSTATE(SNES_B)
  DIGITALSTATE(SNES_Y)
  DIGITALSTATE(SNES_SELECT)
  DIGITALSTATE(SNES_START)
  DIGITALSTATE(SNES_UP)
  DIGITALSTATE(SNES_DOWN)
  DIGITALSTATE(SNES_LEFT)
  DIGITALSTATE(SNES_RIGHT)
  DIGITALSTATE(SNES_A)
  DIGITALSTATE(SNES_X)
  DIGITALSTATE(SNES_L)
  DIGITALSTATE(SNES_R)
}

void setup() {
  //Init the library.
  snes.begin();
  
  delay(50);

  dstart (115200);
  debugln (F("Powered on!"));
}

void loop() {
  static unsigned long idleTimer = 0;
  static uint8_t lastControllerCount = 0;
  static uint8_t lastMultitapPorts = 0;
  static SnesDeviceType_Enum dtype[4] = { SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE };
  
  //It's not required to disable interrupts but it will gain some performance
  noInterrupts();
  const unsigned long start = micros();
  
  //Call update to read the controller(s)
  snes.update();

  //Time spent to read controller(s) in microseconds
  const unsigned long delta = micros() - start;
  interrupts();

  const uint8_t multitapPorts = snes.getMultitapPorts();
  //A multitap was connected or disconnected?
  if (lastMultitapPorts > multitapPorts) {
    debugln(F("Multitap disconnected"));
  } else if (lastMultitapPorts < multitapPorts) {
    debug(F("Multitap connected. Ports: "));
    debugln(snes.getMultitapPorts());
  }

  const uint8_t joyCount = snes.getControllerCount();
  //debugln(joyCount);
  if (lastControllerCount != joyCount) {
    debug(F("Connected devices: "));
    debugln(joyCount);
  }

  bool isIdle = true;
  for (uint8_t i = 0; i < joyCount; i++) {
    const SnesController& sc = snes.getSnesController(i);
    if (sc.stateChanged()) {
      isIdle = false;
      //hatData = sc.hat();
      //dtype = sc.deviceType();

      //Controller just connected.
      if (sc.deviceJustChanged()) {
        debug(F("Device changed from "));
        printDeviceType(dtype[i]);
        debug(F(" to "));
        dtype[i] = sc.deviceType();
        printDeviceType(dtype[i]);
        debugln(F(""));

        if (dtype[i] == SNES_DEVICE_NTT) {
          debugln(F("NTT DATA"));
          debugln(sc.extendedRaw(), BIN);
        }
        
      }

      //bool isPressed = sc.digitalPressed(SNES_B);
      
      printButtons(sc);
    }
    
  }

  //Controller has been disconnected?
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      const SnesController& sc = snes.getSnesController(i);
      if (sc.stateChanged() && sc.deviceJustChanged()) {
        debugln(F("Device disconnected"));
        dtype[i] = SNES_DEVICE_NONE;
      }
    }
  }
  
  lastControllerCount = joyCount;
  lastMultitapPorts = multitapPorts;

  if(isIdle) {
    if (millis() - idleTimer >= 3000) {
      idleTimer = millis();
      debug(F("Idle. Read time: "));
      debugln(delta);
    }
  } else {
    idleTimer = millis();
    debug(F("Read time: "));
    debugln(delta);
  }

  delay(5);
  
}
