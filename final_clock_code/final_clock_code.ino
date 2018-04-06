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

#define DEBUG 1
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
#define DST_BUTTON 5

#define WORD_MODE 0
#define BIG_DIGIT_MODE 1
#define BIRTHDAY_MODE 2
#define SECRET_WORD_MODE 3

#define NUM_BIRTHDAYS 1

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

//Global var color
uint16_t color;
uint16_t AM_COLOR = matrix.Color(0, 255, 196);
uint16_t PM_COLOR = matrix.Color(239, 0, 255);
uint16_t BIG_DIGIT_COLOR = matrix.Color(0, 255, 0);
uint16_t BIG_DIGIT_OUTSIDE_COLOR = matrix.Color(0, 0, 0);
uint16_t CURSOR_COLOR = makeColor('R');

//For Debounce
unsigned long lastTimeChanged;
int previousState; //For the button debounce
int buttonState = HIGH;
int lastButtonState = LOW;

//For DST State
int lastDSTstate; //For the changing of the DST BUTTON

//For Blinking Cursor
unsigned long lastBlink = 0;
bool blinkState = 0;

//Various
bool firstSecretTime = 1; //For the Secret Message timer
double seed; //For the color fade
uint8_t prevSecond = 0; //Tracker for the color fade, to increment each second
bool isBirthday = 0; //Flash the happy birthday when in birthday mode (will be true for entirety of brithday)
uint8_t digitPrevSecond = 0; //For the flashing of the big digits


uint8_t BIRTH_MONTH[NUM_BIRTHDAYS] = {8};
uint8_t BIRTH_DAY[NUM_BIRTHDAYS] =   {12};

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

// ---- END Number Character declarations ----

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  matrix.begin(); //across, down
  matrix.setTextWrap(true); //Used for big digit display mode

  rtc.begin(); //start the time module

#ifdef RESET_TIME
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 6)); //Resets to time of compilation w/ 6s adjust for upload time
#endif

  if (rtc.lostPower()) { //If RTC loses power, flash red lights 5 times, then adjust time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 6)); //Potential problem bc 5 seconds after runtime?
    for (int i = 0; i < 5; i++) {
      matrix.fillScreen(matrix.Color(255, 0, 0));
      delay(1000);
      matrix.fillScreen(0);
      delay(1000);
    }

  }

  pinMode(BUTTON, INPUT_PULLUP);
  previousState = digitalRead(BUTTON);

  pinMode(DST_BUTTON, INPUT_PULLUP); // Ground DST Pushbutton, LOW means on
  //lastDSTstate = digitalRead(DST_BUTTON);
  if(digitalRead(DST_BUTTON) == HIGH)

  mode = WORD_MODE;

}

void loop() {
  matrix.clear();
  updateTime(); //Update the time and timing container (hour, min, sec) every clock cycle
  updateSeed();
  readButtonPush(); //Read the Mode button push
  checkDST();

//  Serial.println();
//  Serial.print("**********************");
//  Serial.print(seed);
//  Serial.print("***************");
//  Serial.println();

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

  //delay(1000);
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

#ifdef DEBUG

  Serial.print(theHour);
  Serial.print(':');
  Serial.print(theMinute);
  Serial.print(':');
  Serial.print(theSecond);
  Serial.println();

#endif

  if (theHour == theMinute == theSecond == 0) { //Every new day check for a birthday
    checkBirthday();
  }
}

void readButtonPush() {
  int reading = digitalRead(BUTTON);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) { //On any change of the switch
    // reset the debouncing timer
    lastTimeChanged = millis();
  }

  if ((millis() - lastTimeChanged) > 2000) {
    if (reading == LOW) {
      mode = SECRET_WORD_MODE;
      firstSecretTime = 1;

#ifdef DEBUG
      Serial.println();
      Serial.print("------------MODE CHANGE, New Mode: ");
      Serial.print(mode);
      Serial.print("--------------");
      Serial.println();
#endif

      lastButtonState = HIGH;
      return;
    }
  }

  if ((millis() - lastTimeChanged) > 50) { //If the swtich was last changed 50 ms ago
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        // CHANGE MODE HERE
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

#ifdef DEBUG
        Serial.println();
        Serial.print("------------MODE CHANGE, New Mode: ");
        Serial.print(mode);
        Serial.print("--------------");
        Serial.println();
#endif

        lastButtonState = reading;
        return;
      }
      Serial.println(50);
    }

  }

  lastButtonState = reading;

}

void checkDST() {
  int currentState = digitalRead(DST_BUTTON);

  if (currentState != lastDSTstate) { //If the button has changed positions
    if (currentState == LOW) { //If the button is PRESSED IN
      DateTime hourAhead = (rtc.now() + TimeSpan(0, 1, 0, 0));
      rtc.adjust(hourAhead);
    }

    if (currentState == HIGH) { //If the button is NOT PRESSED IN
      DateTime hourBehind = (rtc.now() - TimeSpan(0, 1, 0, 0));
      rtc.adjust(hourBehind);
    }

    lastDSTstate = currentState; //Update the last state of the button
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
  } else if (theMinute < 35) {   //30-35 - Its half way past CURRENT HOUR
    W_HALF();
    W_WAY();
    W_PAST();
    lightHour(0);
  } else if (theMinute < 40) {  //35-40 - Its twenty five til NEXT HOUR
    W_TWENTY();
    W_FIVE();
    W_TIL();
    lightHour(1);
  } else if (theMinute < 45) {  //40-45 - Its twenty til NEXT HOUR
    W_TWENTY();
    W_TIL();
    lightHour(1);
  } else if (theMinute < 50) {  //45-50 - Its a quarter til NEXT HOUR
    W_A();
    W_QUARTER();
    W_TIL();
    lightHour(1);
  } else if (theMinute < 55) {  //50-55 - Its ten til NEXT HOUR
    W_TEN();
    W_TIL();
    lightHour(1);
  } else if (theMinute < 60) {  //55-60 - Its five til NEXT HOUR
    W_FIVE();
    W_TIL();
    lightHour(1);
  }
}

void displayBigDigits() { //HAVE TO TEST WHETHER IT LOOKS NATURAL TO DO 4 DIGIT DISPLAY ON ARRAY
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

  color = makeColor('R');
  W_I();
  W_LOVE();
  W_YOU();

  if (firstSecretTime) {
    matrix.show();
    delay(3000);
    firstSecretTime = 0;
  }
}

void displayBirthday() {
  color = matrix.Color(255, 230, 0);
  W_HAPPY();
  W_BIRTHDAY();
}

// ----- Functions for COLOR --------

uint16_t makeColor(char letter) { //Sends red, green, or blue depending on given char input
  if (letter == 'R') {
    return matrix.Color(255, 0, 0);
  }

  if (letter == 'G') {
    return matrix.Color(0, 255, 0);
  }

  if (letter == 'B') {
    return matrix.Color(0, 0, 255);
  }

  return 0;
}

uint16_t fadeGreenToRed(double num) { //seed will be number from 0-299
  double r;
  double g;
  int b = 0;

  if (num < 150) { //Green is 255, red fades in from 0-255
    g = 255;
    r = num * 1.7; //Being that the increment is 5 minutes, and this is all 510 steps broken into 5*60 steps, changes once per second
  }

  if (num >= 150) { //Red is 255, green fades out from 255-0
    r = 255;
    g = (300 - num) * 1.7;
  }

  return matrix.Color((int) r, (int) g, b);
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

void lightTL(int num) { // Accepts input 1-9 (actually should really only be a zero or one or two), displays number in the top left quadrant
  int i, j;

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (CHAR_NUMS[num][i][j]) {
        matrix.drawPixel(j, i, BIG_DIGIT_COLOR);
      } else matrix.drawPixel(j, i, BIG_DIGIT_OUTSIDE_COLOR);
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
      } else matrix.drawPixel(j + offsetHorizontal, i, BIG_DIGIT_OUTSIDE_COLOR);
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
      } else matrix.drawPixel(j, i + offsetVertical, BIG_DIGIT_OUTSIDE_COLOR);
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
      } else matrix.drawPixel(j + offsetVerticalAndHorizontal, i + offsetVerticalAndHorizontal, BIG_DIGIT_OUTSIDE_COLOR);
    }
  }

}

void blinkCursor() {
  //  if (hasSecondChanged()) {
  //    if (theSecond % 2 == 0) { //Every 2 distinct seconds, set the color to the cursor color
  //      color = CURSOR_COLOR;
  //      C_CURSOR();
  //    } else {
  //      //color = matrix.Color(0, 0, 0); //Every offset two seconds, set the color to OFF
  //    }
  //  }

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
  light(5, 5);
  light(5, 6);
  light(4, 5);
  light(6, 5);
}

