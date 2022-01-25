// To connect to the serial console, do: screen /dev/ttyACM0
// Then, to disconnect, do ctrl+a \ y
// The IDE can't upload code while you're connected.

#include <Adafruit_CircuitPlayground.h>
#include <FastLED.h>
#include "Adafruit_ZeroFFT.h"

#define TAU 6.2831853071

#define N_LEDS 300
#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 15
#define LED_PIN 6
#define GRAV_ANGLE -1.6 // tilt correction in case the board is glued in at an angle
const float SIN_GRAV_ANGLE = sin(GRAV_ANGLE);
const float COS_GRAV_ANGLE = cos(GRAV_ANGLE);

#define INITIAL_BRIGHTNESS 150
#define MAX_CMD_LEN 256
#define MAX_HEARTS 4
#define TEXT_COLOR CHSV(200, 190, 199)

const int ENTER = 13;
const int BACKSPACE = 127; //todo: add backspace support?
const int DELETE = 195;
#define F0 240 // F1 is 241, F2 is 242, up to F12
#define TAB 9


int incomingByte; // for incoming serial data
bool light = false; // test

char commandBytes[MAX_CMD_LEN];
int bytesTyped = 0;
String commandString;
bool newCommandFlag;
bool staleCommandFlag;

word font[64] = {0b0000000000000000, 0b0000011101000000, 0b1100000000110000, 0b1111101010111110, 0b1101011111101100, 0b1001100100110010, 0b1111111101001110, 0b0000011000000000, 0b0111010001000000, 0b0000010001011100, 0b1010101110101010, 0b0010001110001000, 0b0000100010000000, 0b0010000100001000, 0b0000000001000000, 0b0000101110100000, 0b1111110001111110, 0b1000111111000010, 0b1011110101111010, 0b1000110101111110, 0b1110000100111110, 0b1110110101101110, 0b1111110101101110, 0b1000010000111110, 0b1111110101111110, 0b1110010100111110, 0b0000001010000000, 0b0000101010000000, 0b0010001010100010, 0b0101001010010100, 0b1000101010001000, 0b1000010101111000, 0b0111010001011010, 0b1111110100111110, 0b1111110101110110, 0b0111010001100010, 0b1111110001011100, 0b1111110101100010, 0b1111110100100000, 0b0111110001100110, 0b1111100100111110, 0b1000111111100010, 0b1000111111100000, 0b1111100100110110, 0b1111100001000010, 0b1111111000111110, 0b1111110000011110, 0b0111110001111100, 0b1111110100111000, 0b0111010011011010, 0b1111110100010110, 0b0110110101101100, 0b1000011111100000, 0b1111000001111110, 0b1111000011111100, 0b1111100011111110, 0b1101100100110110, 0b1110100101111110, 0b1001110101110010, 0b1111110001000000, 0b1000001110000010, 0b0000010001111110, 0b0100010000010000, 0b0000100001000010};

CRGB raw_leds[N_LEDS];
CRGB rgb_leds[N_LEDS];
CHSV hsv_leds[N_LEDS];

// for converting to boustrophedon
int zmap[N_LEDS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299};

struct point {
  int8_t x = 0;
  int8_t y = 0;
  bool alive = false;
};

byte heartSprite[] = { // len=7
  0b01000100,
  0b11101110,
  0b11111110,
  0b11111110,
  0b01111100,
  0b00111000,
  0b00010000
};

void rgb_show() {
  for (int dot = 0; dot < N_LEDS; dot++) {
    raw_leds[zmap[dot]] = rgb_leds[dot];
  }
  FastLED.show();
}

void hsv_show() {
  for (int dot = 0; dot < N_LEDS; dot++) {
    rgb_leds[dot] = hsv_leds[dot];
  }
  rgb_show();
}

void hsv_linfade(byte amount) { // improve with reference to led?
  for (int dot = 0; dot < N_LEDS; dot++) {
    if (hsv_leds[dot].value <= amount) {
      hsv_leds[dot].value = amount;
    }
    hsv_leds[dot].value = hsv_leds[dot].value - amount;
  }
}

void hsv_expfade() {
  for (int dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot].value *= 0.75;
    if (hsv_leds[dot].value < 2) {
      hsv_leds[dot].value = 0;
    }
  }
}

void hsv_clear() {
  for (int dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot].value = 0;
  }
}

void hsv_zero() { // todo: see if we can replace hsv_clear with this
  for (int dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot] = CHSV(0, 0, 0);
  }
}

void blit(byte sprite[], byte len, int8_t spriteX, int8_t spriteY, CHSV color) {
  byte row;
  
  for (int8_t y = 0; y < len; y++) {
    if (spriteY + y >= SCREEN_HEIGHT) {
      break;
    }
    row = sprite[y];
    for (int8_t x = 0; x < 8; x++) {
      if (spriteX + x >= SCREEN_WIDTH) {
        break;
      }
      if ((row & 0b10000000) && (spriteX + x >= 0) && (spriteY + y >= 0)) {
        hsv_leds[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
      }
      row = row << 1;
    }
  }
}
void blit(uint16_t sprite[], byte len, int8_t spriteX, int8_t spriteY, CHSV color) {
  uint16_t row;
  
  for (int8_t y = 0; y < len; y++) {
    if (spriteY + y >= SCREEN_HEIGHT) {
      break;
    }
    row = sprite[y];
    for (int8_t x = 0; x < 16; x++) {
      if (spriteX + x >= SCREEN_WIDTH) {
        break;
      }
      if ((row & bit(15)) && (spriteX + x >= 0) && (spriteY + y >= 0)) {
        hsv_leds[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
      }
      row = row << 1;
    }
  }
}
void blit(uint32_t sprite[], byte len, int8_t spriteX, int8_t spriteY, CHSV color) {
  uint32_t row;
  int x; int y;
  
  for (y = 0; y < len; y++) {
    if (spriteY + y >= SCREEN_HEIGHT) {
      break;
    }
    row = sprite[y];
    for (x = 0; x < 32; x++) {
      if (spriteX + x >= SCREEN_WIDTH) {
        break;
      }
      if ((row & bit(31-12)) && (spriteX + x >= 0) && (spriteY + y >= 0)) {
        hsv_leds[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
      }
      row = row << 1;
    }
  }
}

void blitChr(char character, int chrX, int chrY, CHSV color) {
  if (character > 96) { // convert lowercase to uppercase
    character -= 32;
  }
  word sprite = font[character - 32];
  byte i = 0;
  int sumX; int sumY;
  for (int x = 0; x < 3; x++) {
    sumX = chrX + x;
    if (sumX >= SCREEN_WIDTH) {
      break;
    }
    if (sumX < 0) {
      continue;
    }
    for (int y = 0; y < 5; y++) {
      sumY = chrY + y;
      if (sumY >= SCREEN_HEIGHT) {
        break; 
      }
      if (sumY < 0) {
        continue;
      }
      if (bitRead(sprite, 15 - (x * 5 + y))) {
        hsv_leds[sumY * SCREEN_WIDTH + sumX] = color;
      }
    }
  }
}

void blitStr(char str[], int x, int y, CHSV color) {
  int i = 0;
  char chr = str[0];
  int sumX = x;
  do {
    blitChr(chr, sumX, y, color);
    i++;
    chr = str[i];
    sumX += 4;
  }
  while ((chr != 0) and (sumX < SCREEN_WIDTH));
}

void clear_leds() {
  FastLED.clear();
  FastLED.show();
}

void resetCommandBytes() {
  // flush the command bytes.
  // used when executing the command
  for (int i = 0; i < MAX_CMD_LEN; i++) {
    commandBytes[i] = 0;
  }
  bytesTyped = 0;
  newCommandFlag = false;
  staleCommandFlag = false;
}

void listenForCommand() {
  // reads Serial input, sets newCommandFlag if there's a command waiting
  if (staleCommandFlag) {
    resetCommandBytes();
  }
  while (Serial1.available() > 0 and not newCommandFlag) {
    // read the incoming byte:
    incomingByte = Serial1.read();

    if (incomingByte == DELETE) { // clear out what was typed
      resetCommandBytes();
      Serial.println("");
      continue;
    }
    if (incomingByte == (F0 + 1)) { adjustBrightness(-2); continue; }
    if (incomingByte == (F0 + 2)) { adjustBrightness(2); continue; }
    if (incomingByte == (F0 + 3)) { adjustBrightness(-16); continue; }
    if (incomingByte == (F0 + 4)) { adjustBrightness(16); continue; }
    if (incomingByte == TAB) { hsv_clear(); incomingByte = 0; continue; } // clear and stop

    Serial.write('[');
    Serial.write(incomingByte);
    Serial.write(' ');
    Serial.print(incomingByte, DEC);
    Serial.println(']');

    if (incomingByte == ENTER) { // replace Enter with string terminator
      incomingByte = 0;
    }

    commandBytes[bytesTyped] = incomingByte;
    bytesTyped++;

    if (incomingByte == 0) {
      newCommandFlag = true;
    }
  }
}

bool prefix (const char *str, const char *pre) {
  // checks if str begins with pre.
  return strncmp(pre, str, strlen(pre)) == 0;
}
bool strsMatch(const char *str1, const char *str2) {
  return (strcmp(str1, str2) == 0);
}

void executeCommand() {
  //commandString = String(commandBytes);
  newCommandFlag = false;
  staleCommandFlag = true;
  Serial1.print("Command received: ");
  Serial1.println(commandBytes);
  if (strsMatch(commandBytes, "race")) {
    race();
  }
  else if (strsMatch(commandBytes, "racefade")) {
    racefade();
  }
  else if (strsMatch(commandBytes, "heart")) {
    heart();
  }
  else if (strsMatch(commandBytes, "hearts") or strsMatch(commandBytes, "h")) {
    hearts();
  }
  else if (strsMatch(commandBytes, "stop")) {
    hsv_clear();
    hsv_show();
  }
  else if (strsMatch(commandBytes, "matrix") or strsMatch(commandBytes, "m")) {
    matrix();
  }
  else if (strsMatch(commandBytes, "drop")) {
    drop();
  }
  else if (strsMatch(commandBytes, "drops") or strsMatch(commandBytes, "d")) {
    drops();
  }
  else if (strsMatch(commandBytes, "spiral") or strsMatch(commandBytes, "s")) {
    spiral();
  }
  else if (strsMatch(commandBytes, "eye") or strsMatch(commandBytes, "e")) {
    eye();
  }
  else if (prefix(commandBytes, "text ")) {
    text(&commandBytes[5]);
  }
  else if (prefix(commandBytes, "t")) { // example: thello world
    text(&commandBytes[1]);
  }
  else if (strsMatch(commandBytes, "life")) {
    life();
  }
  else if (strsMatch(commandBytes, "lissajous") or strsMatch(commandBytes, "liss")) {
    lissajous();
  }
  else if (strsMatch(commandBytes, "mic")) {
    mic();
  }
  else if (strsMatch(commandBytes, "rain") or strsMatch(commandBytes, "r")) {
    rain();
  }
  else if (strsMatch(commandBytes, "ball") or strsMatch(commandBytes, "b")) {
    ball();
  }
  else if (strsMatch(commandBytes, "melt")) {
    melt();
  }
  else if (strsMatch(commandBytes, "smile") or strsMatch(commandBytes, ":)")) {
    smile();
  }
  else if (strsMatch(commandBytes, "ow") or strsMatch(commandBytes, "Xd")) {
    ow();
  }
  else if (strsMatch(commandBytes, "furry") or strsMatch(commandBytes, ":3")) {
    furry();
  }
  else if (strsMatch(commandBytes, "furrywink") or strsMatch(commandBytes, ";3")) {
    furrywink();
  }
  else if (strsMatch(commandBytes, "error") or strsMatch(commandBytes, "err")) {
    error();
  }
  else if (strsMatch(commandBytes, "sad") or strsMatch(commandBytes, ":(")) {
    sad();
  }
  else if (strsMatch(commandBytes, "cry") or strsMatch(commandBytes, ":'(")) {
    cry();
  }
  else if (strsMatch(commandBytes, "pipes") or strsMatch(commandBytes, "p")) {
    pipes();
  }
  else if (strsMatch(commandBytes, "wave") or strsMatch(commandBytes, "w")) {
    wave();
  }
  else if (strsMatch(commandBytes, "jack") or strsMatch(commandBytes, "j")) {
    jack();
  }
  else if (strsMatch(commandBytes, "u")) {
    CircuitPlayground.setPixelColor(0, 255, 0, 4);
    delay(1000);
    CircuitPlayground.setPixelColor(0, 255, 0, 0);
  }
  else if (strsMatch(commandBytes, "blush") or strsMatch(commandBytes, "bl")) {
    blush();
  }
  else if (strsMatch(commandBytes, "bu") or strsMatch(commandBytes, "brighter")) {
    adjustBrightness(25);
  }
  else if (strsMatch(commandBytes, "bd") or strsMatch(commandBytes, "darker")) {
    adjustBrightness(-25);
  }
  else if (strsMatch(commandBytes, "brightest")) {
    adjustBrightness(255);
  }
  else if (strsMatch(commandBytes, "dark")) {
    FastLED.setBrightness(16);
  }
  else if (strsMatch(commandBytes, "help")) {
    Serial1.println("animation commands: h(earts), m(atrix), d(rops), s(piral), e(ye), t[english text], C[chinese text], life, liss, r(ain), b(all), melt, :), ow, :3, ;3, error, :(, cry, p(ipes), w(ave), bl(ush)");
    Serial1.println("brightness control: bu / brighter, bd / darker, brightest, dark");
  }
  else {
    Serial1.print(">~<\" Error! Invalid command ");
    Serial1.println(commandString);
    //CircuitPlayground.playTone(300, 100);
  }
}

void setup() {
  CircuitPlayground.begin(50); // brightness of onboard LEDs
  Serial.begin(9600); // opens usb serial port, sets data rate to 9600 bps
  Serial1.begin(9600);//
  FastLED.addLeds<NEOPIXEL, LED_PIN>(raw_leds, N_LEDS);
  FastLED.setBrightness(INITIAL_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 900);
  // throw out the initial erroneous byte sent at startup
  delay(10);
  Serial1.read();
  Serial1.println("Hewwo! :3");
  eye(); // play eye animation at startup
}

void loop() {

  if (CircuitPlayground.rightButton()) {
    delay(600000);
  }
  
  listenForCommand();
  if (newCommandFlag) {
    executeCommand();
  }
}

void adjustBrightness(int adjustment) {
  FastLED.setBrightness(min(255, max(0, FastLED.getBrightness() + adjustment)));
  hsv_show();
}
