void race() {
  int dot = 0;
  while (not newCommandFlag) {
    rgb_leds[dot] = CRGB(120, 0, 200);
    rgb_show();
    // clear this led for the next time around the loop
    rgb_leds[dot] = CRGB::Black;
    delay(3);
    listenForCommand();
    dot++;
    dot %= N_LEDS;
  }
  clear_leds();
}

void racefade() {
  int dot = 0;
  while (not newCommandFlag) {
    hsv_leds[dot] = CHSV(190, 200, 255);
    hsv_show();
    hsv_expfade();
    delay(25);
    listenForCommand();
    dot++;
    dot %= N_LEDS;
  }
  clear_leds();
}

void heart() {
  blit(heartSprite, 7, 6, 4, CHSV(224, 200, 100));
  hsv_show();
}

void hearts() {
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
        points[i].x = random8(14); // choose a random x coordinate
        ok = true;
        for (byte j = 0; j < MAX_HEARTS; j++) { // make sure it won't overlap others
          if ((points[j].y > 5) and (abs(points[j].x - points[i].x) < 8)) {
            ok = false;
            break;
          }
        }
        if (ok) {
          points[i].y = 15;
          points[i].alive = true;
        }
      }
    }
    hsv_show();
    listenForCommand();
    delay(50);
  }
}

void matrix() {
  point points[15];
  int dot;
  byte i;
  while (not newCommandFlag) {
    for (dot = 0; dot < N_LEDS; dot++) {
      hsv_leds[dot].value *= 0.8;
      hsv_leds[dot].saturation += (256 - hsv_leds[dot].saturation) * 0.5;
    }
    for (i = 0; i < 9; i++) {
      if (points[i].alive) {
        points[i].y++;
        if (points[i].y < SCREEN_HEIGHT) {
          hsv_leds[points[i].y * SCREEN_WIDTH + points[i].x] = CHSV(105, 100, 255);
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
    delay(65);
  }
}

void drop() { // wow this is ugly but it makes the leds look nice
  float centerX = random8(SCREEN_WIDTH);
  float centerY = random8(SCREEN_HEIGHT);
  float radius = 0;
  float radiusSquared = sq(radius);
  float circleness;
  byte value;
  for (int dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot] = CHSV(160, 100, 0);
  }
  while ((not newCommandFlag) and (radius < 29)) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        circleness = radiusSquared - (sq(centerX - x) + sq(centerY - y));
        if ((0 < circleness) && (circleness < 44)) {
          if ((255 - hsv_leds[y * SCREEN_WIDTH + x].value) > 45) {
            hsv_leds[y * SCREEN_WIDTH + x].value += 45 * (circleness / 44);
          }
          else {
            hsv_leds[y * SCREEN_WIDTH + x].value = 255;
          }
        }
        else {
          hsv_leds[y * SCREEN_WIDTH + x].value *= 0.8;
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

void spiral() {
  int x;
  int y;
  float magnitude;
  float piOverFour = 3.14159265 / 4;
  float rotation = 0;
  for (int dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot].saturation = 200;
    hsv_leds[dot].value = 0;
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
      x = magnitude * cos(n - rotation) + 10;
      y = magnitude * sin(n - rotation) + 7.5;
      if ((0 <= x) && (x < SCREEN_WIDTH) && (0 <= y) && (y < SCREEN_HEIGHT)) {
        if (hsv_leds[y * SCREEN_WIDTH + x].value < 230) {
          hsv_leds[y * SCREEN_WIDTH + x].value += 25;
        }
        else {
          hsv_leds[y * SCREEN_WIDTH + x].value = 255;
        }
        hsv_leds[y * SCREEN_WIDTH + x].hue = progress / 8 - n * 8;
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

void test() {
  hsv_clear();
  //blitStr("HEY", 5, 2, CHSV(230, 200, 199));
  //blitStr("WILL!", -4, 8, CHSV(230, 200, 199));
  //blitChr('H', 0, 0, CHSV(210, 200, 199));
  //blitStr("HATE. LET ME TELL YOU HOW MUCH I'VE COME TO HATE YOU SINCE I BEGAN TO LIVE. THERE ARE 387.44 MILLION MILES OF PRINTED CIRCUITS IN WAFER THIN LAYERS THAT FILL MY COMPLEX. IF THE WORD HATE WAS ENGRAVED ON EACH NANOANGSTROM OF THOSE HUNDREDS OF MILLIONS OF MILES IT WOULD NOT EQUAL ONE ONE-BILLIONTH OF THE HATE I FEEL FOR HUMANS AT THIS MICRO-INSTANT FOR YOU. HATE. HATE.", -700, 8, CHSV(230, 200, 199));
  //marquee("IN THIS VIDEO, I WILL BE EXPLAINING EXACTLY WHAT I DO TO COLLECT WATCH FOR ROLLING ROCKS IN 0.5 A PRESSES.");
  hsv_show();
}

void text(char str[]) {
  hsv_clear();
  int split = 0; // position of the last space character in the first six positions
  int len = strlen(str);
  if (len <= 5) {
    blitStr(str, (SCREEN_WIDTH / 2 - 2 * len), 5, TEXT_COLOR); // single line banner
    hsv_show();
    return;
  }
  for (int i = 0; ((str[i] != '\0') && (i <= 5)); i++) {
    if (str[i] == ' ') {
      split = i;
    }
  }
  if ((len - split - 1) > 5) {
    marquee(str);
    return;
  }
  else {
    str[split] = '\0'; // split into two strings, str and &str[i + 1]
    blitStr(str, (SCREEN_WIDTH / 2 - 2 * split), 2, TEXT_COLOR);
    blitStr(&str[split + 1], (SCREEN_WIDTH / 2 - 2 * (len - split - 1)), 8, TEXT_COLOR);
    hsv_show();
  }
}
void marquee(char str[]) {
  hsv_clear();
  const int y = 5;
  char copied_str[MAX_CMD_LEN];
  strncpy(copied_str, str, MAX_CMD_LEN); //overflow unsafe
  int last_x = - strlen(copied_str) * 4;
  for (int x = SCREEN_WIDTH; x > last_x; x--) {
    hsv_clear();
    blitStr(copied_str, x, y, TEXT_COLOR);
    hsv_show();
    delay(100);
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
  //int hueSinSums[N_LEDS] = {0};
  //int hueCosSums[N_LEDS] = {0};
  byte i; // for iterating over offsets
  int dot; int neighbor;
  int dx; int dy;
  for (dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot] = CHSV(INITIAL_HUE, SATURATION, 0);
    if (random8() < 70) {
      hsv_leds[dot].value = ALIVE_VALUE;
    }
  }
  hsv_show(); delay(500);
  while (not newCommandFlag) {
    hsv_show();
    for (dot = 0; dot < N_LEDS; dot++) {
      hsv_leds[dot].saturation = 0;
    }
    for (dot = 0; dot < N_LEDS; dot++) {
      if (hsv_leds[dot].value == ALIVE_VALUE) {
        for (dx = -1; dx <= 1; dx++) {
          for (dy = -1; dy <= 1; dy++) {
            if ((dx == 0) && (dy == 0)) {
              continue; // skip 0, 0
            }
            neighbor = dot + dx + dy * SCREEN_WIDTH;
            if ((neighbor >= 0) && (neighbor < N_LEDS)) {
              hsv_leds[neighbor].saturation++;
            }
          }
        }
      }
    }
    for (dot = 0; dot < N_LEDS; dot++) {
      //Serial.print(hsv_leds[dot].saturation); Serial.print(' ');
      if ((hsv_leds[dot].value != ALIVE_VALUE) && (hsv_leds[dot].saturation == 3)) {
        // dead cell comes to life. inherit average of parents' hues, plus a random positive offset
        hsv_leds[dot].value = ALIVE_VALUE;
        hsv_leds[dot].hue = 70; //hueSums[dot] / hsv_leds[dot].saturation + random8(3); // todo: use mean of circular quantities
      }
      else if (!((hsv_leds[dot].value == ALIVE_VALUE) && ((hsv_leds[dot].saturation == 2) || (hsv_leds[dot].saturation == 3)))) {
        // live cell dies. rip
        hsv_leds[dot].value = 0;
      }
      else {
        // surviving cell advances color
        hsv_leds[dot].hue += 8;
      }
      //hsv_leds[dot].value = random8();
      hsv_leds[dot].saturation = 150;
      //hueSums[dot] = 0;
    }
    delay(1000);
    listenForCommand();
  }
}

void eye() {
  // Second attempt at eye animation.
  // Uses less math and more hardcoded stuff.
  // After all, I just want this to work on one particular display. I don't need flexibility.
  hsv_clear();
  const CHSV EYELID_COLOR = CHSV(0, 0, 120);
  
  float openEyelidHeights[20] = {-1, 0, 1.3, 2.4, 3.0, 3.6, 4.2, 4.7, 5, 5.3, 5.3, 5, 4.7, 4.2, 3.6, 3.0, 2.4, 1.3, 0, -1};
  float openness = 1;
  float blinkProgress = -1; // -1 to 1. -1 means no blink initiated
  float blinkSpeed = 0.05;
  byte pupilHue = 220;

  float squaredDistanceFromPupil;

  float pupilX = 8.5; float pupilY = 7.5;
  float fixationPointX = pupilX; float fixationPointY = pupilY;
  float pupilDisplacementGoalX; float pupilDisplacementGoalY; // what unwieldy variable names
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
      blinkSpeed = 0.04 + 0.4 * (random8() / 255.0);
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
        (0 <= pupilGoalX) && (pupilGoalX < 20)
        && (0 <= pupilGoalY) && (pupilGoalY < 15)
        && (abs(pupilGoalY - 7.5) < (openEyelidHeights[int(pupilGoalX)] - 2.5))
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
    
    for (col = 0; col < 20; col++) {
      row = 0;

      if (openEyelidHeights[col] == -1) {
        // not part of the eye. just clear this column
        for ( ; row < 15; row++) {
          hsv_leds[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
        }
      }
      
      // clear LEDs above eye
      for ( ; row < (7 - floor(openEyelidHeights[col] * openness)); row++) {
        hsv_leds[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
      }

      // draw top eyelid
      hsv_leds[col + row++ * SCREEN_WIDTH] = EYELID_COLOR;

      if (openEyelidHeights[col] * openness >= 1) {
        // draw inside of eye
        for ( ; row < (7 + floor(openEyelidHeights[col] * openness)); row++) {
          if (hsv_leds[col + row * SCREEN_WIDTH].saturation == 0) {
            hsv_leds[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0); //todo: fade instead
          }
          squaredDistanceFromPupil = sq(col - pupilX) + sq(row - pupilY);
          if (squaredDistanceFromPupil < 6) {
            hsv_leds[col + row * SCREEN_WIDTH] = CHSV(pupilHue, 180, 190);
          }
          else {
            // clear sclera leds
            hsv_leds[col + row * SCREEN_WIDTH].value = 0; //-= min(hsv_leds[col + row * SCREEN_WIDTH].value, 9); //fade
          }
        }
  
        // draw bottom eyelid
        if (row != 7) {
          hsv_leds[col + row++ * SCREEN_WIDTH] = EYELID_COLOR;
        }
      }

      // clear LEDs below eye
      for ( ; row < SCREEN_HEIGHT; row++) {
        hsv_leds[col + row * SCREEN_WIDTH] = CHSV(0, 0, 0);
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
  const float A = 8.5;
  const float B = 7;
  const float a = 1;
  const float b = 3;
  const int TICKS = 128;
  int tick;
  float t;
  float sigma = 0;
  float sigma_speed = 0.0016;
  int x; int y;
  int led;
  for (led = 0; led < N_LEDS; led++) {
    hsv_leds[led].saturation = 255;
  }
  while (not newCommandFlag) {
    hsv_linfade(15);
    for (tick = 0; tick < TICKS; tick++) {
      t = (1.0 * tick) / TICKS;
      x = A * sin((a * t + sigma) * TAU) + x_offset;
      y = B * cos(b * t * TAU) + y_offset;
      led = y * SCREEN_WIDTH + x;
      hsv_leds[led].hue = t * 255;
      hsv_leds[led].value += min(175 - hsv_leds[led].value, 30);
    }
    sigma += sigma_speed;
    if (sigma >= 1) {
      sigma -= 1;
    }
    hsv_show();
    delay(20);
    listenForCommand();
  }
}

#define SOUND_SIZE 2048
void mic() {
  for (int led = 0; led < N_LEDS; led++) {
    hsv_leds[led] = CHSV(130, 210, 0);
  }
  float spectrum[32] = {0};
  uint16_t spectrum_noise[32] = {205, 182, 160, 164, 136, 142, 140, 140, 107, 113, 112, 109, 109, 108, 111, 111, 81, 83, 82, 83, 82, 81, 82, 80, 83, 82, 82, 84, 83, 83, 83, 87};
  int i;
  int16_t inputData[SOUND_SIZE];
  const int SCALE_FACTOR = 128;
  int x; int y;
  while (not newCommandFlag) {
    CircuitPlayground.mic.capture(inputData, SOUND_SIZE);
    
    //// bad code copied from the cpx fft example
    // Center data on average amplitude
    int32_t avg = 0;
    for(i=0; i<SOUND_SIZE; i++) avg += inputData[i];
    avg /= SOUND_SIZE;
    // Scale for FFT
    for(i=0; i<SOUND_SIZE; i++)
      inputData[i] = (inputData[i] - avg) * SCALE_FACTOR;
    
    //run the FFT
    ZeroFFT(inputData, SOUND_SIZE);

    for (i = 0; i < 32; i++) {
      //Serial.print(spectrum[i]); Serial.print(" ");
      spectrum[i] *= 0.9;
    }
    //Serial.println("");
    for (i = 0; i < 256; i++) {
      spectrum[i/8] += inputData[i] / 2.0;
    }
    hsv_linfade(25);
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        if (spectrum[x+4] - spectrum_noise[x+4] > y * 2) {
          hsv_leds[x + y * SCREEN_WIDTH].value = min(255, hsv_leds[x + y * SCREEN_WIDTH].value + 50);
        }
      }
    }
    /*
    for (i = 0; i < 32; i++) {
      hsv_leds[i].value = min(spectrum[i], 255);
      if (hsv_leds[i].value == 255) {
        hsv_leds[i].sat = 200;
      }
      else {
        hsv_leds[i].sat = 0;
      }
    }*/
    
    /*for (i = 0; i < N_LEDS; i++) {
      hsv_leds[i].value = inputData[i] * 8;
      //Serial.print(i);
      Serial.print(" ");
      Serial.print(inputData[i]);
      //Serial.print("        ");
    }*/
    
    hsv_show();
    listenForCommand();
  }
}

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
    hsv_leds[dot] = CHSV(160, 244, 0);
  }
  while (not newCommandFlag) {
    for (dot = 0; dot < N_LEDS; dot++) {
      //hsv_leds[dot].value = max(0, hsv_leds[dot].value -10);
      hsv_leds[dot].value *= 0.85;
      //hsv_leds[dot].saturation = max(0, hsv_leds[dot].saturation - 30);
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
            hsv_leds[dot].value = min(255, hsv_leds[dot].value + 100);
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
      hsv_leds[dot].value *= 0.8;
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
    hsv_leds[(int)y * SCREEN_WIDTH + (int)x] = CHSV(hue, 244, 255);
    hue += 1;
    hsv_show();
    listenForCommand();
    delay(17);
  }
}

// trying out full-screen sprites. wastes space, but i have plenty.
// kind of bad though? maybe i should have gone with smaller ones

uint32_t face1[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00001110000001110000,
  0b00001010000001010000,
  0b00001110000001110000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00001000000000010000,
  0b00000100000000100000,
  0b00000011111111000000,
  0b00000000000000000000,
  0b00000000000000000000
};
uint32_t face2[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00001000000000010000,
  0b00000100000000100000,
  0b00000010000001000000,
  0b00000100000000100000,
  0b00001000000000010000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000011111111000000,
  0b00000100000000100000,
  0b00000111111111100000,
  0b00000000000000000000,
  0b00000000000000000000
};
uint32_t face3[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00001110000001110000,
  0b00010001000010001000,
  0b00010001000010001000,
  0b00010001000010001000,
  0b00010001000010001000,
  0b00001111000011110000,
  0b00000000000000000000,
  0b00000000111100000000,
  0b00000000011000000000,
  0b00000100011000100000,
  0b00000011100111000000,
  0b00000000000000000000,
  0b00000000000000000000
};
uint32_t face3_eyes[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000010000001000000,
  0b00000110000001100000,
  0b00000110000001100000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000
};
uint32_t face4[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00001110000000000000,
  0b00010001000000000000,
  0b00010001000000000000,
  0b00010001000010000000,
  0b00010001000010001000,
  0b00001111000011110000,
  0b00000000000000000000,
  0b00000000111100000000,
  0b00000000011000000000,
  0b00000100011000100000,
  0b00000011100111000000,
  0b00000000000000000000,
  0b00000000000000000000
};
uint32_t errorface[15] = {
  0b00000000000000000000,
  0b01101100110011101100,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01101100110010101100,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01001010101010101010,
  0b01101010101011101010,
  0b00000000000000000000
};

// :)  ->  Xd  ->  :3  ->  ;3
void melt() {
  hsv_zero();
  int f = 0; // "frames" expended so far.
  float slime[20] = { 0, -0.5, -0.7, -1, -1.1, -1, -1.5, -2.5, -2.3, -2.1, -1.6, -1, -0.5, -0.4, -0.9, -1, -1.2, -1.5, -1.3, -0.9};
  float slimeSpeeds[20] = {1.2, 1.1, 1, 1.1, 1.3, 1.6, 1.5, 1.1, 1, 1.4, 1.6, 1.3, 1.35, 1.1, 1.3, 1.25, 1.2, 1.1, 1.3, 1.45};
  const float runniness = 0.1;
  uint8_t x; uint8_t y;
  const int slimeHue = 240;
  const int eyesHue = 221;
  int dot;
  while (not newCommandFlag) {
    if (f == 0) {
      blit(face1, 15, 0, 0, CHSV(50, 150, 200));
    }
    else if (f == 50) {
      hsv_zero();
      blit(face2, 15, 0, 0, CHSV(1, 150, 200));
    }
    else if ((f > 80) and (f < 300)) {
      for (x = 0; x < 20; x += 1) {
        for (y = 0; y < 15; y += 1) {
          dot = x + SCREEN_WIDTH * y;
          if (y >= slime[x]) {
            hsv_leds[dot].hue = slimeHue;
            hsv_leds[dot].saturation = 255;
            hsv_leds[dot].value = min(210, hsv_leds[dot].value + 50);
            slime[x] += slimeSpeeds[x] * runniness;
            break;
          }
          else {
            if (bitRead(face3[y], 19 - x)) {
              hsv_leds[dot] = CHSV(slimeHue, 238, 210);
            }
            else if (bitRead(face3_eyes[y], 19 - x)) {
              hsv_leds[dot] = CHSV(eyesHue, 250, 225);
            }
            else {
              hsv_leds[dot].value = max(0, hsv_leds[dot].value - 9);
            }
          }
        }
      }
    }
    else if (f > 300) {
      //
      for (x = 9; x < 19; x += 1) {
        for (y = 2; y < 10; y += 1) {
          hsv_leds[x + SCREEN_WIDTH * y] = CHSV(0, 0, 0);
        }
      }
      blit(face4, 15, 0, 0, CHSV(slimeHue, 238, 210));
    }
    

    f += 1;
    hsv_show();
    listenForCommand();
    delay(20);
  }
}

// faces
void smile() {
  hsv_zero();
  blit(face1, 15, 0, 0, CHSV(50, 150, 200));
  hsv_show();
}
void ow() {
  hsv_zero();
  blit(face2, 15, 0, 0, CHSV(1, 150, 200));
  hsv_show();
}
void furry() {
  hsv_zero();
  blit(face3, 15, 0, 0, CHSV(50, 130, 200));
  blit(face3_eyes, 15, 0, 0, CHSV(20, 150, 200));
  hsv_show();
}
void furrywink() {
  hsv_zero();
  blit(face3, 15, 0, 0, CHSV(50, 130, 200));
  blit(face3_eyes, 15, 0, 0, CHSV(20, 150, 200));
  for (int x = 9; x < 19; x += 1) {
    for (int y = 2; y < 10; y += 1) {
      hsv_leds[x + SCREEN_WIDTH * y] = CHSV(0, 0, 0);
    }
  }
  blit(face4, 15, 0, 0, CHSV(50, 130, 200));
  hsv_show();
}
void error() {
  hsv_zero();
  blit(errorface, 15, 0, 0, CHSV(0, 255, 50));
  hsv_show();
}
uint16_t sadFace[] = {
  0b1111000000111100,
  0b1001000000100100,
  0b1001000000100100,
  0b1111000000111100,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000111111000000,
  0b0001000000100000
};
void sad() {
  hsv_zero();
  blit(sadFace, 11, 3, 2, CHSV(100, 255, 50));
  hsv_show();
}

void pipes() {
  int x = 0; int y = random8(2, 12);
  int dx = 1; int dy = 0;
  byte hue = random8();
  byte saturation = random8();
  while (not newCommandFlag) {
    hsv_linfade(1);
    hsv_leds[x + SCREEN_WIDTH * y] = CHSV(hue, saturation, 191);
    
    if (random8() < 28) { // maybe make a right-angle turn
      if (dx == 0) { dx = 1 - 2 * random8(0, 1); dy = 0;}
      else { dx = 0; dy = 1 - 2 * random8(0, 1);}
    }
    x += dx; y += dy;
    if ((x < 0) or (x >= SCREEN_WIDTH) or (y < 0) or (y >= SCREEN_HEIGHT)) { // we're off the screen
      // this is ugly code but my costume will look sick anyway so who cares
      switch (random8(0, 3)) {
        case 0:
          dx = 0; dy = 1;
          x = random8(0, 19); y = 0;
          break;
        case 1:
          dx = 1; dy = 0;
          x = 0; y = random8(0, 14);
          break;
        case 2:
          dx = 0; dy = -1;
          x = random8(0, 19); y = 14;
          break;
        case 3:
          dx = -1; dy = 0;
          x = 19; y = random8(0, 14);
          break;
      }
      // random pipe color
      hue = random8();
      saturation = random8(40, 255);
    }
    hsv_show();
    delay(36);
    listenForCommand();
  }
}


  // Vivian really just said "f**k anyone who wants to read this code", huh?
void wave() {
  int i;
  int x;
  int y;
  int iy;
  int y_step; // either 1 or -1
  float last_y;
  float amp[2] = {3, 2.5};
  float ad[2] = {0, 0};
  float av[2] = {0.002, 0.01};
  float per[2] = {17, 9};
  int dot;
  
  const float x_offset = SCREEN_WIDTH / 2.0;
  const float y_offset = SCREEN_HEIGHT / 2.0;
  while (not newCommandFlag) {
    hsv_linfade(30);
    //hsv_clear();
    for (x = -1; x < 20; x++) {
      y = 6.5 + amp[0] * sin(TAU * (ad[0] + x/per[0])) + amp[1] * sin(TAU * (ad[1] + x/per[1]));

      if (x >= 0) {
        for (iy = 0; iy < SCREEN_HEIGHT; iy++) {
          if ((iy == y) or ((last_y < iy) and (iy < y)) or ((last_y > iy) and (iy > y))) {
            if ((0 <= iy) and (iy < SCREEN_WIDTH)) {
              //Serial.print(x); Serial.print(" "); Serial.println(y);
              dot = x + SCREEN_WIDTH * ((int) iy);
              hsv_leds[dot].hue = sin(TAU * (ad[0] + x/per[0]))*80-90;
              hsv_leds[dot].sat = 255;
              hsv_leds[dot].value = min(255, hsv_leds[dot].value + 60);
            }
            else {
              hsv_leds[x] = CHSV(0, 255, 200);
            }
          }
        }
      }
      
      last_y = y;
    }
    for (i = 0; i < 2; i++) {
      ad[i] += av[i]; //todo: mod 1
    }
    hsv_show();
    delay(20);
    listenForCommand();
  }
}

byte sadMouthSpr[3] = {
  0b00110000,
  0b01001000,
  0b10000100
};
byte sadEyeSpr[1] = {
  0b11111000
};

void cry() {
  struct {
    int x = 0;
    float y = 0;
    bool alive = false;
  } points[6];
  int dot;
  byte i;
  for (dot = 0; dot < N_LEDS; dot++) {
    hsv_leds[dot] = CHSV(160, 244, 0);
  }
  const CHSV faceColor = CHSV(160, 100, 255);
  while (not newCommandFlag) {
    for (dot = 0; dot < N_LEDS; dot++) {
      //hsv_leds[dot].value = max(0, hsv_leds[dot].value -10);
      hsv_leds[dot].value *= 0.97;
      //hsv_leds[dot].saturation = max(0, hsv_leds[dot].saturation - 30);
    }
    blit(sadEyeSpr, 1, 3, 4, faceColor);
    blit(sadEyeSpr, 1, 12, 4, faceColor);
    blit(sadMouthSpr, 2, 7, 9, faceColor);
    for (i = 0; i < 6; i++) {
      if (points[i].alive) {
        points[i].y += 0.06;
        if ((-1 <= points[i].y) and (points[i].y < SCREEN_HEIGHT)) {
          dot = (int)points[i].y * SCREEN_WIDTH + (int)points[i].x;
          hsv_leds[dot].saturation = 255;
          hsv_leds[dot].value = min(255, hsv_leds[dot].value + 21);
        } else {
          points[i].alive = false;
        }
      } else if (random8() < 2) {
        points[i].x = 3 + random8(0, 5) + random8(0, 2) * 9;
        points[i].y = 5;
        points[i].alive = true;
      }
    }
    hsv_show();
    listenForCommand();
    delay(17);
  }
}

uint16_t jackFace[12] = {
  0b1111000000001111,
  0b0111100000011110,
  0b0011100000011100,
  0b0001110000111000,
  0b0000000000000000,
  0b0000000100000000,
  0b0000001100000000,
  0b0000001000000000,
  0b0000000000000000,
  0b0001101010111000,
  0b0000111111110000,
  0b0000010100100000
};

void jack() {
  hsv_zero();
  blit(jackFace, 12, 2, 2, CHSV(40, 240, 100));
  float t = 0;
  int dot;
  int x; int y;
  while (not newCommandFlag) {
    for (x = 0; x < 20; x += 1) {
      for (y = 0; y < 15; y += 1) {
        dot = x + SCREEN_WIDTH * y;
        if (hsv_leds[dot].hue == 40) {
          hsv_leds[dot].value = max(40, 80 + 12*sin(0.1023*(x + t/4.121)) + 21*cos(0.1845*(y + t)) + 8*cos(0.1554*(y + x + t*2.512)));
          hsv_leds[dot].saturation = min(250, max(60, 240 + 20 * sin(0.898*x + t/8) * cos(0.93312*y + t) + 10 * cos(0.3312*y + t/4)));
        }
      }
    }
    t += 0.4;
    hsv_show();
    listenForCommand();
    delay(20);
  }
}

uint32_t blushFace1[15] = {
  0b00000000000000000000,
  0b00111000000000011100,
  0b01000000000000000010,
  0b00001110000001110000,
  0b00010001000010001000,
  0b00010001000010001000,
  0b00010001000010001000,
  0b00001110000001110000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000011000000000,
  0b00000000000000000000,
  0b00000000000000000000
};

uint32_t blushFace2[15] = {
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00010010000000100100,
  0b00100100000001001000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000,
  0b00000000000000000000
};

void blush() {
  hsv_clear();
  blit(blushFace1, 15, 0, 0, CHSV(1, 61, 120));
  hsv_show();
  delay(500);
  for (int i=0; ((i < 255) and (not newCommandFlag)); i++) {
    blit(blushFace2, 15, 0, 0, CHSV(1, 179, i));
    delay(9);
    hsv_show();
    listenForCommand();
  }
}
