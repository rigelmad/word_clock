/*
 * ITEMS CHANGED IN THIS VERSION
 * 1) Night Mode - The latch switch now controls the brightness of the lights, LOW yields a 50% brightness decrease
 * 2) Startup Lights - On startup, the color of the hourly text will be the appropriate color
 * 3) "Half way pat" -> "Half past"
 * 4) Changed colors around
 *      - Changed the AM color to white
 *      - Changed big digit color to orange
 * 5) Button debounce method has been changed
 *      - A short (under 1000 ms) hold is a normal press, changes mode; release is necessary for change to take place
 *      - A medium (between 1000 ms and 3000 ms) hold results in Secret Word Mode; no release necessary
 *      - A long (over 3000 ms) hold will adjust the DST time; no release neccesary
 * 6) New secret word is a heart, cliche, I know but I'm a hopeless romantic what can I say?
 * 7) For hours that aren't SEVEN or NOON, TIL has been replaced with a vertical TO
 */

/* HOW THE LIGHT PATTERN WORKS:
  > The prefix (half past, quarter to, etc) will be a separate color from the hours.
  > Hours will be one of two solid colors, one for AM, one for PM
    - Light blue for AM, Purple for PM?
  > The prefix will fade from green -> red over the course of the 5 min interval
  */

/*
 *  Button 1: Big Word Mode (100ms); Secret Message Mode (5s)
 */

//------Define/Includes----------

#define UNCOMMENT_IF_IT_IS_DST 1
//#define DEBUG 1
//#define RESET_TIME 1 //Comment out to negate

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

#define NUM_BIRTHDAYS 2

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


//For Brightness
double DAY = 1;
double NIGHT = 0.5;
double brightness_mode;

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
int lastDSTAdjust;


//For Latch (Brightness) State
int lastLatchState; //For the changing of the DST BUTTON

//For Blinking Cursor
unsigned long lastBlink = 0;
bool blinkState = 0;

//Various
bool firstSecretTime = 1; //For the Secret Message timer
double seed; //For the color fade
uint8_t prevSecond = 0; //Tracker for the color fade, to increment each second
bool isBirthday = 0; //Flash the happy birthday when in birthday mode (will be true for entirety of brithday)
uint8_t digitPrevSecond = 0; //For the flashing of the big digits

//For Birthdays
uint8_t BIRTH_MONTH[NUM_BIRTHDAYS] = {8, 10};
uint8_t BIRTH_DAY[NUM_BIRTHDAYS] =   {12, 7};

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

#ifdef RESET_TIME
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 7)); //Resets to time of compilation w/ 6s adjust for upload time
#endif

  if (rtc.lostPower()) { //If RTC loses power, flash red lights 5 times, then adjust time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 7)); //Potential problem bc 5 seconds after runtime?
    for (int i = 0; i < 5; i++) {
      matrix.fillScreen(matrix.Color(255, 0, 0));
      delay(1000);
      matrix.fillScreen(0);
      delay(1000);
    }

  }

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LATCH_BUTTON, INPUT_PULLUP);

  mode = WORD_MODE;

  updateTime();
  seed = (theMinute % 5) * 60 + theSecond;

  lastDSTAdjust = BACK; //For DST

#ifdef UNCOMMENT_IF_IT_IS_DST //If it is currently DST, the next press of the button should make the time go backwards
  lastDSTAdjust = FWD;
#endif

  setColor(); //Initialize all colors

}

void loop() {
  matrix.clear();
  updateTime(); //Update the time and timing container (hour, min, sec) every clock cycle
  updateSeed();
  readButtonPush(); //Read the Mode button push
  checkLatch();

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
    default:
      matrix.fillScreen(matrix.Color(255, 0, 0)); //Fill screen with red for error
      break;
  }

  matrix.show();

}

// ----- AUXILIARY FUNCTIONS FROM VOID LOOP() ----
void updateTime() {
  theTime = rtc.now(); //Gather current time information

  // Find the date
  theYear = theTime.year();
  theMonth = theTime.month();
  theDay = theTime.day();

  //Find the time
  theHour = theTime.hour();
  theMinute = theTime.minute();
  theSecond = theTime.second();

  Serial.print(theHour);
  Serial.print(':');
  Serial.print(theMinute);
  Serial.print(':');
  Serial.print(theSecond);
  Serial.println();

  if (theHour == theMinute == theSecond == 0) { //Every new day check for a birthday
    checkBirthday();
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

      }
    }

    if (millis_held >= 1000 && current == LOW && !beenHeld) { //This is outside bc I dont want to have to release the button for it to ha[en
      mode = SECRET_WORD_MODE;
      beenHeld = 1;

      Serial.println();
      Serial.print("------------MODE CHANGE, New Mode: ");
      Serial.print(mode);
      Serial.print("--------------");
      Serial.println();

    }

    if (millis_held >= 3000 && current == LOW & !beenHeldLong) {
      beenHeldLong = 1;

      if (lastDSTAdjust == FWD) { //If the last time DST was asjusted was Forward, change time back

        lastDSTAdjust = BACK;
        DateTime hourBehind = (rtc.now() - TimeSpan(0, 1, 0, 0));
        rtc.adjust(hourBehind);

      } else if (lastDSTAdjust == BACK) {

        lastDSTAdjust = FWD;
        DateTime hourAhead = (rtc.now() + TimeSpan(0, 1, 0, 0));
        rtc.adjust(hourAhead);

      }
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

void checkLatch() {
  int currentLatchState = digitalRead(LATCH_BUTTON);

  if (currentLatchState != lastLatchState) { //If the button has changed positions
    if (currentLatchState == LOW) brightness_mode = DAY;
    if (currentLatchState == HIGH) brightness_mode = NIGHT;

    setColor();

    lastLatchState = currentLatchState; //Update the last state of the button
  }
}

void checkBirthday() { //Sets a boolean if there is a birthday occuring, then changes mode if birthday
  for (int i = 0; i < NUM_BIRTHDAYS; i++) {
    if (BIRTH_MONTH[i] == theMonth) {
      if (BIRTH_DAY[i] == theDay) {
        isBirthday = 1;
      } else isBirthday = 0;
    } else isBirthday = 0;
  }

  if (isBirthday) {
    mode = BIRTHDAY_MODE;
  }
}

// ----- STATE FUNCTIONS ---- The Display Modes

void displayWords() {

  color = fadeGreenToRed(seed);

  W_ITS();

  //Break up time into 5 minute intervals

  if (theMinute < 5) {          //0-5 past hour - Its CURRENT HOUR oclock
    lightHour(0);
    color = fadeGreenToRed(seed); //For the OCLOCK
    W_OCLOCK();
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
  int localMode; //1 for numbers, 2 for happy birthday

  if (isBirthday) { //If its a birthday, flash the happy birthday message every 5 seconds between
    if (millis() - digitPrevSecond > 5000) {
      localMode = 2; //Happy birthday
      digitPrevSecond = millis();
    } else localMode = 1; //Big Digits
  } else localMode = 1; //Big Digits

  switch (localMode) {
    case 1: //Digits
      parseTimeAndDisplayDigits();
      blinkCursor();
      break;
    case 2: //Happy birthday
      displayBirthday();
      break;
  }

}

void displaySecretWord() {
  int i, j;

  for (i = 0; i < 11; i++) {
    for (j = 0; j < 11; j++) {
      if (OBJECTS[1][i][j]) { //Hardcoded for the first object, as there is only 1
        matrix.drawPixel(j, i, OBJECT_COLOR);
      }
    }
  }

}

void displayBirthday() {
  color = fC(255, 230, 0);
  W_HAPPY();
  W_BIRTHDAY();
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

// ----- Other Auxiliary functions called elsewhere ----

// @PARAM int - set to 0 for current hour, set to 1 for next hour (30-60 min)
void lightHour (int hourFlag) { // Pushes out info for the hour, Called by displayWords()
  if (theHour >= 12) color = PM_COLOR;
  if (theHour < 12) color = AM_COLOR;
  int lightHour = (theHour + hourFlag) % 12;
  switch (lightHour) {
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
