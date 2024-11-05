#pragma once

#include "wled.h"

//
// Inspired by the v1 usermods
// * rotary_encoder_change_brightness
// * rotary_encoder_change_effect
//
// v2 usermod that provides a rotary encoder-based UI.
//
// This usermod allows you to control:
// 
// * Brightness
// 
// Change between modes by pressing a button.
//
// Dependencies
// * This usermod REQUIRES the ModeSortUsermod
// * This Usermod works best coupled with 
//   FourLineDisplayUsermod.
//

#ifndef ENCODER_DT_PIN
#define ENCODER_DT_PIN 4
#endif

#ifndef ENCODER_CLK_PIN
#define ENCODER_CLK_PIN 5
#endif


class RotaryEncoderBRIUsermod : public Usermod {
private:
  int fadeAmount = 10;             // Amount to change every step (brightness)
  unsigned long currentTime;
  unsigned long loopTime;
  int8_t pinA = ENCODER_DT_PIN;       // DT from encoder
  int8_t pinB = ENCODER_CLK_PIN;      // CLK from encoder
  

  unsigned char Enc_A;
  unsigned char Enc_B;
  unsigned char Enc_A_prev = 0;


  bool initDone = false;
  bool enabled = true;

  // strings to reduce flash memory usage (used more than twice)
  static const char _name[];
  static const char _enabled[];
  static const char _DT_pin[];
  static const char _CLK_pin[];

public:
  /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
  void setup()
  {
    DEBUG_PRINTLN(F("Usermod Rotary Encoder init."));
    PinManagerPinType pins[2] = { { pinA, false }, { pinB, false } };
    if (!pinManager.allocateMultiplePins(pins, 2, PinOwner::UM_RotaryEncoderBRI)) {
      // BUG: configuring this usermod with conflicting pins
      //      will cause it to de-allocate pins it does not own
      //      (at second config)
      //      This is the exact type of bug solved by pinManager
      //      tracking the owner tags....
      pinA = pinB= -1;
      enabled = false;
      return;
    }

    #ifndef USERMOD_ROTARY_ENCODER_GPIO
      #define USERMOD_ROTARY_ENCODER_GPIO INPUT_PULLUP
    #endif
    pinMode(pinA, USERMOD_ROTARY_ENCODER_GPIO);
    pinMode(pinB, USERMOD_ROTARY_ENCODER_GPIO);

    currentTime = millis();
    loopTime = currentTime;



    initDone = true;
  }

  /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
  void connected()
  {
    //Serial.println("Connected to WiFi!");
  }

  /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
  void loop()
  {
    if (!enabled) return;

    currentTime = millis(); // get the current elapsed time

    // Initialize effectCurrentIndex and effectPaletteIndex to
    // current state. We do it here as (at least) effectCurrent
    // is not yet initialized when setup is called.

    if (currentTime >= (loopTime + 2)) // 2ms since last check of encoder = 500Hz
    {

      int Enc_A = digitalRead(pinA); // Read encoder pins
      int Enc_B = digitalRead(pinB);
      if ((!Enc_A) && (Enc_A_prev))
      { // A has gone from high to low
        if (Enc_B == HIGH)
        { // B is high so clockwise
              changeBrightness(true);
        }
        else if (Enc_B == LOW)
        { // B is low so counter-clockwise

              changeBrightness(false);
    
        }
      }
      Enc_A_prev = Enc_A;     // Store value of A for next time
      loopTime = currentTime; // Updates loopTime
    }
  }



  void lampUdated() {
    colorUpdated(CALL_MODE_BUTTON);
    updateInterfaces(CALL_MODE_BUTTON);
  }

  void changeBrightness(bool increase) {
#ifdef USERMOD_FOUR_LINE_DISPLAY
    if (display && display->wakeDisplay()) {
      // Throw away wake up input
      return;
    }
#endif
    if (increase) {
      bri = (bri + fadeAmount <= 255) ? (bri + fadeAmount) : 255;
    }
    else {
      bri = (bri - fadeAmount >= 0) ? (bri - fadeAmount) : 0;
    }
    lampUdated();
  }





  /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
  /*
    void addToJsonInfo(JsonObject& root)
    {
      int reading = 20;
      //this code adds "u":{"Light":[20," lux"]} to the info object
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
      JsonArray lightArr = user.createNestedArray("Light"); //name
      lightArr.add(reading); //value
      lightArr.add(" lux"); //unit
    }
    */

  /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
  void addToJsonState(JsonObject &root)
  {
    //root["user0"] = userVar0;
  }

  /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
  void readFromJsonState(JsonObject &root)
  {
    //userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
    //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
  }

  /**
   * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
   */
  void addToConfig(JsonObject &root) {
    // we add JSON object: {"Rotary-Encoder":{"DT-pin":12,"CLK-pin":14,"SW-pin":13}}
    JsonObject top = root.createNestedObject(FPSTR(_name)); // usermodname
    top[FPSTR(_enabled)] = enabled;
    top[FPSTR(_DT_pin)]  = pinA;
    top[FPSTR(_CLK_pin)] = pinB;
    DEBUG_PRINTLN(F("Rotary Encoder config saved."));
  }

  /**
   * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
   *
   * The function should return true if configuration was successfully loaded or false if there was no configuration.
   */
  bool readFromConfig(JsonObject &root) {
    // we look for JSON object: {"Rotary-Encoder":{"DT-pin":12,"CLK-pin":14,"SW-pin":13}}
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) {
      DEBUG_PRINT(FPSTR(_name));
      DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
      return false;
    }
    int8_t newDTpin  = top[FPSTR(_DT_pin)]  | pinA;
    int8_t newCLKpin = top[FPSTR(_CLK_pin)] | pinB;

    enabled   = top[FPSTR(_enabled)] | enabled;

    DEBUG_PRINT(FPSTR(_name));
    if (!initDone) {
      // first run: reading from cfg.json
      pinA = newDTpin;
      pinB = newCLKpin;
      DEBUG_PRINTLN(F(" config loaded."));
    } else {
      DEBUG_PRINTLN(F(" config (re)loaded."));
      // changing parameters from settings page
      if (pinA!=newDTpin || pinB!=newCLKpin) {
        pinManager.deallocatePin(pinA, PinOwner::UM_RotaryEncoderBRI);
        pinManager.deallocatePin(pinB, PinOwner::UM_RotaryEncoderBRI);
        pinA = newDTpin;
        pinB = newCLKpin;
        if (pinA<0 || pinB<0) {
          enabled = false;
          return true;
        }
        setup();
      }
    }
    // use "return !top["newestParameter"].isNull();" when updating Usermod with new features
    return !top[FPSTR(_enabled)].isNull();
  }

  /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
  uint16_t getId()
  {
    return USERMOD_ID_ROTARY_ENC_BRI;
  }
};

// strings to reduce flash memory usage (used more than twice)
const char RotaryEncoderBRIUsermod::_name[]     PROGMEM = "Rotary-Encoder";
const char RotaryEncoderBRIUsermod::_enabled[]  PROGMEM = "enabled";
const char RotaryEncoderBRIUsermod::_DT_pin[]   PROGMEM = "DT-pin";
const char RotaryEncoderBRIUsermod::_CLK_pin[]  PROGMEM = "CLK-pin";
