#include <Adafruit_NeoPixel.h>
#include <DS3231.h>
#include <Wire.h>

#include <stdint.h>

DS3231 clock1;

bool font[10][7] = {
  {0, 1, 1, 1, 1, 1, 1},
  {0, 0, 0, 1, 1, 0, 0},
  {1, 0, 1, 1, 0, 1, 1},
  {1, 0, 1, 1, 1, 1, 0},
  {1, 1, 0, 1, 1, 0, 0},
  {1, 1, 1, 0, 1, 1, 0},
  {1, 1, 1, 0, 1, 1, 1},
  {0, 0, 1, 1, 1, 0, 0},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 0}
};

#define PIN 14
#define PIXELS_PER_SEGMENT 6
#define NUM_SEGMENTS 2
#define DIGIT_PIXELS_PER_MODULE (7 * PIXELS_PER_SEGMENT* NUM_SEGMENTS)

#define MOD1_DIGITS_START  (0)
#define MOD1_DIGITS_END    (DIGIT_PIXELS_PER_MODULE)
#define MOD1_SECS_START    (MOD1_DIGITS_END + 1)
#define MOD1_SECS_END      (MOD1_SECS_START + 2)
#define MOD1_MINUTES_START (MOD1_SECS_END + 1)
#define MOD1_MINUTES_END   (MOD1_MINUTES_START + 5)
#define MOD1_HOURS_START   (MOD1_MINUTES_END + 1)
#define MOD1_HOURS_END     (MOD1_HOURS_START + 4)

#define MOD2_DIGITS_START  (MOD1_HOURS_END)
#define MOD2_DIGITS_END    (MOD2_DIGITS_START + DIGIT_PIXELS_PER_MODULE)
#define MOD2_MINUTES_START (MOD2_DIGITS_END + 1)
#define MOD2_MINUTES_END   (MOD2_MINUTES_START + 5)
#define MOD2_HOURS_START   (MOD2_MINUTES_END + 1)
#define MOD2_HOURS_END     (MOD2_HOURS_START + 4)
#define MOD2_DAYS_START    (MOD2_HOURS_END + 1)
#define MOD2_DAYS_END      (MOD2_DAYS_START + 3)

#define MOD3_DIGITS_START  (MOD2_DAYS_END)
#define MOD3_DIGITS_END    (MOD3_DIGITS_START + DIGIT_PIXELS_PER_MODULE)
#define MOD3_HOURS_START   (MOD3_DIGITS_END + 1)
#define MOD3_HOURS_END     (MOD3_HOURS_START + 3)
#define MOD3_DAYS_START    (MOD3_HOURS_END + 1)
#define MOD3_DAYS_END      (MOD3_DAYS_START + 3)
#define MOD3_MONTHS_START  (MOD3_DAYS_END + 1)
#define MOD3_MONTHS_END    (MOD3_MONTHS_START + 6)

#define NUM_PIXELS MOD3_MONTHS_END

#define COUNTDOWN_DATE_YEAR   (24)
#define COUNTDOWN_DATE_MONTH  (8)
#define COUNTDOWN_DATE_DAY    (10)
#define COUNTDOWN_DATE_HOUR   (0)
#define COUNTDOWN_DATE_MINUTE (0)
#define COUNTDOWN_DATE_SECOND (0)

#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define DAYS_PER_MONTH 30
#define MONTHS_PER_YEAR 12

#define SECONDS_PER_HOUR  (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY   (SECONDS_PER_HOUR * HOURS_PER_DAY)
#define SECONDS_PER_MONTH (SECONDS_PER_DAY * DAYS_PER_MONTH)

#define MONTH_UNIT 1
#define DAY_UNIT 2
#define HOUR_UNIT 3
#define MINUTE_UNIT 4
#define SECOND_UNIT 5


int32_t calculate_seconds_since_epoch(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  const uint8_t days_in_month[] = {31, (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int32_t seconds = ((uint32_t)(year - 1) * MONTHS_PER_YEAR * DAYS_PER_MONTH * HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + ((uint32_t)(month - 1) * DAYS_PER_MONTH * HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + ((uint32_t)(day - 1) * HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + ((uint32_t)hour * MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + ((uint32_t)minute * SECONDS_PER_MINUTE) + (uint32_t)second;
  return seconds;
}

int32_t calculate_remaining_seconds(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t target_year, uint8_t target_month, uint8_t target_day, uint8_t target_hour, uint8_t target_minute, uint8_t target_second) {
  int32_t seconds = calculate_seconds_since_epoch(year, month, day, hour, minute, second);
  int32_t target_seconds = calculate_seconds_since_epoch(target_year, target_month, target_day, target_hour, target_minute, target_second);
  return target_seconds - seconds;
}

#define UNIT_MONTHS  1
#define UNIT_DAYS    2
#define UNIT_HOURS   3
#define UNIT_MINUTES 4
#define UNIT_SECONDS 5

void calculate_biggest_subdivision(uint32_t remaining_seconds, uint8_t* outputs, uint8_t* output_units) {
  uint8_t max_unit = UNIT_SECONDS;
  uint8_t max_value = 0;

  Serial.printf("rem secs 0 %d\n", remaining_seconds);

  if (remaining_seconds >= SECONDS_PER_MONTH) {
    uint8_t months = remaining_seconds / SECONDS_PER_MONTH;
    if (months > max_value) {
      max_value = months;
      max_unit = UNIT_MONTHS;
    }
    remaining_seconds -= months * SECONDS_PER_MONTH;
  } else if (remaining_seconds >= SECONDS_PER_DAY) {
    uint8_t days = remaining_seconds / SECONDS_PER_DAY;
    if (days > max_value) {
      max_value = days;
      max_unit = UNIT_DAYS;
    }
    remaining_seconds -= days * SECONDS_PER_DAY;
  } else if (remaining_seconds >= SECONDS_PER_HOUR) {
    uint8_t hours = remaining_seconds / SECONDS_PER_HOUR;
    if (hours > max_value) {
      max_value = hours;
      max_unit = UNIT_HOURS;
    }
    remaining_seconds -= hours * SECONDS_PER_HOUR;
  }

  outputs[0] = max_value;
  output_units[0] = max_unit;

  max_value = 0;
  max_unit = UNIT_SECONDS;

  if (remaining_seconds >= SECONDS_PER_DAY) {
    uint8_t days = remaining_seconds / SECONDS_PER_DAY;
    if (days > max_value) {
      max_value = days;
      max_unit = UNIT_DAYS;
    }
    remaining_seconds -= days * SECONDS_PER_DAY;
  } else if (remaining_seconds >= SECONDS_PER_HOUR) {
    uint8_t hours = remaining_seconds / SECONDS_PER_HOUR;
    if (hours > max_value) {
      max_value = hours;
      max_unit = UNIT_HOURS;
    }
    remaining_seconds -= hours * SECONDS_PER_HOUR;
  } else if (remaining_seconds >= SECONDS_PER_MINUTE) {
    uint8_t minutes = remaining_seconds / SECONDS_PER_MINUTE;
    if (minutes > max_value) {
      max_value = minutes;
      max_unit = UNIT_MINUTES;
    }
    remaining_seconds -= minutes * SECONDS_PER_MINUTE;
  }

  outputs[1] = max_value;
  output_units[1] = max_unit;

  max_value = 0;
  max_unit = UNIT_SECONDS;

  if (remaining_seconds >= SECONDS_PER_HOUR) {
    uint8_t hours = remaining_seconds / SECONDS_PER_HOUR;
    if (hours > max_value) {
      max_value = hours;
      max_unit = UNIT_HOURS;
    }
    remaining_seconds -= hours * SECONDS_PER_HOUR;
  } else if (remaining_seconds >= SECONDS_PER_MINUTE) {
    uint8_t minutes = remaining_seconds / SECONDS_PER_MINUTE;
    if (minutes > max_value) {
      max_value = minutes;
      max_unit = UNIT_MINUTES;
    }
    remaining_seconds -= minutes * SECONDS_PER_MINUTE;
  } else {
    max_value = remaining_seconds;
    max_unit = UNIT_SECONDS;
  }

  outputs[2] = max_value;
  output_units[2] = max_unit;
}


Adafruit_NeoPixel pixels(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
int counter;
uint8_t color[3] =       {0xff, 0x00, 0x00};
uint8_t strip_color[3] = {0x00, 0xff, 0x00};

void newRandomColor() {
  color[0] = rand() % 255;
  color[1] = rand() % 255;
  color[2] = rand() % 255;
}




void setup() {
  Wire.begin();

  pixels.begin();
  pixels.show();
  pixels.setBrightness(255);

  counter = 99;

  //newRandomColor();

  Serial.begin(57600);

  pinMode(2, OUTPUT);
}

void displayNumberAtDigit(uint16_t start_idx, int number, int digit) {
  if (number == 0 && digit == 1) return; // no leading zero
  if (number > 9) {
    return;
  }

  start_idx += digit * PIXELS_PER_SEGMENT * 7;
  for (uint8_t i = 0; i < 7; i++) {
    for (uint8_t ii = 0; ii < PIXELS_PER_SEGMENT; ii++) {
      uint16_t pixel_idx = start_idx + i * PIXELS_PER_SEGMENT + ii;

      if (font[number][i] == 1) {
        pixels.setPixelColor(pixel_idx, pixels.Color(color[0], color[1], color[2]));
      } else {
        pixels.setPixelColor(pixel_idx, pixels.Color(0, 0, 0));
      }
    }
  }
}

void setStrip(uint16_t start_idx, uint16_t end_idx) {
  for (uint16_t i = start_idx; i < end_idx; i++) {
    pixels.setPixelColor(i, pixels.Color(strip_color[0], strip_color[1], strip_color[2]));
  }
}

void loop() {
  bool century = false;
  bool h12 = false;
  bool pm = false;
  uint8_t year = clock1.getYear();
  uint8_t month = clock1.getMonth(century);
  uint8_t day = clock1.getDate();
  uint8_t hour = clock1.getHour(h12, pm);
  uint8_t minute = clock1.getMinute();
  uint8_t second = clock1.getSecond();

  Serial.print(year, DEC);
  Serial.print(month, DEC);
  Serial.print(day, DEC);
  Serial.print(hour, DEC);
  Serial.print(minute, DEC);
  Serial.print(second, DEC);
  Serial.println();

  int32_t remaining_seconds = calculate_remaining_seconds(
                                 year, month, day, hour, minute, second,
                                 COUNTDOWN_DATE_YEAR, COUNTDOWN_DATE_MONTH, COUNTDOWN_DATE_DAY,
                                 COUNTDOWN_DATE_HOUR, COUNTDOWN_DATE_MINUTE, COUNTDOWN_DATE_SECOND
                               );

  // if the time has already passed, count up since the date
  if(remaining_seconds < 0) {
    remaining_seconds = -remaining_seconds;
  }

  uint8_t digits[3] = {0};
  uint8_t units[3] = {0};

  calculate_biggest_subdivision(remaining_seconds, digits, units);

  Serial.printf("digits %d %d %d\n", digits[0], digits[1], digits[2]);
  Serial.printf("units %d %d %d\n", units[0], units[1], units[2]);
  Serial.println();

  uint16_t starts[3] = {MOD3_DIGITS_START, MOD2_DIGITS_START, MOD1_DIGITS_START};
  uint16_t unit_starts[3][5] = {
    {MOD3_MONTHS_START, MOD3_DAYS_START,  MOD3_HOURS_START,   0xffff,             0xffff         },
    {0xffff,            MOD2_DAYS_START,  MOD2_HOURS_START,   MOD2_MINUTES_START, 0xffff         },
    {0xffff,            0xffff,           MOD1_HOURS_START,   MOD1_MINUTES_START, MOD1_SECS_START},
  };
  uint16_t unit_ends[3][5] = {
    {MOD3_MONTHS_END,   MOD3_DAYS_END,    MOD3_HOURS_END,     0xffff,             0xffff         },
    {0xffff,            MOD2_DAYS_END,    MOD2_HOURS_END,     MOD2_MINUTES_END,   0xffff         },
    {0xffff,            0xffff,           MOD1_HOURS_END,     MOD1_MINUTES_END,   MOD1_SECS_END  },
  };

  pixels.clear();
  
  if((hour >= 21) || (hour < 9)) {
    for(uint16_t i = 0; i < NUM_PIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
    return; // turn off clock between 9pm and 9am
  }
  
  for (uint8_t i = 0; i < 3; i++) {
    displayNumberAtDigit(starts[i], digits[i] % 10, 0);
    displayNumberAtDigit(starts[i], digits[i] / 10, 1);

    uint8_t unit_idx = units[i] - 1;
    setStrip(unit_starts[i][unit_idx], unit_ends[i][unit_idx]);
  }
  pixels.show();
  delay(100);
  if(counter-- == 0) counter = 99;

  digitalWrite(2, counter % 2 == 0 ? HIGH : LOW);


  /*
    displayNumberAtDigit(MOD1_DIGITS_START, counter % 10, 0);
    displayNumberAtDigit(MOD1_DIGITS_START, counter / 10, 1);

    displayNumberAtDigit(MOD2_DIGITS_START, counter % 10, 0);
    displayNumberAtDigit(MOD2_DIGITS_START, counter / 10, 1);

    displayNumberAtDigit(MOD3_DIGITS_START, counter % 10, 0);
    displayNumberAtDigit(MOD3_DIGITS_START, counter / 10, 1);

    uint16_t start_idx = 0, end_idx = 0;
    switch(counter % 6) {
    case 0:
      start_idx = MOD1_SECS_START;
      end_idx   = MOD1_SECS_END;
      break;
    case 1:
      start_idx = MOD1_MINUTES_START;
      end_idx   = MOD1_MINUTES_END;
      break;
    case 2:
      start_idx = MOD1_HOURS_START;
      end_idx   = MOD1_HOURS_END;
      break;
    case 3:
      start_idx = MOD2_MINUTES_START;
      end_idx   = MOD2_MINUTES_END;
      break;
    case 4:
      start_idx = MOD2_HOURS_START;
      end_idx   = MOD2_HOURS_END;
      break;
    case 5:
      start_idx = MOD2_DAYS_START;
      end_idx   = MOD2_DAYS_END;
      break;
    }

    for(uint16_t i = start_idx; i < end_idx; i++) {
    pixels.setPixelColor(i, pixels.Color(color[0], color[1], color[2]));
    }
    pixels.show();

    delay(500);
  */
}
