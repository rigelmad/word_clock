#define DEBUG 1

#define BUTTON 7 //The Mode Button

#define WORD_MODE 0
#define BIG_DIGIT_MODE 1
#define BIRTHDAY_MODE 2
#define SECRET_WORD_MODE 3

//For new method (time held)

int current;         // Current state of the button
long millis_held;    // How long the button was held (milliseconds)
long secs_held;      // How long the button was held (seconds)
long prev_secs_held; // How long the button was held in the previous check
byte previous = HIGH;
unsigned long firstTime; // how long since the button was first pressed

bool beenHeld = 0;
bool beenHeldLong = 0;

//For Brightness
double DAY = 1;
double NIGHT = 0.5;
double brightness_mode;

//For Master Mode
int mode;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(BUTTON, INPUT_PULLUP);

  brightness_mode = DAY;
  mode = WORD_MODE;

}

void loop() {
  newReadButtonPush(); //Read the Mode button push
}

void newReadButtonPush() {
  current = digitalRead(BUTTON);

  // if the button state changes to pressed, remember the start time
  if (current == LOW && previous == HIGH /*&&(millis() - firstTime) > 50*/) {
    firstTime = millis();
    Serial.println("Pushed");
  }

  millis_held = (millis() - firstTime);
  secs_held = millis_held / 1000;

  if (millis_held > 50) {

    // check if the button was released since we last checked
    if (current == HIGH && previous == LOW) {
      // HERE YOU WOULD ADD VARIOUS ACTIONS AND TIMES FOR YOUR OWN CODE
      // ===============================================================================

      if (millis_held < 1000) { //Regular, one-press
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

      // ===============================================================================
    }

    if (millis_held >= 3000 && current == LOW & !beenHeldLong){
      beenHeldLong = 1;
      Serial.println("DST Adjust");
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


  }

  if (beenHeld && current == HIGH) {
    beenHeld = 0;
  }

  if(beenHeldLong && current == HIGH){
    beenHeldLong = 0;
  }

  previous = current;

}
