#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <arduinoFFT.h>

#define MIC_IN 34 // Use A0 for mic input
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 255
#define DATA_PIN 2
#define TOTAL_STRIPS 63
#define NUM_LEDS 882
#define BRIHTNESS 255
#define SAMPLES 1024
#define SAMPLIN_FREQ 40000
#define AMPLITUDE                                                              \
  10000 // Depending on your audio source level, you may need to alter
        //   this
        // value. Can be used as a 'sensitivity' control.
#define COLOR_ORDER GRB // If colours look wrong, play with this
#define CHIPSET WS2812B // LED strip type
#define MAX_MILLIAMPS                                                          \
  2000 // Careful with the amount of power here if running off USB port
#define LED_VOLTS 5 // Usually 5 or 12
#define NUM_BANDS                                                              \
  16 // To change this, you will need to change the bunch of if statements
// describing the mapping from bins to bands
#define NOISE 500 // Used as a crude noise filter, values below this are
// ignored
#define BAR_WIDTH                                                              \
  (TOTAL_STRIPS / (NUM_BANDS - 1)) // If width >= 8 light 1 LED width per
// bar,
// >= 16 light 2 LEDs width bar etc
//   true // Set to false if you're LEDS are connected end to end, true if
// serpentine

// Adjust these values to get the visualizer to show the way you want it to
constexpr int g_brightness{255};
//

// Shouldn't need to adjust the values below here
constexpr int g_data_pin{2};   // Pin that communicates with the leds.
constexpr int g_samples{1024}; // Amount of samples to take for arduinoFFT
                               // to
// handle, must be power of 2.
constexpr int g_totalStrips{63}; // Total amount of strips in use.
constexpr int g_num_leds{882};   // Combined total of leds.
constexpr int g_longStrips{9};   // Amount of long strips.
constexpr int g_shortStrips{54}; // Amount of short strips.
constexpr int g_sampling_freq{8000};
constexpr int g_amplitude{10000};
constexpr int g_noise{500};
constexpr int g_num_bands{16};
constexpr int g_bar_width{4};
// constexpr int g_bar_width{g_totalStrips / (g_num_bands - 1)};

//
unsigned int sampling_period_us;
byte peak[] = {0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0}; // The length of these arrays must be
                                        // >= NUM_BANDS
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long newTime;

double vReal[g_samples];
double vImag[g_samples];

// int color{0};
uint8_t colorTimer{0};
CRGB leds[g_num_leds]; // Create LED Object
// Creating the arduinoFFT object and passing what variables it should use.
arduinoFFT FFT = arduinoFFT(vReal, vImag, g_samples, g_sampling_freq);

struct strips {
  int length;
  int stripleds[13];
};

// Creates a struct for the long strips
struct stripLong {
  int length;
  int stripleds[42];
};

// Defines the amount of normal max 13 leds strips (doesn't have to be a full 13
// leds strip, just can't be longer than 13 leds). The [54] here means you will
// have 54 shorter strips, adjust to the amount you need
strips Strips[g_shortStrips];

// Defines the amount of normal max 41 leds strips (doesn't have to be a full 41
// leds strip), the [9] here means you will have 9 of those long strips, change
// that number to the amount you need Don't use it for the shorter strips as it
// would waste quite a bit of memory (would only need 13 out of the 41 slots)
stripLong StripsLong[g_longStrips];

// Returns the physical led number of the strip and led number supplied.
int getLedNumber(int stripNumber, int ledNumber) {
  int ledPhysNumber{0};
  if (stripNumber < 27) {
    ledPhysNumber = Strips[stripNumber].stripleds[ledNumber];
  } else if (stripNumber > 26 && stripNumber < 36) {
    ledPhysNumber = StripsLong[(stripNumber - 27)].stripleds[ledNumber];
  } else if (stripNumber > 35) {
    ledPhysNumber = Strips[(stripNumber - 9)].stripleds[ledNumber];
  }
  return ledPhysNumber;
}

// Returns the strip length of the strip supplied.
int getStripLength(int stripNumber) {
  int length{0};
  if (stripNumber < 27) {
    length = Strips[stripNumber].length;
  } else if (stripNumber > 26 && stripNumber < 36) {
    length = StripsLong[(stripNumber - 27)].length;
  } else if (stripNumber > 35) {
    length = Strips[(stripNumber - 9)].length;
  }
  return length;
}

// Sets the physical led number for the strip and led number supplied.
void setLedNumber(int stripNumber, int ledNumber, int ledPhysNumber) {
  if (stripNumber < 27) {
    Strips[stripNumber].stripleds[ledNumber] = ledPhysNumber;
  } else if (stripNumber > 26 && stripNumber < 36) {
    StripsLong[(stripNumber - 27)].stripleds[ledNumber] = ledPhysNumber;
  } else if (stripNumber > 35) {
    Strips[(stripNumber - 9)].stripleds[ledNumber] = ledPhysNumber;
  }
}

// Function to handle the wiring of the strips so all leds are lit up in the
// same orientation.
bool checkReverseLed(bool reverseLeds, int stripNumber) {
  if (stripNumber == 36) {
  }
  if (stripNumber > 12 && stripNumber < 27) {
    if (stripNumber % 2 != 0) {
      reverseLeds = !reverseLeds;
    }
  } else if (stripNumber > 35 && stripNumber < 50) {
    if (stripNumber % 2 == 0) {
      reverseLeds = !reverseLeds;
    }
  } else if (stripNumber > 26 && stripNumber < 36) {
    if (stripNumber % 2 == 0) {
      reverseLeds = true;
    } else {
      reverseLeds = false;
    }
  } else if (stripNumber % 2 != 0) {
    reverseLeds = true;
  } else {
    reverseLeds = false;
  }
  return reverseLeds;
}

// Gives each individual led in the strips their own physical number (the
// number
// between 0 - 886)
void initializeStrips() {
  static int ledPhysNumber{0};
  bool reverseLeds{};
  for (int k = 0; k < g_totalStrips; k += 1) {
    reverseLeds = checkReverseLed(reverseLeds, k);
    for (int i = 0; i < getStripLength(k); i++) {
      if (reverseLeds) {
        setLedNumber(k, (getStripLength(k) - (i + 1)), ledPhysNumber);
      } else {
        setLedNumber(k, i, ledPhysNumber);
      }
      ledPhysNumber++;
    }
  }
}

void tempStripLength() { Strips[0].length = 4; }

// Gives each led strip the length or amount of leds that strip has
void setStripLength() {
  Strips[0].length = 13;
  Strips[1].length = 13;
  Strips[2].length = 13;
  Strips[3].length = 13;
  Strips[4].length = 13;
  Strips[5].length = 13;
  Strips[6].length = 13;
  Strips[7].length = 11;
  Strips[8].length = 9;
  Strips[9].length = 8;
  Strips[10].length = 7;
  Strips[11].length = 7;
  Strips[12].length = 7;
  Strips[13].length = 7;
  Strips[14].length = 6;
  Strips[15].length = 6;
  Strips[16].length = 7;
  Strips[17].length = 7;
  Strips[18].length = 6;
  Strips[19].length = 7;
  Strips[20].length = 7;
  Strips[21].length = 8;
  Strips[22].length = 8;
  Strips[23].length = 10;
  Strips[24].length = 9;
  Strips[25].length = 11;
  Strips[26].length = 13;
  StripsLong[0].length = 42;
  StripsLong[1].length = 42;
  StripsLong[2].length = 42;
  StripsLong[3].length = 42;
  StripsLong[4].length = 42;
  StripsLong[5].length = 42;
  StripsLong[6].length = 42;
  StripsLong[7].length = 42;
  StripsLong[8].length = 42;
  Strips[27].length = 11;
  Strips[28].length = 13;
  Strips[29].length = 10;
  Strips[30].length = 9;
  Strips[31].length = 8;
  Strips[32].length = 8;
  Strips[33].length = 7;
  Strips[34].length = 7;
  Strips[35].length = 7;
  Strips[36].length = 6;
  Strips[37].length = 6;
  Strips[38].length = 7;
  Strips[39].length = 7;
  Strips[40].length = 6;
  Strips[41].length = 7;
  Strips[42].length = 7;
  Strips[43].length = 7;
  Strips[44].length = 8;
  Strips[45].length = 9;
  Strips[46].length = 11;
  Strips[47].length = 13;
  Strips[48].length = 13;
  Strips[49].length = 13;
  Strips[50].length = 13;
  Strips[51].length = 13;
  Strips[52].length = 13;
  Strips[53].length = 13;
}

// Cleans up the leds, sets them all to off/black.
void showProgramCleanup(long delayTime) {
  for (int i = 0; i < g_num_leds; ++i) {
    leds[i] = CRGB::Black;
  }
  delay(delayTime);
  FastLED.show();
}

void rainbowBars(int band, int barHeight) {
  int xStart = g_bar_width * band;
  for (int i = xStart; i < xStart + g_bar_width || i < 63; i++) {
    for (int j = getStripLength(i); j >= getStripLength(i) - barHeight; j--) {
      leds[getLedNumber(i, j)] =
          CHSV((i / g_bar_width) * (255 / g_num_bands), 255, 155);
    }
  }
}

void displayUpdate(int band, int barHeight) {
  int xStart = g_bar_width * band;
  int color = 0;
  for (int i = xStart; i < xStart + g_bar_width; i++) {
    if (i == 63) {
      return;
    }
    for (int j = 0; j < getStripLength(i); j++) {
      int ledNumber = getLedNumber(i, j);
      if (j > 0) {
        color = 255 / j;
      }
      if (j <= barHeight) {
        leds[ledNumber] = CHSV(color, 255, 150);
      } else {
        leds[ledNumber] = CHSV(color, 255, 0);
      }
    }
    FastLED.show();
    delayMicroseconds(75);
  }
}

void blink() {
  int ledColor = 0;
  for (int i = 0; i < g_num_leds; i++) {
    if (i > 0) {
      ledColor = 255 / i;
    }
    leds[i] = CHSV(ledColor, 255, 150);
    // ledColor++;
    // if (ledColor == 255) {
    //   ledColor = 0;
    // }
    // leds[i] = CRGB::Red;
  }
  FastLED.show();
  delayMicroseconds(75);
  // delay(500);
  for (int i = 0; i < g_num_leds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  delayMicroseconds(75);
  // delay(500);
}

void samples() {
  // Reset bandValues[]
  for (int i = 0; i < g_num_bands; i++) {
    bandValues[i] = 0;
  }

  Serial.println("Starting getting samples");
  // Sample the audio pin
  for (int i = 0; i < g_samples; i++) {
    newTime = micros();
    vReal[i] = analogRead(MIC_IN); // A conversion takes about 9.7uS on an
    // ESP32
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* chill */
    }
  }

  Serial.println("Going to FFT");
  // Compute FFT
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  // Analyse FFT results
  for (int i = 2; i < (g_samples / 2);
       i++) { // Don't use sample 0 and only first SAMPLES/2 are usable. Each
              // array element represents a frequency bin and its value the
              // amplitude.
    // Serial.print("FFT Data: ");
    // Serial.println(vReal[i]);
    if (vReal[i] > g_noise) { // Add a crude noise filter

      if (g_num_bands == 8) {
        // 8 bands, 12kHz top band
        if (i <= 3)
          bandValues[0] += (int)vReal[i];
        if (i > 3 && i <= 6)
          bandValues[1] += (int)vReal[i];
        if (i > 6 && i <= 13)
          bandValues[2] += (int)vReal[i];
        if (i > 13 && i <= 27)
          bandValues[3] += (int)vReal[i];
        if (i > 27 && i <= 55)
          bandValues[4] += (int)vReal[i];
        if (i > 55 && i <= 112)
          bandValues[5] += (int)vReal[i];
        if (i > 112 && i <= 229)
          bandValues[6] += (int)vReal[i];
        if (i > 229)
          bandValues[7] += (int)vReal[i]; //
      } else if (g_num_bands == 16) {
        // 16 bands, 12kHz top band
        // if (i <= 1)
        //   bandValues[0] += (int)vReal[i];
        // if (i > 0 && i <= 1)
        //   bandValues[1] += (int)vReal[i];
        // if (i > 1 && i <= 2)
        //   bandValues[2] += (int)vReal[i];
        // if (i > 2 && i <= 3)
        //   bandValues[3] += (int)vReal[i];
        // if (i > 3 && i <= 4)
        //   bandValues[4] += (int)vReal[i];
        // if (i > 4 && i <= 5)
        //   bandValues[5] += (int)vReal[i];
        // if (i > 5 && i <= 8)
        //   bandValues[6] += (int)vReal[i];
        // if (i > 8 && i <= 11)
        //   bandValues[7] += (int)vReal[i];
        // if (i > 11 && i <= 15)
        //   bandValues[8] += (int)vReal[i];
        // if (i > 15 && i <= 21)
        //   bandValues[9] += (int)vReal[i];
        // if (i > 21 && i <= 29)
        //   bandValues[10] += (int)vReal[i];
        // if (i > 29 && i <= 40)
        //   bandValues[11] += (int)vReal[i];
        // if (i > 40 && i <= 56)
        //   bandValues[12] += (int)vReal[i];
        // if (i > 56 && i <= 79)
        //   bandValues[13] += (int)vReal[i];
        // if (i > 79 && i <= 110)
        //   bandValues[14] += (int)vReal[i];
        // if (i > 110)
        //   bandValues[15] += (int)vReal[i];
        if (i <= 2)
          bandValues[0] += (int)vReal[i];
        if (i > 2 && i <= 3)
          bandValues[1] += (int)vReal[i];
        if (i > 3 && i <= 5)
          bandValues[2] += (int)vReal[i];
        if (i > 5 && i <= 7)
          bandValues[3] += (int)vReal[i];
        if (i > 7 && i <= 9)
          bandValues[4] += (int)vReal[i];
        if (i > 9 && i <= 13)
          bandValues[5] += (int)vReal[i];
        if (i > 13 && i <= 18)
          bandValues[6] += (int)vReal[i];
        if (i > 18 && i <= 25)
          bandValues[7] += (int)vReal[i];
        if (i > 25 && i <= 36)
          bandValues[8] += (int)vReal[i];
        if (i > 36 && i <= 50)
          bandValues[9] += (int)vReal[i];
        if (i > 50 && i <= 69)
          bandValues[10] += (int)vReal[i];
        if (i > 69 && i <= 97)
          bandValues[11] += (int)vReal[i];
        if (i > 97 && i <= 135)
          bandValues[12] += (int)vReal[i];
        if (i > 135 && i <= 189)
          bandValues[13] += (int)vReal[i];
        if (i > 189 && i <= 264)
          bandValues[14] += (int)vReal[i];
        if (i > 264)
          bandValues[15] += (int)vReal[i];
      }
    }
  }

  // Process the FFT data into bar heights
  for (int band = 0; band < g_num_bands; band++) {
    // Scale the bars for the display
    int barHeight = bandValues[band] / g_amplitude;
    if (barHeight > getStripLength(band)) {
      barHeight = (getStripLength(band));
    }

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    // if (barHeight > peak[band]) {
    //   peak[band] = min(getStripLength(band), barHeight);
    // }

    displayUpdate(band, barHeight);
    // blink();

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
  }

  // Decay peak
  // EVERY_N_MILLISECONDS(60) {
  //   for (byte band = 0; band < g_num_bands; band++)
  //     if (peak[band] > 0)
  //       peak[band] -= 1;
  //   colorTimer++;
  // }

  // Used in some of the patterns
  // EVERY_N_MILLISECONDS(10) { colorTimer++; }

  // FastLED.show();
}

void fillDisplay() {
  int height[16] = {0, 1, 3, 6, 4, 3, 8, 7, 1, 10, 8, 2, 1, 5, 9, 0};
  for (int i = 0; i < 16; i++) {
    displayUpdate(i, height[i]);
  }
  FastLED.show();
}

// Captures audio frequencies with the mic and calculates them with the
// ArduinoFFT library.

void visualizer() {
  // Collect Samples
  // getSamples();
  samples();
  // blink();
  // Update Display
  // displayUpdate();

  // FastLED.show();
}

// Function that is run on initial boot.
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  // delay(1000); // delay 1 seconds on startup
  // tempStripLength();
  setStripLength();
  initializeStrips();
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip); // initializes LED strip
  FastLED.setBrightness(BRIGHTNESS);   // global brightness
  // showProgramCleanup(3000);
  sampling_period_us = round(1000000 * (1.0 / g_sampling_freq));
  // visualizer();
}

// The program loop, this function will constantly keep on running.
void loop() {
  // showProgramCleanup(3000);
  // showProgramOrange(2, 1000);
  // showProgramCleanup(3000);
  // showProgram2by2Checkerboard(CRGB::Orange,CRGB::White,1000000);
  // blink();
  // fillDisplay();
  visualizer();
}
