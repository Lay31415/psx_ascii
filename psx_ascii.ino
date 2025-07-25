#include <arduino_psx.h>
#include "HID-Project.h"
#include <EEPROM.h>

#define ACK 9
#define TT_R 0
#define TT_L 1
#define TT_DELAY 136
// #define POPN

const bool TT_R_ACTIVE = false;
const bool RISE_EDGE_ACTIVE = false;

enum KeyIndex {
  KEY1,        // KEY1 = 0
  KEY2,        // KEY2 = 1
  KEY3,        // KEY3 = 2
  KEY4,        // KEY4 = 3
  KEY5,        // KEY5 = 4
  KEY6,        // KEY6 = 5
  KEY7,        // KEY7 = 6
  KEY8,        // KEY8 = 7
  KEY9,        // KEY9 = 8
  KEY_SL,      // SELECT = 9
  KEY_ST,      // START  = 10
  KEY_LEN      // 配列サイズ
};
const byte key[] = {
  2,    // KEY1
  3,    // KEY2
  4,    // KEY3
  5,    // KEY4
  6,    // KEY5
  7,    // KEY6
  8,    // KEY7
  A0,   // KEY8
  A1,   // KEY9
  A2,   // SELECT
  A3    // START
  };

#define CONFIG_ADDR 0
enum Config {DIGITAL, ANALOG, CONFIG_LEN};
byte config;

// TT
volatile int8_t TT_count = 0;
volatile char TT_ACTIVE = 0;
volatile long TT_time;

void TTproc(char count){
  TT_ACTIVE = count;
  TT_time = millis();

  if (config == ANALOG) {
    switch (count){
      case 1:
        TT_count++;
        break;
      case -1:
        TT_count--;
        break;
    }
    Serial.print("TT: "); Serial.println(TT_count);
  }
}

void setButton(uint8_t key, bool state) {
  if (state) {
    Gamepad.press(key);
  } else {
    Gamepad.release(key);
  }
}

void TT_L_func(){
  if (RISE_EDGE_ACTIVE || !digitalRead(TT_L)) {
    TTproc(digitalRead(TT_R) ? 1 : -1);
  }
}
void TT_R_func(){
  if (RISE_EDGE_ACTIVE || !digitalRead(TT_R)) {
    TTproc(digitalRead(TT_L) ? 1 : -1);
  }
}

void setup() {
    Serial.begin(9600);
    delay(3000);
    Serial.println("Serial Start");
    // Setup PSX
    PSX.init(ACK, false, true);
    // Setup Gamepad
    Gamepad.begin();
    
    // Setup buttons
    pinMode(TT_L, INPUT_PULLUP);
    pinMode(TT_R, INPUT_PULLUP);
    for(int i= 0 ; i < KEY_LEN; i++){
      pinMode(key[i], INPUT_PULLUP);
    }
    
    #ifdef POPN
      Serial.println("mode: Pop'n");
      PSX.setAlwaysHeld(PS_INPUT::PS_LEFT | PS_INPUT::PS_DOWN | PS_INPUT::PS_RIGHT);
    #else
      Serial.println("mode: beatmania");
      // Set/Read Config
      if (!digitalRead(key[KEY_SL])) {
        config = DIGITAL;
        EEPROM.update(CONFIG_ADDR, config);
        Serial.println("Config Set DIGITAL");
      } else if (!digitalRead(key[KEY_ST])) {
        config = ANALOG;
        EEPROM.update(CONFIG_ADDR, config);
        Serial.println("Config Set ANALOG");
      } else{
        config = EEPROM.read(CONFIG_ADDR);
        Serial.print("Config read "); Serial.println(config);
        config = config >= CONFIG_LEN ? DIGITAL : config;
      }

      attachInterrupt(digitalPinToInterrupt(TT_L), TT_L_func, CHANGE);
      if (TT_R_ACTIVE) {
        attachInterrupt(digitalPinToInterrupt(TT_R), TT_R_func, CHANGE);
      }
    #endif
}

void loop() {
    // Set PSX buttons
    PSX.setButton(PS_INPUT::PS_SELECT, !digitalRead(key[KEY_SL]));
    PSX.setButton(PS_INPUT::PS_START , !digitalRead(key[KEY_ST]));
    #ifdef POPN
      PSX.setButton(PS_INPUT::PS_TRIANGLE, !digitalRead(key[KEY1]));
      PSX.setButton(PS_INPUT::PS_CIRCLE  , !digitalRead(key[KEY2]));
      PSX.setButton(PS_INPUT::PS_R1      , !digitalRead(key[KEY3]));
      PSX.setButton(PS_INPUT::PS_CROSS   , !digitalRead(key[KEY4]));
      PSX.setButton(PS_INPUT::PS_L1      , !digitalRead(key[KEY5]));
      PSX.setButton(PS_INPUT::PS_SQUARE  , !digitalRead(key[KEY6]));
      PSX.setButton(PS_INPUT::PS_R2      , !digitalRead(key[KEY7]));
      PSX.setButton(PS_INPUT::PS_UP      , !digitalRead(key[KEY8]));
      PSX.setButton(PS_INPUT::PS_L2      , !digitalRead(key[KEY9]));
    #else
      PSX.setButton(PS_INPUT::PS_SQUARE  , !digitalRead(key[KEY1]));
      PSX.setButton(PS_INPUT::PS_L1      , !digitalRead(key[KEY2]));
      PSX.setButton(PS_INPUT::PS_CROSS   , !digitalRead(key[KEY3]));
      PSX.setButton(PS_INPUT::PS_R1      , !digitalRead(key[KEY4]));
      PSX.setButton(PS_INPUT::PS_CIRCLE  , !digitalRead(key[KEY5]));
      PSX.setButton(PS_INPUT::PS_TRIANGLE, !digitalRead(key[KEY6]));
      PSX.setButton(PS_INPUT::PS_LEFT    , !digitalRead(key[KEY7]));
    #endif
    
    // Set USB buttons
    for(int i=0; i < KEY_LEN; i++){
      if(key[i]){ setButton(i+1, !digitalRead(key[i])); }
    }

    // Set TT
    if (TT_ACTIVE != 0 && millis() - TT_time > TT_DELAY){
      PSX.setButton(PS_INPUT::PS_UP  , false);
      PSX.setButton(PS_INPUT::PS_DOWN, false);
      if (config == DIGITAL) Gamepad.dPad2(GAMEPAD_DPAD_CENTERED);
      TT_ACTIVE = 0;
    } else if (TT_ACTIVE == 1) {
      PSX.setButton(PS_INPUT::PS_UP  , true);
      PSX.setButton(PS_INPUT::PS_DOWN, false);
      if (config == DIGITAL) Gamepad.dPad2(GAMEPAD_DPAD_UP);
    } else if (TT_ACTIVE == -1) {
      PSX.setButton(PS_INPUT::PS_UP  , false);
      PSX.setButton(PS_INPUT::PS_DOWN, true);
      if (config == DIGITAL) Gamepad.dPad2(GAMEPAD_DPAD_DOWN);
    }
    if (config == ANALOG) {
      Gamepad.xAxis( TT_count * 256 );
    }

    // Send
    PSX.send();
    Gamepad.write();
}
