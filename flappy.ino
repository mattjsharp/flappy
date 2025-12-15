/*
     ________    ___    ____  ______  __
    / ____/ /   /   |  / __ \/ __ \ \/ /
   / /_  / /   / /| | / /_/ / /_/ /\  /
  / __/ / /___/ ___ |/ ____/ ____/ / /
 /_/   /_____/_/  |_/_/   /_/     /_/
       ____  ________  ____
      / __ )/  _/ __ \/ __ \
     / __  |/ // /_/ / / / /
    / /_/ // // _, _/ /_/ /
   /_____/___/_/ |_/_____/

                  ███████████████
            ██████         ███   ███
         ███            ███         ███
   ████████████         ███      ███   ███
███            ███      ███      ███   ███
███               ███      ███         ███
███               ███         ███████████████
   ███         ███         ███               ███
      █████████         ███   ███████████████
      ███                  ███            ███
         ██████               ████████████
               ███████████████
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_PIN 2
#define GRAVITY_STRENGTH 2
#define FLAP_STRENGTH 20
#define TOP_OFFSET 0
#define BOTTOM_OFFSET 1
#define TAIL_OFFEST 2
#define NUM_PIPES 3
#define PIPE_WIDTH 24
#define PIPE_GAP 40

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const int16_t FLAPPY_WIDTH = 16;
const int16_t FLAPPY_HEIGHT = 12;
int16_t flappyPosX, flappyPosY;
static const unsigned char PROGMEM flappy_bmp[] =
{ B00000011, B11100000,
  B00001100, B01010000,
  B00010000, B10001000,
  B01111000, B10010100,
  B10000100, B10010100,
  B10000010, B01000100,
  B10000010, B00111110,
  B01000100, B01000001,
  B00111000, B10111110,
  B00100000, B01000010,
  B00011000, B00111100,
  B00000111, B11000000};

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint8_t score;
int elevation;
bool isFlapping;
bool prevPress;
bool gameOver;
bool hitPipe;

struct Pipe {
  int xPos;
  int yPos;
};
Pipe pipes[NUM_PIPES];

void setup() {
  Serial.begin(9600);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.invertDisplay(true);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Init Game
  GAME:
  score = 0;
  elevation = 50;
  flappyPosX = 25;
  isFlapping = false;
  prevPress = false;
  gameOver = false;
  hitPipe = false;

  pipes[0] = {120, random(15, 60)};
  pipes[1] = {180, random(15, 60)};
  pipes[2] = {240, random(15, 60)};

  while (!gameOver) {
    if (!digitalRead(BUTTON_PIN)) {
      if (!prevPress) {
        isFlapping = true;
      } else {
        isFlapping = false;
      }
      prevPress = true;
    } else {
      isFlapping = false;
      prevPress = false;
    }

    nextFrame();
  }
  while(true) {
    if (!digitalRead(BUTTON_PIN)) {
      if (!prevPress) {
        prevPress = true;
        goto GAME;
      }
      prevPress = true;
    } else {
      prevPress = false;
    }
  }
}

void loop() {}

void drawFlappy() {
  display.drawBitmap(
    flappyPosX, flappyPosY,
    flappy_bmp, FLAPPY_WIDTH, FLAPPY_HEIGHT, 1);
}

void nextFrame(void) {
  display.clearDisplay();

  updateFlappy();
  updatePipes();
  updateScore();

  display.display();
}

void updateFlappy() {
  drawFlappy();
  if (checkCollisions()) {
    prevPress = true;
    gameOver = true;
  }

  if (isFlapping) {
    elevation += FLAP_STRENGTH;
  } else {
    elevation -= GRAVITY_STRENGTH;
  }
  flappyPosY = map(elevation, 100, 0, -FLAPPY_HEIGHT , SCREEN_HEIGHT - FLAPPY_HEIGHT - 1);
  //drawFlappy();
}

void updatePipes(void) {
  int xPix;
  int yPix;
  for (int i = 0; i < NUM_PIPES; i++) {
    xPix = map(pipes[i].xPos, 100, PIPE_WIDTH, SCREEN_WIDTH - 1, 0);
    yPix = map(pipes[i].yPos, 0, 100, SCREEN_HEIGHT - 1, 0);
    
    // Bottom Pipe
    display.drawRect(
      xPix,
      yPix,
      PIPE_WIDTH, 10, WHITE
    );
    display.drawRect( // Bottom Base
      xPix + 2,
      yPix + 9,
      PIPE_WIDTH - 4, 50, WHITE
    );

    // Top Pipe
    display.drawRect(
      xPix,
      yPix - PIPE_GAP,
      PIPE_WIDTH, 10, WHITE
    );
    display.drawRect( // Top Base
      xPix + 2,
      -50,
      PIPE_WIDTH - 4, abs(-50 - yPix) - PIPE_GAP + 1, WHITE
    );
    
    // Update Score
    if (xPix == (flappyPosX - (FLAPPY_WIDTH / 2))) {
      score++;
    }

    // Check collision
    if (xPix <= flappyPosX + FLAPPY_WIDTH &&           // Front of Flappy
        xPix + PIPE_WIDTH >= flappyPosX + TAIL_OFFEST) // Back of Flappy
    {
      if (flappyPosY + FLAPPY_HEIGHT >= yPix + BOTTOM_OFFSET || // Bottom Pipe
          flappyPosY <= (yPix - PIPE_GAP) + 10 - TOP_OFFSET) // Top Pipe
      { 
        hitPipe = true;
      }
    }

    pipes[i].xPos--;

    // Resetting Pipe Position
    if (pipes[i].xPos <= 0)  { 
      pipes[i].xPos = 180;
      pipes[i].yPos = random(10, 60); 
    }
  }
}

void updateScore(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5, 1);
  display.print(score);
}

bool checkCollisions(void) {
  if (elevation < 0) {
    gameOver = true;
    return true;
  }

  if (hitPipe) {
    return true;
  }

  return false;
}
