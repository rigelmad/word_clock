#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define DATA_PIN 6

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(11, 11, DATA_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG, NEO_KHZ800 + NEO_GRB);

uint16_t color = matrix.Color(0,0,0);
uint16_t BIG_DIGIT_COLOR = matrix.Color(255, 0, 0);
uint16_t BIG_DIGIT_OUTSIDE_COLOR = matrix.Color(0,0,0);

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
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  matrix.setBrightness(175);

  matrix.begin();
  matrix.show(); // Initialize all pixels to 'off'
}

void loop() {

  color = matrix.Color(255,0,0);

  for(int i = 0; i < 11; i++){
    for(int j = 0; j < 11; j++){
      light(i,j);
    }
  }
  
  matrix.show();

}

//Misleading name, actually does not push info out to matrix
void light(uint8_t across, uint8_t down) { //Pushes out info to light a specific pixel- DOES NOT UPDATE SCREEN
  matrix.drawPixel(across, down, color);
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
