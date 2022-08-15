#define FASTLED_ALLOW_INTERRUPTS 0
#include "driver/adc.h"
#include <FastLED.h>
#include <arduinoFFT.h>

// #include "ESP32_fft.h"

// Adjust these values to get the visualizer to show the way you want it to
constexpr int g_brightness{255};
constexpr int g_data_pin{2};     // Pin that communicates with the leds.
const uint32_t g_samples{1024};  // Amount of samples to take for arduinoFFT
                                 // to handle, must be power of 2.
constexpr int g_totalStrips{63}; // Total amount of strips in use.
constexpr int g_num_leds{882};   // Combined total of leds.
constexpr int g_longStrips{9};   // Amount of long strips.
constexpr int g_shortStrips{54}; // Amount of short strips.
constexpr int g_sampling_freq{40000};
constexpr int g_amplitude{2000};
//Band Filters:
constexpr int g_noise_overall{5000};
constexpr int g_noise_band1{17000};
constexpr int g_noise_band2{15000};
constexpr int g_noise_band3{30000};
constexpr int g_noise_band4{40000};
constexpr int g_noise_band5{40000};
constexpr int g_noise_band6{12000};

constexpr int g_num_bands{16};
constexpr int g_bar_width{4};
// constexpr int g_bar_width{g_totalStrips / (g_num_bands - 1)};

TaskHandle_t Task1;
TaskHandle_t Task2;
//
unsigned int sampling_period_us;
byte peak[g_num_bands] = {0}; // The length of these arrays must be
                              // >= NUM_BANDS
int oldBarHeights[g_num_bands] = {0};
int bandValues[g_num_bands] = {0};

bool updateDisplay;

auto vReal = new float[g_samples];
auto vImag = new float[g_samples];

CRGB leds[g_num_leds]; // Create LED Object
// Creating the arduinoFFT object and passing what variables it should use.
auto FFT = ArduinoFFT<float>(vReal, vImag, g_samples, g_sampling_freq);

// Creating the ESPFFT object and passing what variables it should use.
// auto FFT =
//     ESP_fft(g_samples, g_sampling_freq, FFT_REAL, FFT_FORWARD, vReal, vReal);

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
// number between 0 - 886)
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

// Sets what leds to light up and what color.
void displayUpdate(int band, int barHeight) {
  int xStart = g_bar_width * band;
  int color = 0;
  for (int i = xStart; i < xStart + g_bar_width; i++) {
    if (i == 63) {
      return;
    }
    map(barHeight, 0, 10, 0, getStripLength(i));
    if (i > 0) {
      color = (255 / g_totalStrips) * i;
    }
    for (int j = 0; j < getStripLength(i); j++) {
      int ledNumber = getLedNumber(i, j);
      if (j <= barHeight) {
        leds[ledNumber] = CHSV(color, 255, 255);
      } else {
        leds[ledNumber] = CHSV(color, 255, 0);
      }
    }
  }
}

// Test function to make all the leds blink.
void blink() {
  int ledColor = 0;
  for (int i = 0; i < g_num_leds; i++) {
    if (i > 0) {
      ledColor = 255 / i;
    }
    leds[i] = CHSV(ledColor, 255, 150);
  }
  FastLED.show();
  for (int i = 0; i < g_num_leds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

// Gets the band values and translates them to what amount of leds per strip
// should light up.
void barHeightDisplay() {
  unsigned long time{millis()};
  // Process the FFT data into bar heights
  for (int band = 0; band < g_num_bands; band++) {
    // Scale the bars for the display
    // Serial.printf("BandValue: %d, Band: %d \n", bandValues[band], band);
    int barHeight = bandValues[band] / g_amplitude;

    constrain(barHeight, 0, 10);

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
  Serial.printf("Setting leds took %lu mS \n", (millis() - time));

  // time = millis();
  // FastLED.show();
  // Serial.printf("Displaying leds took %lu mS \n", (millis() - time));
}

// Adjusting the bars to the frequency ranges.
void setBandValues() {
  unsigned long time{millis()};
  for (int i = 0; i < g_num_bands; i++) {
    bandValues[i] = 0;
  }

  // Compute FFT

  time = millis();
  // ESP32FFT Library commands:
  // FFT.removeDC();
  // FFT.hammingWindow();
  // FFT.execute();
  // FFT.complexToMagnitude();
  //

  // ArduinoFFT develop version commands:
  FFT.dcRemoval();
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();
  //

  // ArduinoFFT master version commands:
  // FFT.dcRemoval();
  // FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  // FFT.compute(FFT_FORWARD);
  // FFT.complexToMagnitude();
  //

  Serial.printf("FFT took %lu mS \n", (millis() - time));

  // Analyse FFT results
  time = millis();
  for (int i = 2; i < (g_samples / 2); i++) {
    // Don't use sample 0 and only first SAMPLES/2 are usable. Each
    // array element represents a frequency bin and its value the amplitude.

    //Serial.print("FFT Data: ");
    //Serial.println(vReal[i]);
    if (vReal[i] > g_noise_overall) { // Add a crude noise filter
      // 16 bands, 12kHz top band
      if (i > 13 && i <= 16 && vReal[i] > g_noise_band1)
        bandValues[0] += (int)vReal[i];
      if (i > 16 && i <= 20 && vReal[i] > g_noise_band2)
        bandValues[1] += (int)vReal[i];
      if (i > 20 && i <= 25 && vReal[i] > g_noise_band3)
        bandValues[2] += (int)vReal[i];
      if (i > 25 && i <= 31 && vReal[i] > g_noise_band4)
        bandValues[3] += (int)vReal[i];
      if (i > 31 && i <= 39 && vReal[i] > g_noise_band5)
        bandValues[4] += (int)vReal[i];
      if (i > 39 && i <= 48 && vReal[i] > g_noise_band6)
        bandValues[5] += (int)vReal[i];
      if (i > 48 && i <= 60)
        bandValues[6] += (int)vReal[i];
      if (i > 60 && i <= 74)
        bandValues[7] += (int)vReal[i];
      if (i > 74 && i <= 93)
        bandValues[8] += (int)vReal[i];
      if (i > 93 && i <= 115)
        bandValues[9] += (int)vReal[i]*2;
      if (i > 115 && i <= 144)
        bandValues[10] += (int)vReal[i]*2;
      if (i > 144 && i <= 179)
        bandValues[11] += (int)vReal[i]*2;
      if (i > 179 && i <= 223)
        bandValues[12] += (int)vReal[i];
      if (i > 223 && i <= 277)
        bandValues[13] += (int)vReal[i];
      if (i > 277 && i <= 345)
        bandValues[14] += (int)vReal[i];
      if (i > 345)
        bandValues[15] += (int)vReal[i];
    }
  }
  Serial.printf("Setting bands took %lu mS \n", (millis() - time));
}

// Getting the audio samples from the mic.
void getSamples() {
  // Reset bandValues[]
  unsigned long newTime;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);

  Serial.println("Starting getting samples");
  // Sample the audio pin
  unsigned long time{millis()};

  for (int i = 0; i < g_samples; i++) {
    newTime = micros();
    vReal[i] = adc1_get_raw(ADC1_CHANNEL_6);
    // vReal[i] = analogRead(MIC_IN);
    //Serial.print("Samples Data: ");
    //Serial.println(vReal[i]);
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* chill */
    }
  }
  Serial.printf("Getting samples took %lu mS \n", (millis() - time));
}

// Captures audio frequencies with the mic and calculates them with the
// ArduinoFFT library.
void visualizer(void *) {
  for (;;) {
    getSamples();
    setBandValues();
    barHeightDisplay();
    updateDisplay = true;
    delay(1);
  }
}

// Loop for addressing the leds through a different core.
void printLeds(void *) {
  unsigned long timer;
  for (;;) {
    // Serial.printf("Update Display Bool: %d \n", updateDisplay);
    // if (updateDisplay == true) {
    // timer = millis();
    FastLED.show();
    Serial.printf("Displaying leds took %lu mS \n", (millis() - timer));
    updateDisplay = false;
    delay(1);
    // }
  }
}

// Function that is run on initial boot.
void setup() {
  Serial.begin(115200);
  setStripLength();
  initializeStrips();
  FastLED.addLeds<WS2812B, g_data_pin, GRB>(leds, g_num_leds)
      .setCorrection(TypicalLEDStrip); // initializes LED strip
  FastLED.setBrightness(g_brightness); // global brightness
  // showProgramCleanup(3000);
  sampling_period_us = round(1000000 * (1.0 / g_sampling_freq));
  xTaskCreatePinnedToCore(visualizer, "Visualizer", 10000, NULL, 1, &Task2, 0);
  xTaskCreatePinnedToCore(printLeds, "PrintLeds", 10000, NULL, 1, &Task1, 1);
}

// The program loop, this function is default, but gets deleted by the
// vTaskDelete.
void loop() {
  // visualizer();
  vTaskDelete(NULL);
}
