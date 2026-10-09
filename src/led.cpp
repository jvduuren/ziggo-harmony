#include "led.h"

namespace {

struct Rgb {
  uint8_t r, g, b;
  bool operator!=(const Rgb& o) const { return r != o.r || g != o.g || b != o.b; }
};

constexpr Rgb kOff{0, 0, 0};
constexpr Rgb kAmber{24, 8, 0};
constexpr Rgb kBlue{0, 0, 10};
constexpr Rgb kPurple{20, 0, 24};
constexpr Rgb kWhite{24, 24, 24};

led::Mode mode = led::Mode::Off;
Rgb flashColor = kOff;
uint32_t flashUntil = 0;
uint32_t findMeUntil = 0;
Rgb shown{1, 1, 1};  // forces the first write

void write(const Rgb& c) {
  if (!(c != shown)) return;
  shown = c;
#if defined(LED_WS2812_PIN)
#if LED_SWAP_RG
  neopixelWrite(LED_WS2812_PIN, c.g, c.r, c.b);
#else
  neopixelWrite(LED_WS2812_PIN, c.r, c.g, c.b);
#endif
#elif defined(LED_GPIO_PIN)
  bool on = c.r || c.g || c.b;
#if LED_ACTIVE_LOW
  digitalWrite(LED_GPIO_PIN, on ? LOW : HIGH);
#else
  digitalWrite(LED_GPIO_PIN, on ? HIGH : LOW);
#endif
#endif
}

// true during the first `onMs` of every `periodMs`
bool blink(uint32_t now, uint32_t periodMs, uint32_t onMs) { return now % periodMs < onMs; }

}  // namespace

namespace led {

void begin() {
#if defined(LED_GPIO_PIN)
  pinMode(LED_GPIO_PIN, OUTPUT);
#endif
  write(kOff);
}

void selfTest() {
  const Rgb sequence[] = {{30, 0, 0}, {0, 30, 0}, {0, 0, 30}, kOff};
  for (const Rgb& c : sequence) {
    write(c);
    delay(300);
  }
}

void setMode(Mode m) { mode = m; }

void flash(uint8_t r, uint8_t g, uint8_t b, uint16_t ms) {
  flashColor = {r, g, b};
  flashUntil = millis() + ms;
}

void findMe(uint32_t ms) { findMeUntil = millis() + ms; }

void update() {
  uint32_t now = millis();
  if ((int32_t)(flashUntil - now) > 0) return write(flashColor);
  if ((int32_t)(findMeUntil - now) > 0) return write(blink(now, 200, 100) ? kWhite : kOff);
  switch (mode) {
    case Mode::Advertising:
      return write(blink(now, 1000, 100) ? kAmber : kOff);
    case Mode::Connected:
      return write(kBlue);
    case Mode::Learning:
      return write(blink(now, 250, 125) ? kPurple : kOff);
    default:
      return write(kOff);
  }
}

}  // namespace led
