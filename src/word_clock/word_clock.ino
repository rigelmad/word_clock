/*
   VERSION 6
    - Fixed the birthday feature. Now on all preprogrammed birthdays, the clock will now display 
      "Happy Birthday" until the mode is manually changed.
    - Also added the option to add other special dates, but currently the only programmed holiday is Valentines Day. Open to others?
   
   VERSION 5
    - Changed DST to be controlled by latching buton
    - Changed the brightness to be triggered by a single press of the momentary switch
    - Secret word now blinks on an interval
    
   VERSION 4
    - Removed checking birthdays, waiting on a better implementation of this
    - Fixed "noon oclock" and "midnight oclock"
    - Fixed latch not being checked on stratup, resulting in a blank face

   VERSION 3

    - Noon oclock, midnight oclock
    - Heart code?

   VERSION 2
   1) Night Mode - The latch switch now controls the brightness of the lights, LOW yields a 50% brightness decrease
   2) Startup Lights - On startup, the color of the hourly text will be the appropriate color
   3) "Half way pat" -> "Half past"
   4) Changed colors around
        - Changed the AM color to white
        - Changed big digit color to orange
   5) Button debounce method has been changed
        - A short (under 1000 ms) hold is a normal press, changes mode; release is necessary for change to take place
        - A medium (between 1000 ms and 3000 ms) hold results in Secret Word Mode; no release necessary
        - A long (over 3000 ms) hold will adjust the DST time; no release neccesary
   6) New secret word is a heart, cliche, I know but I'm a hopeless romantic what can I say?
   7) For hours that aren't SEVEN or NOON, TIL has been replaced with a vertical TO
*/

/* HOW THE LIGHT PATTERN WORKS:
  > The prefix (half past, quarter to, etc) will be a separate color from the hours.
  > Hours will be one of two solid colors, one for AM, one for PM
    - Light blue for AM, Purple for PM?
  > The prefix will fade from green -> red over the course of the 5 min interval
*/

/*
    Button 1: Big Word Mode (100ms); Secret Message Mode (5s)
*/

//------Define/Includes----------

//#define DEBUG 1

#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include <Wire.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define DATA_PIN 6
#define BUTTON 4 //The Mode Button
#define LATCH_BUTTON 5

#define WORD_MODE 0
#define BIG_DIGIT_MODE 1
#define BIRTHDAY_MODE 2
#define SECRET_WORD_MODE 3
#define STARTUP 4

#define FWD 1
#define BACK 0

//------Object Declarations-------

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(11, 11, DATA_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG, NEO_KHZ800 + NEO_GRB);
RTC_DS3231 rtc;

//------Global Variables----------

DateTime theTime;

// Find the date
uint16_t theYear;
uint8_t theMonth;
uint8_t theDay;

//Find the time
uint8_t theHour;
uint8_t theMinute;
uint8_t theSecond;
uint8_t dst_add; //How much to add for DST


//For Brightness
double DAY = 1;
double NIGHT = 0.5;
double brightness_mode = DAY;

//Global var color
uint16_t color;
uint16_t AM_COLOR;
uint16_t PM_COLOR;
uint16_t BIG_DIGIT_COLOR;
uint16_t BIG_DIGIT_OUTSIDE_COLOR;
uint16_t CURSOR_COLOR;
uint16_t OBJECT_COLOR;

void setColor() { //Bad practice - but putting this function on top for easy access

  AM_COLOR = fC(150, 150, 255);
  PM_COLOR = fC(150, 0, 255);
  BIG_DIGIT_COLOR = fC(255, 100, 0);
  CURSOR_COLOR = fC(255, 0, 255);
  OBJECT_COLOR = fC(255, 0, 0);

}

//For Debounce

int current;         // Current state of the button
long millis_held;    // How long the button was held (milliseconds)
byte previous = HIGH;
unsigned long firstTime; // how long since the button was first pressed

bool beenHeld = 0;
bool beenHeldLong = 0;


//For Latch (Brightness) State
int lastLatchState; //For the changing of the DST BUTTON
unsigned long lastSecretSwitch = 0;
float HEART_BEAT_HZ = 1.5;

//For Blinking Cursor
unsigned long lastBlink = 0;
bool blinkState = 0;

//Various
const int STARTUP_DELAY = 10;
bool firstSecretTime = 1; //For the Secret Message timer
double seed; //For the color fade
uint8_t prevSecond = 0; //Tracker for the color fade, to increment each second
bool isBirthday = 0; //Flash the happy birthday when in birthday mode (will be true for entirety of brithday)
uint8_t digitPrevSecond = 0; //For the flashing of the big digits

//For Birthdays and holidays
#define NUM_BIRTHDAYS 7
uint8_t BIRTH_MONTH[NUM_BIRTHDAYS]  = { 8,  4,  1,  1,  6,  6,  9 };
uint8_t BIRTH_DAY[NUM_BIRTHDAYS]    = { 12, 22, 17, 22, 6,  8,  23 };
//For holidays, currently only Valentines Day
#define NUM_SPECIAL_DATES 1
uint8_t SPECIAL_DATE_MONTHS[NUM_SPECIAL_DATES]  = { 2 };
uint8_t SPECIAL_DATE_DAYS[NUM_SPECIAL_DATES]    = { 14 };
bool modeChangedToday = false;
uint8_t specialDateSelector = 0;

//For Master Mode
int mode;

// ----Character Number Declarations ----

const bool CHAR_NUMS[10][5][5] {
  {
    {0, 1, 1, 1, 0}, // ZERO
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 0, 0}, // ONE
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //TWO
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //THREE
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 0, 1, 0}, //FOUR
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //FIVE
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //SIX
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //SEVEN
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //EIGHT
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0}, //NINE
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}
  }
};

const bool OBJECTS[1][11][11] {
  {
    {0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
  }
};

// ---- END Number Character declarations ----

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  matrix.begin(); //across, down
  matrix.setTextWrap(true); //Used for big digit display mode

  rtc.begin(); //start the time module

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LATCH_BUTTON, INPUT_PULLUP);

  mode = STARTUP; //Initialize mode to word mode
  //  lastLatchState = !digitalRead(LATCH_BUTTON); //Should induce a checking of the latch pin on startup
  updateTime();
  seed = (theMinute % 5) * 60 + theSecond;

}

void loop() {
  matrix.clear();
  updateTime(); //Update the time and timing container (hour, min, sec) every clock cycle
  updateSeed();

  readButtonPush(); //Read the Mode button push
  dst_add = !digitalRead(LATCH_BUTTON);

  switch (mode) {
    case WORD_MODE: //Word mode
      displayWords();
      break;
    case BIG_DIGIT_MODE:
      displayBigDigits();
      break;
    case SECRET_WORD_MODE:
      displaySecretWord();
      break;
    case BIRTHDAY_MODE:
      displayBirthday();
      break;
    case STARTUP:
      runStartup(STARTUP_DELAY);
      setColor(); //Initialize all colors
      mode = WORD_MODE;
      break;
    default:
      matrix.fillScreen(matrix.Color(255, 0, 0)); //Fill screen with red for error
      break;
  }

  // Continuously check if today is a birthday, and change the mode if so
  // This needs to be after the switch so that startup runs. This should be fixed.
  checkSpecialDatesAndChangeModeOnce();

  matrix.show();

}

// ----- AUXILIARY FUNCTIONS FROM VOID LOOP() ----
void updateTime() {
  theTime = rtc.now(); //Gather current time information

  // Keep track of the previous day to know if we've changed days
  uint8_t prevDay = theDay;

  // Find the date
  theYear = theTime.year();
  theMonth = theTime.month();
  theDay = theTime.day();

  //Find the time
  theHour = (theTime.hour() + dst_add) % 24;
  theMinute = theTime.minute();
  theSecond = theTime.second();

  Serial.print(theHour);
  Serial.print(':');
  Serial.print(theMinute);
  Serial.print(':');
  Serial.print(theSecond);
  Serial.println();

  if (prevDay != theDay) { // Every new day, reset things that are supposed to reset every day
    resetDay();
  }
}

/**
 * Resets all variables that should be reset at the end of each day 
 */
void resetDay() {
  modeChangedToday = false;
  specialDateSelector = 0;
  if (mode == BIRTHDAY_MODE) {
    mode = WORD_MODE;
  }
}

void readButtonPush() {
  current = digitalRead(BUTTON);

  // if the button state changes to pressed, remember the start time
  if (current == LOW && previous == HIGH) {
    firstTime = millis();
    Serial.println("Pushed");
  }

  millis_held = (millis() - firstTime);

  if (millis_held > 50) {

    if (current == HIGH && previous == LOW) { // If the button has been released
      if (millis_held < 1000) { //Regular, one-press, less than a second
        toggleDayNight();
      }
    }

    if (millis_held >= 1000 && current == LOW && !beenHeld) { //This is outside bc I dont want to have to release the button for it to ha[en
      beenHeld = 1;
      modeChangedToday = true;

      switch (mode) {
        case WORD_MODE:
          mode = BIG_DIGIT_MODE;
          break;
        case BIG_DIGIT_MODE:
        case SECRET_WORD_MODE:
        case BIRTHDAY_MODE:
          mode = WORD_MODE;
          break;
      }

      Serial.println();
      Serial.print("------------MODE CHANGE, New Mode: ");
      Serial.print(mode);
      Serial.print("--------------");
      Serial.println();
      // Change the brightness

    }

    if (millis_held >= 3000 && current == LOW & !beenHeldLong) {
      beenHeldLong = 1;
      modeChangedToday = true;

      // If today is a birthday, go into birthday mode, else go into secret word mode
      if (isTodayABirthday()) {
        mode = BIRTHDAY_MODE;
      } else {
        mode = SECRET_WORD_MODE;
      }

      Serial.println();
      Serial.print("------------MODE CHANGE, New Mode: ");
      Serial.print(mode);
      Serial.print("--------------");
      Serial.println();
    }
  }

  if (beenHeld && current == HIGH) {
    beenHeld = 0;
  }

  if (beenHeldLong && current == HIGH) {
    beenHeldLong = 0;
  }

  previous = current;
}

void toggleDayNight() {
  if (brightness_mode == NIGHT) brightness_mode = DAY;
  else brightness_mode = NIGHT;
  setColor();
}

// ----- STATE FUNCTIONS ---- The Display Modes

void displayWords() {

  color = fadeGreenToRed(seed);

  W_ITS();

  //Break up time into 5 minute intervals

  if (theMinute < 5) {          //0-5 past hour - Its CURRENT HOUR oclock
    int hr = lightHour(0);
    color = fadeGreenToRed(seed); //For the OCLOCK
    if (hr != 0) W_OCLOCK();
  } else if (theMinute < 10) {  //5-10 past hour - Its Five past CURRENT HOUR
    W_FIVE();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 15) {   //10-15 past hour - Its ten past CURRENT HOUR
    W_TEN();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 20) {   //15-20 - Its a quarter past CURRENT HOUR
    W_A();
    W_QUARTER();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 25) {   //20-25 - Its twenty past CURRENT HOUR
    W_TWENTY();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 30) {   //25-30 - Its twenty five past CURRENT HOUR
    W_TWENTY();
    W_FIVE();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 35) {   //30-35 - Its half past CURRENT HOUR
    W_HALF();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 40) {  //35-40 - Its twenty five til NEXT HOUR
    W_TWENTY();
    W_FIVE();
    W_TO();
    lightHour(1);
  } else if (theMinute < 45) {  //40-45 - Its twenty til NEXT HOUR
    W_TWENTY();
    W_TO();
    lightHour(1);
  } else if (theMinute < 50) {  //45-50 - Its a quarter til NEXT HOUR
    W_A();
    W_QUARTER();
    W_TO();
    lightHour(1);
  } else if (theMinute < 55) {  //50-55 - Its ten til NEXT HOUR
    W_TEN();
    W_TO();
    lightHour(1);
  } else if (theMinute < 60) {  //55-60 - Its five til NEXT HOUR
    W_FIVE();
    W_TO();
    lightHour(1);
  }
}

void displayBigDigits() {
  parseTimeAndDisplayDigits();
  blinkCursor();
}

// Display the heart animation
float brightness = NIGHT;
bool secretFadeUp = true;
float SECRET_BLINK_INT = 50.0;
void displaySecretWord() {
  int i, j;

  if (millis() - lastSecretSwitch > ((int) SECRET_BLINK_INT)) {
    lastSecretSwitch = millis();

    if (secretFadeUp) {
      brightness += (DAY - NIGHT) / (1000.0 / SECRET_BLINK_INT);
    } else {
      brightness -= (DAY - NIGHT) / (1000.0 / SECRET_BLINK_INT);
    }

    if (brightness >= DAY) {
      secretFadeUp = false;
      brightness = DAY;
    } else if (brightness <= NIGHT) {
      secretFadeUp = true;
      brightness = NIGHT;
    }
  }

  for (i = 0; i < 11; i++) {
    for (j = 0; j < 11; j++) {
      if (OBJECTS[specialDateSelector][i][j]) { //Hardcoded for the first object, as there is only 1
        matrix.drawPixel(j, i, fC(255 * brightness, 0.0 , 0.0));
      }
    }
  }



}

void displayBirthday() {
  color = fC(255, 230, 0);
  W_HAPPY();
  W_BIRTHDAY();
}

void runStartup(const int speed) { //Blocking function
  //matrix.drawPixel(column, row)
  int numPixel = 0;
  for (int i = 0; i < matrix.height(); i++) { //for every row
    bool goForward = i % 2 == 0;
    if (goForward) {
      for (int j = 0; j < 14; j++) {
        matrix.clear();
        uint16_t trailColors[3] {
          Wheel(numPixel),
          Wheel(numPixel - 1),
          Wheel(numPixel - 2)
        };
        switch (j) {
          case 0:
            matrix.drawPixel(0, i, trailColors[0]);
            break;
          case 1:
            matrix.drawPixel(1, i, trailColors[0]);
            matrix.drawPixel(0, i, trailColors[1]);
            break;
          case 11:
            matrix.drawPixel(10, i, trailColors[1]);
            matrix.drawPixel(9, i, trailColors[2]);
            break;
          case 12:
            matrix.drawPixel(10, i, trailColors[2]);
            break;
          case 13:
            break;
          default:
            matrix.drawPixel(j, i, trailColors[0]);
            matrix.drawPixel(j - 1, i, trailColors[1]);
            matrix.drawPixel(j - 2, i, trailColors[2]);
            break;
        }
        matrix.show();
        numPixel++;
        delay(speed);
      }
    } else { //Going backwards
      for (int j = 13; j >= 0; j--) {
        matrix.clear();
        uint16_t trailColors[3] {
          Wheel(numPixel),
          Wheel(numPixel - 1),
          Wheel(numPixel - 2)
        };
        switch (j) {
          case 13:
            matrix.drawPixel(10, i, trailColors[0]);
            break;
          case 12:
            matrix.drawPixel(10, i, trailColors[1]);
            matrix.drawPixel(9, i, trailColors[1]);
            break;
          case 2:
            matrix.drawPixel(0, i, trailColors[1]);
            matrix.drawPixel(1, i, trailColors[2]);
            break;
          case 1:
            matrix.drawPixel(0, i, trailColors[2]);
            break;
          case 0: //FIX ME
            break;
          default:
            matrix.drawPixel(j, i, trailColors[0]);
            matrix.drawPixel(j + 1, i, trailColors[1]);
            matrix.drawPixel(j + 2, i, trailColors[2]);
            break;
        }
        matrix.show();
        numPixel++;
        delay(speed);
      }
    }
  }
}

// ----- Functions for COLOR --------

uint16_t fadeGreenToRed(double num) { //seed will be number from 0-299
  double r;
  double g;

  if (num < 150) { //Green is 255, red fades in from 0-255
    g = 255;
    r = num * 1.7; //Being that the increment is 5 minutes, and this is all 510 steps broken into 5*60 steps, changes once per second
  }

  if (num >= 150) { //Red is 255, green fades out from 255-0
    r = 255;
    g = (300 - num) * 1.7;
  }

  return fC((int) r, (int) g, 0);
}

uint16_t fC(int r, int g, int b) { //final Color - takes R,G,B values and produces a color by multiplying the parameters by the brightness_mode
  return matrix.Color((int)r * brightness_mode, (int) g * brightness_mode, (int) b * brightness_mode);
}

uint16_t fC(double r, double g, double b) { //final Color - takes R,G,B values and produces a color by multiplying the parameters by the brightness_mode
  return matrix.Color((int)r * brightness_mode, (int) g * brightness_mode, (int) b * brightness_mode);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint16_t Wheel(int WheelPos) {
  if (WheelPos < 0) return fC(0, 0, 0);
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return fC(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return fC(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return fC(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// ----- Other Auxiliary functions called elsewhere ----

// @PARAM int - set to 0 for current hour, set to 1 for next hour (30-60 min)
int lightHour (int hourFlag) { // Pushes out info for the hour, Called by displayWords()
  if (theHour >= 12) color = PM_COLOR;
  if (theHour < 12) color = AM_COLOR;
  int hourToLight = (theHour + hourFlag) % 12;
  switch (hourToLight) {
    case 0: // THIS IS 12, handle special
      //compare pre - mod value
      if ((theHour + hourFlag) == 12) {
        fixTO();
        color = PM_COLOR;
        N_NOON();
      } else if (((theHour + hourFlag) == 24) || ((theHour + hourFlag) == 0)) {
        color = AM_COLOR;
        N_MIDNIGHT();
      }
      break;
    case 1:
      N_ONE();
      break;
    case 2:
      N_TWO();
      break;
    case 3:
      N_THREE();
      break;
    case 4:
      N_FOUR();
      break;
    case 5:
      N_FIVE();
      break;
    case 6:
      N_SIX();
      break;
    case 7:
      fixTO();
      if (theHour >= 12) color = PM_COLOR;
      if (theHour < 12) color = AM_COLOR;
      N_SEVEN();
      break;
    case 8:
      N_EIGHT();
      break;
    case 9:
      N_NINE();
      break;
    case 10:
      N_TEN();
      break;
    case 11:
      N_ELEVEN();
      break;
    default:
      break;
  }
  return hourToLight;
}

//Misleading name, actually does not push info out to matrix
void light(uint8_t across, uint8_t down) { //Pushes out info to light a specific pixel- DOES NOT UPDATE SCREEN
  matrix.drawPixel(across, down, color);
}

void off() { // Pushes info to clear screen
  matrix.fillScreen(0);
}

void parseTimeAndDisplayDigits() { //Example input (15, 56), taken from global variables theHour, theMinute
  int hourTen, hourOne, minuteTen, minuteOne;

  hourTen = theHour / 10;
  minuteTen = theMinute / 10;

  hourOne = theHour % 10;
  minuteOne = theMinute % 10;

  lightTL(hourTen);
  lightTR(hourOne);
  lightBL(minuteTen);
  lightBR(minuteOne);

}

void fixTO() {
  if (theMinute >= 35 && theMinute < 60) {
    color = matrix.Color(0, 0, 0); //Turn off W_TO
    W_TO();
    color = fadeGreenToRed(seed); //Light up W_TIL
    W_TIL();
  }
}

void lightTL(int num) { // Accepts input 1-9 (actually should really only be a zero or one or two), displays number in the top left quadrant
  int i, j;

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (CHAR_NUMS[num][i][j]) {
        matrix.drawPixel(j, i, BIG_DIGIT_COLOR);
      }
    }
  }

}

void lightTR(int num) { // Accepts input 1-9  displays number in the top right quadrant
  int i, j;
  int offsetHorizontal = 6;

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (CHAR_NUMS[num][i][j]) {
        matrix.drawPixel(j + offsetHorizontal, i, BIG_DIGIT_COLOR);
      }
    }
  }

}

void lightBL(int num) { // Accepts input 1-9 displays number in the bottom left quadrant
  int i, j;
  int offsetVertical = 6;

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (CHAR_NUMS[num][i][j]) {
        matrix.drawPixel(j, i + offsetVertical, BIG_DIGIT_COLOR);
      }
    }
  }

}

void lightBR(int num) { // Accepts input 1-9 displays number in the bottom right quadrant
  int i, j;
  int offsetVerticalAndHorizontal = 6;

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (CHAR_NUMS[num][i][j]) {
        matrix.drawPixel(j + offsetVerticalAndHorizontal, i + offsetVerticalAndHorizontal, BIG_DIGIT_COLOR);
      }
    }
  }

}

void blinkCursor() {

  if ((millis() - lastBlink) >= 1000) {
    lastBlink = millis();
    if (blinkState == 0) {
      blinkState = 1;
    } else blinkState = 0;
  }

  if (blinkState) {
    color = CURSOR_COLOR;
  } else color = matrix.Color(0, 0, 0);

  C_CURSOR();

}

bool hasSecondChanged() {
  if (theSecond != prevSecond) {
    prevSecond = theSecond;
    return 1;
  } else return 0;
}


void updateSeed() {
  if (hasSecondChanged()) {
    if (theMinute % 5 == 0 && theSecond == 0) { //If we land on a fifth minute, reset the seed
      seed = 0;
    } else seed++; //else add one to the seed
  }
}

// Returns true if today's date is in the list of birth month/days
bool isTodayABirthday() {
  for (int i = 0; i < NUM_BIRTHDAYS; i++) {
    if (BIRTH_MONTH[i] == theMonth) {
      if (BIRTH_DAY[i] == theDay) {
        return true;
      }
    }
  }
  return false;
}

// Returns the index of the special date that today is, if it is one. If not, returns -1
int isTodayASpecialDate() {
  for (int i = 0; i < NUM_SPECIAL_DATES; i++) {
    if (SPECIAL_DATE_MONTHS[i] == theMonth) {
      if (SPECIAL_DATE_DAYS[i] == theDay) {
        return i;
      }
    }
  }
  return -1;
}

void checkSpecialDatesAndChangeModeOnce() {
  if (!modeChangedToday) {
    if (isTodayABirthday()) {
      mode = BIRTHDAY_MODE;
    } else if (isTodayASpecialDate() != -1) {
      mode = SECRET_WORD_MODE;
      specialDateSelector = isTodayASpecialDate();
    }
  }
}
// ----- WORD FUNCTIONS -- Pushes out info for the words/phrases ------

void W_ITS() { //Word_ITS
  for (int i = 0; i <= 2; i++) {
    light(i, 0);
  }
}

void W_A() {
  light(4, 0);
}

void W_HALF() {
  for (int i = 6; i <= 9; i++) {
    light(i, 0);
  }
}

void W_TEN() {
  for (int i = 1; i <= 3; i++) {
    light(i, 1);
  }
}

void W_QUARTER() {
  for (int i = 4; i <= 10; i++) {
    light(i, 1);
  }
}

void W_TWENTY() {
  for (int i = 0; i <= 5; i++) {
    light(i, 2);
  }
}

void W_FIVE() {
  for (int i = 7; i <= 10; i++) {
    light(i, 2);
  }
}

void W_WAY() {
  for (int i = 0; i <= 2; i++) {
    light(i, 3);
  }
}

void W_TIL() {
  for (int i = 3; i <= 5; i++) {
    light(i, 3);
  }
}

void W_TO() { // Added because TIL was driving me fucking mad
  light(9, 3);
  light(9, 4);
}

void W_PAST() {
  for (int i = 6; i <= 9; i++) {
    light(i, 3);
  }
}

void N_SEVEN() {
  for (int i = 1; i <= 5; i++) {
    light(i, 4);
  }
}

void N_NOON() {
  for (int i = 7; i <= 10; i++) {
    light(i, 4);
  }
}

void W_BIRTHDAY() {
  for (int i = 1; i <= 5; i++) {
    light(i, 5);
  }

  for (int j = 7; j <= 9; j++) {
    light(j, 5);
  }
}

void N_MIDNIGHT() {
  for (int i = 0; i <= 7; i++) {
    light(i, 6);
  }
}

void N_TEN() {
  for (int i = 8; i <= 10; i++) {
    light(i, 6);
  }
}

void N_FIVE() {
  for (int i = 0; i <= 3; i++) {
    light(i, 7);
  }
}

void N_NINE() {
  for (int i = 4; i <= 7; i++) {
    light(i, 7);
  }
}

void N_TWO() {
  for (int i = 8; i <= 10; i++) {
    light(i, 7);
  }
}

void N_ELEVEN() {
  for (int i = 0; i <= 5; i++) {
    light(i, 8);
  }
}

void N_EIGHT() {
  for (int i = 6; i <= 10; i++) {
    light(i, 8);
  }
}

void N_ONE() {
  for (int i = 0; i <= 2; i++) {
    light(i, 9);
  }
}

void N_SIX() {
  for (int i = 3; i <= 5; i++) {
    light(i, 9);
  }
}

void N_THREE() {
  for (int i = 6; i <= 10; i++) {
    light(i, 9);
  }
}

void N_FOUR() {
  for (int i = 0; i <= 3; i++) {
    light(i, 10);
  }
}

void W_OCLOCK() {
  for (int i = 5; i <= 10; i++) {
    light(i, 10);
  }
}

void W_HAPPY() {
  for (int i = 0; i <= 4; i++) {
    light(6, i);
  }
}

void W_I() {
  light(3, 0);
}

void W_LOVE() {
  light(8, 0);
  light(10, 0);
  light(9, 2);
  light(10, 2);
}

void W_YOU() {
  light(0, 4);
  light(0, 5);
  light(4, 10);
}

void C_CURSOR() {
  light(5, 4);
  light(5, 6);
  light(4, 5);
  light(5, 5);
  light(6, 5);
}
