#include <FastLED.h>
#include <arduinoFFT.h>

#define MIC_IN A0 // Use A0 for mic input
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// Adjust these values to get the visualizer to show the way you want it to
constexpr int g_lowestFreq{80};   // Set the lowest frequency to constrain to.
constexpr int g_highestFreq{450}; // Set the highest frequency to constrain to.
constexpr int g_colorIncr{3};     // Set how many color gradients increase each strip should have.
constexpr int g_minimumLed{0};    // Set the minimum amount of leds per strip that should light up.
constexpr int g_mapMulti{1};      // Set the width of the map, increase for more leds to light up.
constexpr int g_brightness{150};  // Set the brightness of the leds.
//

// Shouldn't need to adjust the values below here
constexpr int g_data_pin{3};      // Pin that communicates with the leds.
constexpr int g_samples{64};      // Amount of samples to take for arduinoFFT to handle, must be power of 2.
constexpr int g_totalStrips{63};  // Total amount of strips in use.
constexpr int g_num_leds{882};    // Combined total of leds.
constexpr int g_longStrips{9};    // Amount of long strips.
constexpr int g_shortStrips{54};  // Amount of short strips.
//

double vReal[g_samples];
double vImag[g_samples];

int Intensity[g_samples] = {};    // initialize Frequency Intensity to zero
int Displacement = 1;

CRGB leds[g_num_leds];            // Create LED Object
arduinoFFT FFT = arduinoFFT(vReal, vImag, g_samples,
                            8000); // Creating the arduinoFFT object and passing
                                   // what variables it should use.

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

// Gives each individual led in the strips their own physical number (the number
// between 0 - 886)
void initializeStrips() {
  static int ledPhysNumber{0};
  bool reverseLeds{};
  for (int k = 0; k < 63; k += 1) {
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

// Sorting function, so that the array is sorted in ascending numbers.
int sort_asc(const void *cmp1, const void *cmp2) {
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  return a < b ? -1 : (a > b ? 1 : 0);
}

// Captures audio frequencies with the mic and calculates them with the
// ArduinoFFT library.
void getSamples() {

  // Grabbing the audio samples from the mic and storing them in vReal.
  for (int i = 0; i < g_samples; i++) {
    vReal[i] = analogRead(MIC_IN);
    vImag[i] = 0;
  }
  // FFT
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HANN, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  // Going to sort the vReal array in ascending form.
  int vReal_length = sizeof(vReal) / sizeof(vReal[0]);
  qsort(vReal, vReal_length, sizeof(vReal[0]), sort_asc);

  // Constraining the values in vReal to the lowest and highest defined
  // frequencies. Mapping the values in vReal to the amount of leds on the led
  // strips, increased by the multiplier (multiplier can't be a decimal value,
  // as that could introduce unexpected behavior).
  for (int i = 0; i < (g_totalStrips * Displacement); i += Displacement) {
    //Serial.print("vReal: ");
    //Serial.println(vReal[i]);
    vReal[i] = constrain(vReal[i], g_lowestFreq, g_highestFreq);
    vReal[i] = map(vReal[i], g_lowestFreq, g_highestFreq, g_minimumLed,
                   ((getStripLength(i) - 1) * g_mapMulti));

    // Not sure if this part is really needed, could likely just use vReal in
    // the displayUpdate function.
    Intensity[(i / Displacement)]--;
    if (vReal[i] > Intensity[(i / Displacement)]) {
      Intensity[(i / Displacement)] = vReal[i];
    }
  }
}

// Tells what leds should be lit up and what color and brightness they should
// have.
void displayUpdate() {
  int color = 0;
  for (int i = 0; i < g_totalStrips; i++) {
    for (int j = 0; j < getStripLength(i); j++) {
      if (j <= Intensity[i]) {
        leds[getLedNumber(i, j)] = CHSV(color, 255, g_brightness);
      } else {
        leds[getLedNumber(i, j)] = CHSV(color, 255, 0);
      }
    }
    color += g_colorIncr;
  }
}

void visualizer() {
  // Collect Samples
  getSamples();

  // Update Display
  displayUpdate();

  FastLED.show();
}

// Function that is run on initial boot.
void setup() {
  Serial.begin(9600);
  delay(1000); // delay 1 seconds on startup
  setStripLength();
  initializeStrips();
  showProgramCleanup(3000);
  FastLED.addLeds<LED_TYPE, g_data_pin, COLOR_ORDER>(leds, g_num_leds)
      .setCorrection(TypicalLEDStrip); // initializes LED strip
  FastLED.setBrightness(g_brightness); // global brightness
}

// The program loop, this function will constantly keep on running.
void loop() {
  // showProgramCleanup(3000);
  // showProgramOrange(2, 1000);
  // showProgramCleanup(3000);
  // showProgram2by2Checkerboard(CRGB::Orange,CRGB::White,1000000);
  visualizer();
}
