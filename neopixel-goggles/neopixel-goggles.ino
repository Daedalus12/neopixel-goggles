#include <Adafruit_NeoPixel.h>

const byte PIN = 3;
const byte STRIPSIZE = 16;

const unsigned int audioSampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
const unsigned int audioMemoryLength = 5 * (1000/audioSampleWindow);
unsigned int audioMemory[audioMemoryLength];
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIPSIZE, PIN, NEO_GRB + NEO_KHZ800);

uint32_t hsi2rgb(float H, float S, float I) {
    uint8_t r, g, b;
    H = fmod(H, 360); // cycle H around to 0-360 degrees
    H = 3.14159*H / (float)180; // Convert to radians.
    S = S>0 ? (S<1 ? S : 1) : 0; // clamp S and I to interval [0,1]
    I = I>0 ? (I<1 ? I : 1) : 0;

    // Math! Thanks in part to Kyle Miller.
    if (H < 2.09439) {
        r = 255 * I / 3 * (1 + S*cos(H) / cos(1.047196667 - H));
        g = 255 * I / 3 * (1 + S*(1 - cos(H) / cos(1.047196667 - H)));
        b = 255 * I / 3 * (1 - S);
    }
    else if (H < 4.188787) {
        H = H - 2.09439;
        g = 255 * I / 3 * (1 + S*cos(H) / cos(1.047196667 - H));
        b = 255 * I / 3 * (1 + S*(1 - cos(H) / cos(1.047196667 - H)));
        r = 255 * I / 3 * (1 - S);
    }
    else {
        H = H - 4.188787;
        b = 255 * I / 3 * (1 + S*cos(H) / cos(1.047196667 - H));
        r = 255 * I / 3 * (1 + S*(1 - cos(H) / cos(1.047196667 - H)));
        g = 255 * I / 3 * (1 - S);
    }

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

template<class T>
trigScale(T& ret, const float& val, const T& valMin, const T& valMax) {
    ret = valMin + 0.5*(valMax - valMin)*(1 + val);
}

uint32_t hues[STRIPSIZE];
byte saturations[STRIPSIZE];

void setup() {
    strip.begin();
    strip.setBrightness(100); // should only be called once
    strip.show();

    for (byte k = 0; k < STRIPSIZE; ++k)
    {
        saturations[k] = 0;
        hues[k] = 0;
    }

    Serial.begin(9600);
    for (unsigned int i = 0; i < audioMemoryLength; ++i) {
        audioMemory[i] = 50;
    }
}

const uint16_t nCycles = 63;
const byte cycleTime = 15;
uint16_t i = 0;
int brightness;
int audioSample;

void loop() {
    double intensity, saturation;
    float val = cos(i * 2.0 * PI / nCycles + PI);
    trigScale(intensity, val, 0.35, 1.0);
    trigScale(saturation, val, 0.0, 1.0);

    unsigned long startMillis = millis();  // Start of sample window
    unsigned int peakToPeak = 0;   // peak-to-peak level

    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;

    // collect data for 50 mS
    while (millis() - startMillis < audioSampleWindow)
    {
        audioSample = analogRead(2);
        //Serial.println(audioSample);
        if (audioSample < 1024)  // toss out spurious readings
        {
            if (audioSample > signalMax)
            {
                signalMax = audioSample;  // save just the max levels
            }
            else if (audioSample < signalMin)
            {
                signalMin = audioSample;  // save just the min levels
            }
        }
    }
    unsigned int minV = 1024;
    unsigned int maxV = 0;
    for (unsigned int i = 0; i < audioMemoryLength - 1; ++i) {
        minV = min(minV, audioMemory[i + 1]);
        maxV = max(maxV, audioMemory[i + 1]);
        audioMemory[i] = audioMemory[i + 1];
    }

    peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
    minV = min(minV, peakToPeak);
    maxV = max(maxV, peakToPeak);
    audioMemory[audioMemoryLength - 1] = peakToPeak;

    byte b = constrain(map(peakToPeak, minV, maxV, 0, 255), 0, 255);

    Serial.println("");
    Serial.print(peakToPeak);
    Serial.print("\t");
    Serial.print(minV);
    Serial.print("\t");
    Serial.print(maxV);
    Serial.print("\t");
    Serial.print(b);
    Serial.println("");


    //++i;
    //if (i == nCycles) {
    //    i = 0;
    //    for (byte k = 0; k < STRIPSIZE; ++k)
    //    {
    //        if (random(2) == 0) {
    //            saturations[k] = 1;
    //            hues[k] = random(180, 270);
    //        } else {
    //            saturations[k] = 0;
    //            hues[k] = 0;
    //        }
    //    }
    //}

    for (byte k = 0; k < 16; ++k) {
        strip.setPixelColor(k, hsi2rgb(0, 1, (float)b/255.0f));
    }
    strip.show();

    delay(10);
}

void setColor(uint32_t c) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
    }
    strip.show();
}