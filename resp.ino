#define TAU 6.2831853071

#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif
#include <FastLED.h>


#ifdef Serial1    // this makes it not complain on compilation if there's no Serial1
  #define BLUEFRUIT_HWSERIAL_NAME      Serial1
#endif

#define BLUEFRUIT_UART_MODE_PIN        12    // Set to -1 if unused

#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4    // Optional but recommended, set to -1 if unused
#define LED_PIN 13



#define INITIAL_BRIGHTNESS 30
#define MAX_CMD_LEN 256
#define TEXT_COLOR CHSV(200, 190, 199)
#define ENTER 13

#define N_LEDS 84
#define N_DOTS 91
#define SCREEN_WIDTH 13
#define SCREEN_HEIGHT 7
int dotToLed[N_DOTS] = {
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,-1,
  23,22,21,20,19,18,17,16,15,14,13,12,11,
  24,25,26,27,28,29,30,31,32,33,34,35,36,
  49,48,47,46,45,44,43,42,41,40,39,38,37,
  50,51,52,53,54,55,56,57,58,59,60,61,62,
  75,74,73,72,71,70,69,68,67,66,65,64,63,
  76,77,78,79,-1,-1,-1,-1,-1,80,81,82,83
};
bool dotHasLed(int dot) {
  return (dot > 0) and (dot < N_DOTS) and (dotToLed[dot] > -1);
}

word font[64] = {0b0000000000000000, 0b0000011101000000, 0b1100000000110000, 0b1111101010111110, 0b1101011111101100, 0b1001100100110010, 0b1111111101001110, 0b0000011000000000, 0b0111010001000000, 0b0000010001011100, 0b1010101110101010, 0b0010001110001000, 0b0000100010000000, 0b0010000100001000, 0b0000000001000000, 0b0000101110100000, 0b1111110001111110, 0b1000111111000010, 0b1011110101111010, 0b1000110101111110, 0b1110000100111110, 0b1110110101101110, 0b1111110101101110, 0b1000010000111110, 0b1111110101111110, 0b1110010100111110, 0b0000001010000000, 0b0000101010000000, 0b0010001010100010, 0b0101001010010100, 0b1000101010001000, 0b1000010101111000, 0b0111010001011010, 0b1111110100111110, 0b1111110101110110, 0b0111010001100010, 0b1111110001011100, 0b1111110101100010, 0b1111110100100000, 0b0111110001100110, 0b1111100100111110, 0b1000111111100010, 0b1000111111100000, 0b1111100100110110, 0b1111100001000010, 0b1111111000111110, 0b1111110000011110, 0b0111110001111100, 0b1111110100111000, 0b0111010011011010, 0b1111110100010110, 0b0110110101101100, 0b1000011111100000, 0b1111000001111110, 0b1111000011111100, 0b1111100011111110, 0b1101100100110110, 0b1110100101111110, 0b1001110101110010, 0b1111110001000000, 0b1000001110000010, 0b0000010001111110, 0b0100010000010000, 0b0000100001000010};

CRGB leds[N_LEDS];
CRGB crgbs[N_DOTS];
CHSV chsvs[N_DOTS];
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

char commandBytes[MAX_CMD_LEN];
int incomingByte; // for incoming serial data
int bytesTyped = 0;
String commandString;
bool newCommandFlag;
bool staleCommandFlag;

struct point {
  int8_t x = 0;
  int8_t y = 0;
  bool alive = false;
};
struct huePoint {
  int8_t x = 0;
  int8_t y = 0;
  int8_t hue = 0;
  bool alive = false;
};

void rgb_show() {
  for (int dot = 0; dot < N_DOTS; dot++) {
    if (dotToLed[dot] > -1) {
      leds[dotToLed[dot]] = crgbs[dot];
    }
  }
  FastLED.show();
}
void hsv_show() {
  for (int dot = 0; dot < N_DOTS; dot++) {
    crgbs[dot] = chsvs[dot];
  }
  rgb_show();
}
void hsv_linfade(byte amount) { // improve with reference to led?
  for (int dot = 0; dot < N_DOTS; dot++) {
    if (chsvs[dot].value <= amount) {
      chsvs[dot].value = amount;
    }
    chsvs[dot].value = chsvs[dot].value - amount;
  }
}
void hsv_expfade() {
  for (int dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot].value *= 0.75;
    if (chsvs[dot].value < 2) {
      chsvs[dot].value = 0;
    }
  }
}
void hsv_clear() {
  for (int dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot].value = 0;
  }
}
void hsv_zero() { // todo: see if we can replace hsv_clear with this
  for (int dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot] = CHSV(0, 0, 0);
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
        chsvs[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
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
        chsvs[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
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
        chsvs[(spriteY + y) * SCREEN_WIDTH + spriteX + x] = color;
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
        chsvs[sumY * SCREEN_WIDTH + sumX] = color;
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
  while (ble.available() > 0 and not newCommandFlag) {
    // read the incoming byte:
    incomingByte = ble.read();

    Serial.write('[');
    Serial.write(incomingByte);
    Serial.write(' ');
    Serial.print(incomingByte, DEC);
    Serial.println(']');

    if (incomingByte == ENTER) { // replace Enter with string terminator // refactor?
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
  ble.print("command received: ");
  ble.println(commandBytes);
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
  else if (strsMatch(commandBytes, "matrixpurple") or strsMatch(commandBytes, "mp")) {
    matrix(CHSV(200, 100, 255));
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
  //else if (strsMatch(commandBytes, "rain") or strsMatch(commandBytes, "r")) {
    //rain();
  //}
  //else if (strsMatch(commandBytes, "ball") or strsMatch(commandBytes, "b")) {
  //  ball();
  //}
  else if (strsMatch(commandBytes, "cry") or strsMatch(commandBytes, ":'(")) {
    cry();
  }
  else if (strsMatch(commandBytes, "pipes") or strsMatch(commandBytes, "p")) {
    pipes();
  }
  else if (strsMatch(commandBytes, "wave") or strsMatch(commandBytes, "w")) {
    wave();
  }
  else if (strsMatch(commandBytes, "blush") or strsMatch(commandBytes, "bl")) {
    blush();
  }
  else if (strsMatch(commandBytes, "uwu")) { uwu(); }
  else if (strsMatch(commandBytes, "owo")) { owo(); }
  else if (strsMatch(commandBytes, "rainbowmatrix") or strsMatch(commandBytes, "rm")) { rainbowmatrix(); }
  else if (strsMatch(commandBytes, "rainbowdrops") or strsMatch(commandBytes, "rd")) { rainbowdrops(); }
  else if (strsMatch(commandBytes, "scan") or strsMatch(commandBytes, "sc")) { scan(0, false); }
  else if (strsMatch(commandBytes, "scanpurple") or strsMatch(commandBytes, "scp")) { scan(210, false); }
  else if (strsMatch(commandBytes, "scantrans") or strsMatch(commandBytes, "sct")) { scantrans(); }
  else if (strsMatch(commandBytes, "rainbowscan") or strsMatch(commandBytes, "rsc")) { scan(0, true); }
  else if (strsMatch(commandBytes, "baton") or strsMatch(commandBytes, "b")) { baton(222, false); }
  else if (strsMatch(commandBytes, "rainbowbaton") or strsMatch(commandBytes, "rb")) { baton(0, true); }
  else if (strsMatch(commandBytes, "underpass") or strsMatch(commandBytes, "und")) { underpass(); }
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
    ble.println("animation commands: h(earts), m(atrix), (m)atrix(p)urple, (r)ainbow(m)atrix, d(rops), (r)ainbow(d)rops, s(piral), e(ye), (b)aton, (r)ainbow(b)aton, (sc)an, (sc)an(p)urple, (sc)an(t)rans, (und)erpass, (t[text], life, liss, p(ipes), w(ave), bl(ush), uwu, owo, cry");
    ble.println("brightness control: bu / brighter, bd / darker, brightest, dark");
  }
  else {
    ble.print(">~<\" Error! Invalid command ");
    ble.println(commandString);
  }
}

void setup() {
  ble.begin();
  ble.setMode(BLUEFRUIT_MODE_COMMAND);
  ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Respirator"));
  ble.setMode(BLUEFRUIT_MODE_DATA);
  Serial.begin(9600); // opens usb serial port, sets data rate to 9600 bps
  Serial1.begin(9600);//
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, N_LEDS);
  FastLED.setBrightness(INITIAL_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 900);
  // todo: wait for connection?
  ble.println("Hewwo! :3");
  eye();//racefade(); // startup animation
}

void loop() {
  listenForCommand();
  delay(50);
  if (newCommandFlag) {
    executeCommand();
  }
}

void adjustBrightness(int adjustment) {
  FastLED.setBrightness(min(255, max(0, FastLED.getBrightness() + adjustment)));
  hsv_show();
}

/*
void loop() {
  // put your main code here, to run repeatedly:
  if (ble.isConnected() && ble.available()) {
    int command = ble.read();
    ble.write("Received command ");
    ble.write(command);
    Serial.write(command);
    Serial1.write(command);
    ble.write(".  ");
  }
}*/

void race() {
  int dot = 0;
  while (not newCommandFlag) {
    crgbs[dot] = CRGB(120, 0, 200);
    rgb_show();
    // clear this led for the next time around the loop
    crgbs[dot] = CRGB::Black;
    delay(16);
    listenForCommand();
    dot++;
    dot %= N_DOTS;
  }
  clear_leds();
}
void racefade() {
  int dot = 0;
  while (not newCommandFlag) {
    chsvs[dot] = CHSV(190, 200, 255);
    hsv_show();
    hsv_expfade();
    delay(25);
    listenForCommand();
    dot++;
    dot %= N_DOTS;
  }
  clear_leds();
}

byte heartSprite[] = { // len=7
  0b01000100,
  0b11101110,
  0b11111110,
  0b11111110,
  0b01111100,
  0b00111000,
  0b00010000
};

void heart() {
  blit(heartSprite, 7, 3, 2, CHSV(224, 200, 100));
  hsv_show();
}

#define MAX_HEARTS 1
void hearts() { // assumes screen width is at least 7
  point points[MAX_HEARTS];
  bool ok = true;
  while (not newCommandFlag) {
    hsv_clear();
    for (byte i = 0; i < MAX_HEARTS; i++) {
      if (points[i].alive) {
        points[i].y -= 1;
        if (points[i].y < -8) {
          points[i].alive = false;  
        }
        else {
          blit(heartSprite, 7, points[i].x, points[i].y, CHSV(224, 200, 130));
        }
      }
      else { // it's not alive. try to spawn it back in
        if (random8() > 64) { // random chance to attempt spawn
          continue;
        }
        points[i].x = random8(SCREEN_WIDTH - 7); // choose a random x coordinate
        ok = true;
        for (byte j = 0; j < MAX_HEARTS; j++) { // make sure it won't overlap others
          if ((points[j].y > 5) and (abs(points[j].x - points[i].x) < 8)) {
            ok = false;
            break;
          }
        }
        if (ok) {
          points[i].y = SCREEN_HEIGHT;
          points[i].alive = true;
        }
      }
    }
    hsv_show();
    listenForCommand();
    delay(50);
  }
}

#define N_MATRIX_POINTS 8
void matrix() {
  matrix(CHSV(105, 100, 255));
}
void matrix(CHSV color) {
  point points[N_MATRIX_POINTS];
  int dot;
  byte i;
  while (not newCommandFlag) {
    for (dot = 0; dot < N_DOTS; dot++) {
      chsvs[dot].value *= 0.8;
      chsvs[dot].saturation += (256 - chsvs[dot].saturation) * 0.5;
    }
    for (i = 0; i < N_MATRIX_POINTS; i++) {
      if (points[i].alive) {
        points[i].y++;
        if (points[i].y < SCREEN_HEIGHT) {
          chsvs[points[i].y * SCREEN_WIDTH + points[i].x] = color;
        }
        else {
          points[i].alive = false;
        }
      }
      else if (random8() < 32) {
        points[i].x = random8(SCREEN_WIDTH);
        points[i].y = -1;
        points[i].alive = true;
      }
    }
    hsv_show();
    listenForCommand();
    delay(105);
  }
}
void rainbowmatrix() {
  huePoint points[N_MATRIX_POINTS];
  int dot;
  byte i;
  while (not newCommandFlag) {
    for (dot = 0; dot < N_DOTS; dot++) {
      chsvs[dot].value *= 0.8;
      chsvs[dot].saturation += (256 - chsvs[dot].saturation) * 0.5;
    }
    for (i = 0; i < N_MATRIX_POINTS; i++) {
      if (points[i].alive) {
        points[i].y++;
        if (points[i].y < SCREEN_HEIGHT) {
          chsvs[points[i].y * SCREEN_WIDTH + points[i].x] = CHSV(points[i].hue, 105, 255);
        }
        else {
          points[i].alive = false;
        }
      }
      else if (random8() < 32) {
        points[i].x = random8(SCREEN_WIDTH);
        points[i].y = -1;
        points[i].hue = random8();
        points[i].alive = true;
      }
    }
    hsv_show();
    listenForCommand();
    delay(66);
  }
}

void drop() {
  drop(160);
}
void drop(int8_t hue) {
  float centerX = random8(SCREEN_WIDTH);
  float centerY = random8(SCREEN_HEIGHT);
  float radius = 0;
  float radiusSquared = sq(radius);
  float circleness;
  byte value;
  for (int dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot] = CHSV(hue, 100, 0);
  }
  while ((not newCommandFlag) and (radius < SCREEN_WIDTH + 9)) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        circleness = radiusSquared - (sq(centerX - x) + sq(centerY - y));
        if ((0 < circleness) && (circleness < 44)) {
          if ((255 - chsvs[y * SCREEN_WIDTH + x].value) > 45) {
            chsvs[y * SCREEN_WIDTH + x].value += 45 * (circleness / 44);
          }
          else {
            chsvs[y * SCREEN_WIDTH + x].value = 255;
          }
        }
        else {
          chsvs[y * SCREEN_WIDTH + x].value *= 0.8;
        }
      }
    }
    hsv_show();
    radius += 0.15;
    radiusSquared = sq(radius);
    listenForCommand();
    delay(16); //60 fps
  }
}
void drops() {
  while (not newCommandFlag) {
    drop();
    delay(random8());
    listenForCommand();
  }
}
void rainbowdrops() {
  while (not newCommandFlag) {
    drop(random8());
    listenForCommand();
  }
}

void spiral() {
  int x;
  int y;
  float magnitude;
  float piOverFour = 3.14159265 / 4;
  float rotation = 0;
  for (int dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot].saturation = 200;
    chsvs[dot].value = 0;
  }
  float magnitudes[160];
  int i = 0;
  for (float n = 1; n < 17; n += 0.1) {
    magnitudes[i] = pow(1.5, n/2);
    i++;
  }
  float n;
  int progress = 0;
  while (not newCommandFlag) {
    for (n = 0; n < 16; n += 0.1) {
      i = (i + 1) % 160;
      magnitude = magnitudes[i];
      x = magnitude * cos(n - rotation) + (SCREEN_WIDTH / 2.0);
      y = magnitude * sin(n - rotation) + (SCREEN_HEIGHT / 2.0);
      if ((0 <= x) && (x < SCREEN_WIDTH) && (0 <= y) && (y < SCREEN_HEIGHT)) {
        if (chsvs[y * SCREEN_WIDTH + x].value < 230) {
          chsvs[y * SCREEN_WIDTH + x].value += 25;
        }
        else {
          chsvs[y * SCREEN_WIDTH + x].value = 255;
        }
        chsvs[y * SCREEN_WIDTH + x].hue = progress / 8 - n * 8;
      }
    }
    hsv_show();
    hsv_linfade(8);
    rotation += 0.05;
    if (rotation > (TAU)) {
      rotation -= TAU;
    }
    progress++;
    delay(1);
    listenForCommand();
  }
  //hsv_clear();
}

void test_font() {
  for (char i = 32; i < 96; i++) {
    blitChr(i, 0, 0, CHSV(73, 120, 150));
    Serial.println(i);
    hsv_show();
    hsv_clear();
    delay(1000);
  }
}

void text(char str[]) {
  hsv_clear();
  int split = 0; // position of the last space character in the first six positions
  int len = strlen(str);
  if (len <= 3) {
    blitStr(str, (SCREEN_WIDTH / 2 - 2 * len + 1), 0, TEXT_COLOR); // single line banner
    hsv_show();
  }
  else {
    marquee(str);
  }
}
void marquee(char str[]) {
  hsv_clear();
  const int y = 0;
  char copied_str[MAX_CMD_LEN];
  strncpy(copied_str, str, MAX_CMD_LEN); //overflow unsafe
  int last_x = - strlen(copied_str) * 4;
  for (int x = SCREEN_WIDTH; x > last_x; x--) {
    hsv_clear();
    blitStr(copied_str, x, y, TEXT_COLOR);
    hsv_show();
    delay(170);
    listenForCommand();
    if (newCommandFlag) {
      break;
    }
  }
  Serial.println("all done :3");
}

void life() {
  // simulate game of life
  // uses saturation to temporarily store extra data
  // doesn't need to be efficient, since it has a low framerate
  hsv_clear();
  const byte ALIVE_VALUE = 128;
  const byte INITIAL_HUE = 70;
  const byte SATURATION = 200;
  int offsets[] = {-1, 1, -SCREEN_WIDTH, SCREEN_WIDTH};
  byte i; // for iterating over offsets
  int dot; int neighbor;
  int dx; int dy;
  for (dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot] = CHSV(INITIAL_HUE, SATURATION, 0);
    if (random8() < 70) {
      chsvs[dot].value = ALIVE_VALUE;
    }
  }
  hsv_show();
  while (not newCommandFlag) {
    hsv_show();
    for (dot = 0; dot < N_DOTS; dot++) {
      chsvs[dot].saturation = 0;
    }
    for (dot = 0; dot < N_DOTS; dot++) {
      if (chsvs[dot].value == ALIVE_VALUE) {
        for (dx = -1; dx <= 1; dx++) {
          for (dy = -1; dy <= 1; dy++) {
            if ((dx == 0) && (dy == 0)) {
              continue; // skip 0, 0
            }
            neighbor = dot + dx + dy * SCREEN_WIDTH;
            if ((neighbor >= 0) && (neighbor < N_DOTS)) {
              chsvs[neighbor].saturation++;
            }
          }
        }
      }
    }
    for (dot = 0; dot < N_DOTS; dot++) {
      //Serial.print(chsvs[dot].saturation); Serial.print(' ');
      if ((chsvs[dot].value != ALIVE_VALUE) && (chsvs[dot].saturation == 3)) {
        // dead cell comes to life. inherit average of parents' hues, plus a random positive offset
        chsvs[dot].value = ALIVE_VALUE;
        chsvs[dot].hue = 70; //hueSums[dot] / chsvs[dot].saturation + random8(3); // todo: use mean of circular quantities
      }
      else if (!((chsvs[dot].value == ALIVE_VALUE) && ((chsvs[dot].saturation == 2) || (chsvs[dot].saturation == 3)))) {
        // live cell dies. rip
        chsvs[dot].value = 0;
      }
      else {
        // surviving cell advances color
        chsvs[dot].hue += 11;
      }
      //chsvs[dot].value = random8();
      chsvs[dot].saturation = 150;
      //hueSums[dot] = 0;
    }
    delay(1000);
    listenForCommand();
  }
}


void eye() {
  hsv_clear();
  const CHSV EYELID_COLOR = CHSV(0, 0, 120);
  
  float openEyelidHeights[SCREEN_WIDTH] = {1.5, 2.1, 2.6, 3.0, 3.2, 3.3, 3.4, 3.3, 3.2, 3.0, 2.6, 2.1, 1.5};
  float openness = 1;
  float blinkProgress = -1; // -1 to 1. -1 means no blink initiated
  float blinkSpeed = 0.05;
  byte pupilHue = 220;

  float squaredDistanceFromPupil;

  float pupilX = SCREEN_WIDTH / 2.0; float pupilY = SCREEN_HEIGHT / 2.0;
  float fixationPointX = pupilX; float fixationPointY = pupilY;
  float pupilDisplacementGoalX; float pupilDisplacementGoalY;
  float saccadeProgress = 0; // what a nice variable name
  float saccadeSpeed = 0.05;
  float pupilGoalX, pupilGoalY;
  
  int col;
  int row;
  int dot;
  while (not newCommandFlag) {
    if (blinkProgress > -1) {
      // continue blink
      blinkProgress += blinkSpeed;
    }
    else if ((random8() == 0)) { 
      // initiate blink
      blinkSpeed = 0.025 + 0.3 * (random8() / 255.0);
      blinkProgress += blinkSpeed;
    }
    if (abs(blinkProgress) < blinkSpeed) {
      // eye is closed. maybe change pupil hue
      if (random8() < 74) {
        pupilHue = random8();
      }
    }
    if (blinkProgress > 1) {
      // conclude blink
      blinkProgress = -1;
    }
    
    openness = abs(blinkProgress);
    
    if (saccadeProgress > 0) {
      // continue saccade
      saccadeProgress += saccadeSpeed;
    }
    else if (random8() < 18) { 
      // attempt to initiate saccade
      pupilDisplacementGoalX = -9 + 18 * (random8() / 255.0);
      pupilDisplacementGoalY = -6 + 12 * (random8() / 255.0);
      pupilGoalX = pupilDisplacementGoalX + pupilX;
      pupilGoalY = pupilDisplacementGoalY + pupilY;
      if (
        (0 <= pupilGoalX) && (pupilGoalX < SCREEN_WIDTH)
        && (0 <= pupilGoalY) && (pupilGoalY < SCREEN_HEIGHT)
        && (abs(pupilGoalY - (SCREEN_HEIGHT / 2.0)) < (openEyelidHeights[int(pupilGoalX)] - 1.5)) // pupil radius?
      ) {
        saccadeSpeed = 0.02 + 0.2 * (random8() / 255.0);
        saccadeProgress += saccadeSpeed;
      }
    }
    if (saccadeProgress > 1) {
      // conclude saccade
      fixationPointX += pupilDisplacementGoalX;
      fixationPointY += pupilDisplacementGoalY;
      pupilDisplacementGoalX = 0;
      pupilDisplacementGoalY = 0;
      saccadeProgress = 0;
    }
    pupilX = fixationPointX + pupilDisplacementGoalX * saccadeProgress;
    pupilY = fixationPointY + pupilDisplacementGoalY * saccadeProgress;
    
    for (col = 0; col < SCREEN_WIDTH; col++) {
      row = 0;

      if (openEyelidHeights[col] == -1) {
        // not part of the eye. just clear this column
        for ( ; row < SCREEN_HEIGHT; row++) {
          chsvs[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
        }
      }
      
      // clear LEDs above eye
      for ( ; row < ((SCREEN_HEIGHT / 2) - floor(openEyelidHeights[col] * openness)); row++) {
        chsvs[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
      }

      // draw top eyelid
      chsvs[col + row++ * SCREEN_WIDTH] = EYELID_COLOR;

      if (openEyelidHeights[col] * openness >= 1) {
        // draw inside of eye
        for ( ; row < (SCREEN_HEIGHT / 2 + floor(openEyelidHeights[col] * openness)); row++) {
          if (chsvs[col + row * SCREEN_WIDTH].saturation == 0) {
            chsvs[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0); //todo: fade instead
          }
          squaredDistanceFromPupil = sq(col - pupilX) + sq(row - pupilY);
          if (squaredDistanceFromPupil < 5) { // draw pupil
            chsvs[col + row * SCREEN_WIDTH] = CHSV(pupilHue, 180, 190);
          }
          else {
            // clear sclera leds
            chsvs[col + row * SCREEN_WIDTH].value = 0; //-= min(chsvs[col + row * SCREEN_WIDTH].value, 9); //fade
          }
        }
  
        // draw bottom eyelid
        if (row != SCREEN_HEIGHT / 2) {
          chsvs[col + row++ * SCREEN_WIDTH] = EYELID_COLOR;
        }
      }

      // clear LEDs below eye
      for ( ; row < SCREEN_HEIGHT; row++) {
        chsvs[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
      }
    }
    hsv_show();
    delay(10);
    listenForCommand();
  }
}

void fire() {
  while (not newCommandFlag) {
    hsv_show();
    delay(100);
    listenForCommand();
  }
}

void lissajous() {
  hsv_clear();
  const float x_offset = SCREEN_WIDTH / 2.0;
  const float y_offset = SCREEN_HEIGHT / 2.0;
  const float A = 6.5; // magnitude
  const float B = 3;
  const float a = 2; // speed
  const float b = 4.5;
  const int TICKS = 128;
  int tick;
  float t;
  float sigma = 0;
  float sigma_speed = 0.0016;
  int x; int y;
  int dot;
  // fully saturate
  for (dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot].saturation = 255;
  }
  while (not newCommandFlag) {
    hsv_linfade(12);
    for (tick = 0; tick < TICKS; tick++) {
      t = (1.0 * tick) / TICKS;
      x = A * sin((a * t + sigma) * TAU) + x_offset;
      y = B * cos(b * t * TAU) + y_offset;
      dot = y * SCREEN_WIDTH + x;
      if ((0 < dot) && (dot < N_DOTS)) {
        chsvs[dot].hue = t * 255;
        chsvs[dot].value += min(175 - chsvs[dot].value, 30);
      }
    }
    sigma += sigma_speed;
    if (sigma >= 1) {
      sigma -= 1;
    }
    hsv_show();
    delay(10);
    listenForCommand();
  }
}

/*

// Gives the accelerometer reading rotated by a constant.
// This is useful because the microcontroller is glued into the helmet at an angle.
void correctedMotionXY(float* x, float* y) {
  float mX = CircuitPlayground.motionX();
  float mY = CircuitPlayground.motionY();
  *x = mX * COS_GRAV_ANGLE - mY * SIN_GRAV_ANGLE;
  *y = mX * SIN_GRAV_ANGLE + mY * COS_GRAV_ANGLE;
}

void rain() {
  // mostly copied from matrix
  struct {
    float x = 0;
    float y = 0;
    bool age = 0;
    bool alive = false;
  } points[35];
  int dot;
  byte i;
  const float gravity = 0.05;
  float ax; float ay;
  for (dot = 0; dot < N_LEDS; dot++) {
    chsvs[dot] = CHSV(160, 244, 0);
  }
  while (not newCommandFlag) {
    for (dot = 0; dot < N_LEDS; dot++) {
      //chsvs[dot].value = max(0, chsvs[dot].value -10);
      chsvs[dot].value *= 0.85;
      //chsvs[dot].saturation = max(0, chsvs[dot].saturation - 30);
    }
    correctedMotionXY(&ax, &ay);
    for (i = 0; i < 35; i++) {
      if (points[i].alive) {
        points[i].x += ax * gravity;
        points[i].y += ay * gravity;
        points[i].age += 1;
        if ((-1 <= points[i].y) and (points[i].y < SCREEN_HEIGHT)) {
          if ((0 <= points[i].x) and (points[i].x < SCREEN_WIDTH)) {
            dot = (int)points[i].y * SCREEN_WIDTH + (int)points[i].x;
            chsvs[dot].value = min(255, chsvs[dot].value + 100);
          }
        } else {
          points[i].alive = false;
        }
      } else if (random8() < 19) {
        points[i].x = random(-10, SCREEN_WIDTH + 10);
        points[i].y = -1;
        points[i].alive = true;
      }
    }
    hsv_show();
    listenForCommand();
    delay(17);
  }
}

void ball() {
  float x = 4; float y = 4;
  float vx = 0; float vy = 0;
  float ax = 0; float ay = 0;
  float aMag;
  byte hue = 200;
  const float gravity = 0.1;
  int dot;
  while (not newCommandFlag) {
    for (dot = 0; dot < N_LEDS; dot++) {
      chsvs[dot].value *= 0.8;
    }
    correctedMotionXY(&ax, &ay); // set ax and ay to corrected accelerometer readings
    // normalize acceleration vector
    aMag = max(sqrt(ax * ax + ay * ay) / gravity, 0.001);
    ax /= aMag; ay /= aMag;
    
    vx += ax;
    vy += ay;
    
    x += vx; y += vy;
    if (x < 0) { x *= -1; vx *= -1;}
    if (y < 0) { y *= -1; vy *= -1;}
    if (x >= SCREEN_WIDTH) { x = 2 * SCREEN_WIDTH - x - 0.001; vx *= -1;}
    if (y >= SCREEN_HEIGHT) { y = 2 * SCREEN_HEIGHT - y - 0.001; vy *= -1;}
    chsvs[(int)y * SCREEN_WIDTH + (int)x] = CHSV(hue, 244, 255);
    hue += 1;
    hsv_show();
    listenForCommand();
    delay(17);
  }
}
*/

void pipes() {
  int x = 0; int y = random8(2, SCREEN_WIDTH / 2);
  int dx = 1; int dy = 0;
  byte hue = random8();
  byte saturation = random8();
  while (not newCommandFlag) {
    hsv_linfade(2);
    chsvs[x + SCREEN_WIDTH * y] = CHSV(hue, saturation, 191);
    
    if (random8() < 28) { // maybe make a right-angle turn
      if (dx == 0) { dx = 1 - 2 * random8(0, 1); dy = 0;}
      else { dx = 0; dy = 1 - 2 * random8(0, 1);}
    }
    x += dx; y += dy;
    if ((x < 0) or (x >= SCREEN_WIDTH) or (y < 0) or (y >= SCREEN_HEIGHT)) { // we're off the screen
      switch (random8(0, 3)) {
        case 0:
          dx = 0; dy = 1;
          x = random8(0, SCREEN_WIDTH - 1); y = 0;
          break;
        case 1:
          dx = 1; dy = 0;
          x = 0; y = random8(0, SCREEN_HEIGHT - 1);
          break;
        case 2:
          dx = 0; dy = -1;
          x = random8(0, SCREEN_WIDTH - 1); y = SCREEN_HEIGHT - 1;
          break;
        case 3:
          dx = -1; dy = 0;
          x = SCREEN_WIDTH - 1; y = random8(0, SCREEN_HEIGHT - 1);
          break;
      }
      // random pipe color
      hue = random8();
      saturation = random8(40, 255);
    }
    hsv_show();
    delay(56);
    listenForCommand();
  }
}

void wave() {
  int i;
  int x;
  int y;
  int iy;
  int y_step; // either 1 or -1
  float last_y;
  float amp[2] = {2.5, 2.0};
  float ad[2] = {0, 0};
  float av[2] = {0.002, 0.01};
  float per[2] = {17, 9};
  int dot;
  
  const float x_offset = SCREEN_WIDTH / 2.0;
  const float y_offset = SCREEN_HEIGHT / 2.0;
  while (not newCommandFlag) {
    hsv_linfade(16);
    //hsv_clear();
    for (x = -1; x < SCREEN_WIDTH; x++) {
      y = SCREEN_HEIGHT / 2.0 + amp[0] * sin(TAU * (ad[0] + x/per[0])) + amp[1] * sin(TAU * (ad[1] + x/per[1]));

      if (x >= 0) {
        for (iy = 0; iy < SCREEN_HEIGHT; iy++) {
          if ((iy == y) or ((last_y < iy) and (iy < y)) or ((last_y > iy) and (iy > y))) {
            if ((0 <= iy) and (iy < SCREEN_WIDTH)) {
              //Serial.print(x); Serial.print(" "); Serial.println(y);
              dot = x + SCREEN_WIDTH * ((int) iy);
              chsvs[dot].hue = sin(TAU * (ad[0] + x/per[0]))*80-90;
              chsvs[dot].sat = 255;
              chsvs[dot].value = min(255, chsvs[dot].value + 60);
            }
            else {
              chsvs[x] = CHSV(0, 255, 200); // overflow detection?
            }
          }
        }
      }
      
      last_y = y;
    }
    for (i = 0; i < 2; i++) {
      ad[i] += av[i]; //todo: mod 1?
    }
    hsv_show();
    delay(20);
    listenForCommand();
  }
}

byte sadMouthSpr[3] = {
  // needs to be centered on the screen
  // so, figure out if SCREEN_WIDTH is odd
  0b00100000,
  0b01010000,
  0b10001000
};
byte sadEyeSpr[1] = {
  0b11111000
};

#define CRY_EYES_Y 1 // distance from eye to top
#define CRY_EYES_X 1 // distance from left
#define CRY_EYES_GAP 1 // distance between eyes
#define CRY_MOUTH_X 4
#define CRY_MOUTH_Y 4
void cry() { // broken
  struct {
    int x = 0;
    float y = 0;
    bool alive = false;
  } tears[6];
  int dot;
  byte i;
  for (dot = 0; dot < N_DOTS; dot++) {
    chsvs[dot] = CHSV(160, 244, 0);
  }
  const CHSV faceColor = CHSV(160, 100, 255);
  while (not newCommandFlag) {
    for (dot = 0; dot < N_DOTS; dot++) {
      //chsvs[dot].value = max(0, chsvs[dot].value -10);
      chsvs[dot].value *= 0.97;
      //chsvs[dot].saturation = max(0, chsvs[dot].saturation - 30);
    }
    blit(sadEyeSpr, 1, CRY_EYES_X, CRY_EYES_Y, faceColor); // screen coordinates
    blit(sadEyeSpr, 1, CRY_EYES_X + 6 + CRY_EYES_GAP, CRY_EYES_Y, faceColor); // screen coordinates
    //blit(sadMouthSpr, 3, CRY_MOUTH_X, CRY_MOUTH_Y, faceColor); // screen coordinates
    for (i = 0; i < 6; i++) {
      if (tears[i].alive) {
        tears[i].y += 0.06;
        if ((-1 <= tears[i].y) and (tears[i].y < SCREEN_HEIGHT)) {
          dot = (int)tears[i].y * SCREEN_WIDTH + (int)tears[i].x;
          chsvs[dot].saturation = 255;
          chsvs[dot].value = min(255, chsvs[dot].value + 21);
        } else {
          tears[i].alive = false;
        }
      } else if (random8() < 2) {
        tears[i].x = CRY_EYES_X + random8(0, 5) + random8(0, 2) * (6 + CRY_EYES_GAP);
        tears[i].y = CRY_EYES_Y + 1;
        tears[i].alive = true;
      }
    }
    hsv_show();
    listenForCommand();
    delay(17);
  }
}

uint32_t blushFace1[SCREEN_HEIGHT] = {
  0b01100000001101000000,
  0b10010000010011000000,
  0b10010000010011000000,
  0b01100000001101000000,
  0b00000000000001000000,
  0b00000000000001000000,
  0b00000000000001000000,
};

uint32_t blushFace2[SCREEN_HEIGHT] = {
  0b00000000000001000000,
  0b00000000000001000000,
  0b00000000000001000000,
  0b00000000000001000000,
  0b00001010100001000000,
  0b01010101010101000000,
  0b10100000001011000000,
};

void blush() {
  hsv_clear();
  blit(blushFace1, SCREEN_HEIGHT, 0, 0, CHSV(1, 61, 120));
  hsv_show();
  delay(500);
  for (int i=0; ((i < 255) and (not newCommandFlag)); i++) {
    blit(blushFace2, SCREEN_HEIGHT, 0, 0, CHSV(1, 179, i));
    delay(9);
    hsv_show();
    listenForCommand();
  }
}

uint32_t uwuFace[SCREEN_HEIGHT] = {
  0b01001000100101000000,
  0b01001000100101000000,
  0b00110000011001000000,
  0b00000000000001000000,
  0b00001010100001000000,
  0b00000101000001000000,
  0b00000000000001000000,
};
void uwu() {
  hsv_clear();
  blit(uwuFace, SCREEN_HEIGHT, 0, 0, TEXT_COLOR);
  hsv_show();
}
uint32_t owoFace[SCREEN_HEIGHT] = {
  0b01100000001101000000,
  0b10010000010011000000,
  0b10010000010011000000,
  0b01100000001101000000,
  0b00001010100001000000,
  0b00000101000001000000,
  0b00000000000001000000,
};
void owo() {
  hsv_clear();
  blit(owoFace, SCREEN_HEIGHT, 0, 0, TEXT_COLOR);
  hsv_show();
}

void trans() {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      
    }
  }
  hsv_show();
  delay(20);
  listenForCommand();
}

void scan(int8_t hue, bool cycle) {
  float t = 0;
  float tSpeed = 0.008;
  int x;
  int dot;
  for (t = 0; not newCommandFlag; t += tSpeed) {
    hsv_linfade(20);
    x = ((1 + sin(t * TAU)) / 2) * SCREEN_WIDTH;
    if (cycle) {
      hue += 1;
    }
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      dot = y * SCREEN_WIDTH + x;
      if (dot < N_DOTS) {
        chsvs[dot] = CHSV(hue, 255, 255);
      }
    }
    if (t > 1) {
      t--;
    }
    hsv_show();
    delay(20);
    listenForCommand();
  }
}
void scantrans() {
  float t = 0;
  float tSpeed = 0.008;
  int x;
  int dot;
  CHSV color;
  for (t = 0; not newCommandFlag; t += tSpeed) {
    hsv_linfade(16);
    x = ((1 + sin(t * TAU)) / 2) * SCREEN_WIDTH;
    for (int y = 1; y < 6; y++) {
      dot = y * SCREEN_WIDTH + x;
      if ((y==2) or (y==4)) {
        color = CHSV(250, 150, 255); // pink
      } else if (y==3) {
        color = CHSV(0, 0, 250); // white
      } else {
        color = CHSV(140, 220, 255); // blue;
      }
      if (dot < N_DOTS) {
        chsvs[dot] = color;
      }
    }
    if (t > 1) {
      t--;
    }
    hsv_show();
    delay(20);
    listenForCommand();
  }
}

void baton(int8_t hue, bool cycle) {
  float t = 0;
  float tSpeed = 0.010;
  float dx;
  float dy;
  float x;
  float y;
  int dot;
  int i;
  int flip;
  for (t = 0; not newCommandFlag; t += tSpeed) {
    hsv_linfade(10);
    dx = cos(t * TAU) * 0.1;
    dy = sin(t * TAU) * 0.1;
    if (cycle) {
      hue += 1;
    }
    for (flip = 1; flip >= -1; flip -= 2) {
      x = SCREEN_WIDTH / 2.0;
      y = SCREEN_HEIGHT / 2.0;
      while ((0 < x) and (x < SCREEN_WIDTH-1) and (0 < y) and (y < SCREEN_HEIGHT-1) and (i < 2000)) {
        i++;
        x += dx * flip; y += dy * flip;
        dot = ((int)y) * SCREEN_WIDTH + ((int)x);
        if ((0 <= dot) and (dot < N_DOTS)) {
          chsvs[dot] = CHSV(hue, 255, 255);
        }
        //hsv_show();
      }
    }
    if (t > 1) {
      t--;
    }
    hsv_show();
    delay(35);
    listenForCommand();
  }
}

void underpass() { // :3
  float radius;
  float theta;
  int8_t hue;
  int8_t sat;
  float x;
  float y;
  int dot;
  while (not newCommandFlag) {
    hue = random8(); sat = random8();
    for (radius = 0; (radius < ((SCREEN_WIDTH + SCREEN_HEIGHT) / 2)) and (not newCommandFlag); radius += 0.23) {
      hsv_linfade(15);
      for (theta = 0; theta < TAU; theta += 0.05) {
        x = (SCREEN_WIDTH / 2.0) + cos(theta) * radius;
        y = (SCREEN_HEIGHT / 2.0) + sin(theta) * radius;
        if ((x < 0) or (x > SCREEN_WIDTH) or (y < 0) or (y > SCREEN_HEIGHT)) {
          continue;
        }
        dot = ((int)y) * SCREEN_WIDTH + x;
        if ((0 <= dot) and (dot < N_DOTS)) {
          chsvs[dot] = CHSV(hue, sat, 255);
        }
      }
      hsv_show();
      delay(5);
      listenForCommand();
    }
  }
}
