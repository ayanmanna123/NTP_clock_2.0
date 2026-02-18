// ESP-01 pinout from top:

// GND    GP2 GP0 RX/GP3
// TX/GP1 CH  RST VCC

// MAX7219
// ESP-1 from rear
// Re Br Or Ye
// Gr -- -- --

// USB to Serial programming
// ESP-1 from rear, FF to GND, RR to GND before upload
// Gr FF -- Bl
// Wh -- RR Vi

// GPIO 2 - DataIn
// GPIO 1 - LOAD/CS
// GPIO 0 - CLK

// ------------------------
// NodeMCU 1.0 pinout:

// D8 - DataIn
// D7 - LOAD/CS
// D6 - CLK

#include "Arduino.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>

WiFiClient client;

String date;
// String temp = " Temp: --C";           // Default value before fetching data
// String weatherCondition = "Unknown";  // Stores weather condition (e.g.,
// Clear, Rain, Clouds)

// String tempMin = " Min: --C";
// String tempMax = " Max: --C";

// String weatherDescription = "Unknown";
String temp = "";               // Current temperature
String tempMin = "";            // Minimum temperature
String tempMax = "";            // Maximum temperature
String weatherCondition = "";   // Weather condition (e.g., Clouds, Rain)
String weatherDescription = ""; // Detailed description (e.g., scattered clouds)
String windSpeed = "";          // Wind speed (e.g., 3.5 m/s)
String holidayToday = "";
String holidayDescription = "";
String aqiDisplay = "";
String lat = "22.5726"; // Kolkata
String lon = "88.3639"; // Kolkata

// EEPROM Addresses
#define ADDR_MIN_TEMP 0
#define ADDR_MAX_TEMP 4
#define ADDR_LAST_DAY 8
#define EEPROM_MAGIC 12345 // To check if EEPROM has been initialized
#define ADDR_MAGIC 12

struct Holiday {
  int day;
  int month;
  const char *name;
  bool isHoliday;
};

// --- Add your holidays here! ---
Holiday holidays[] = {
    {1, 1, "New Year's Day / Vidya Balan Birthday", true},
    {5, 1, "Deepika Padukone Birthday", false},
    {6, 1, "A.R. Rahman & Diljit Dosanjh Birthday", false},
    {10, 1, "Hrithik Roshan Birthday", false},
    {11, 1, "Lal Bahadur Shastri Death Anniversary", false},
    {12, 1, "Swami Vivekananda Birthday", false},
    {13, 1, "Lohri", true},
    {14, 1, "Makar Sankranti / Pongal", true},
    {23, 1, "Netaji Subhash Chandra Bose Birthday", false},
    {26, 1, "Republic Day", true},
    {30, 1, "Mahatma Gandhi Death Anniversary", false},
    {1, 2, "Guru Ravidas Jayanti / Jackie Shroff Birthday", true},
    {5, 2, "Abhishek Bachchan Birthday", false},
    {12, 2, "Maharishi Dayanand Saraswati Jayanti", true},
    {15, 2, "Maha Shivaratri / Randhir Kapoor Birthday", true},
    {17, 2, "Santanu Birthday", false},
    {18, 2, "Manu Bhaker Birthday", false},
    {19, 2, "Chhatrapati Shivaji Maharaj Jayanti", true},
    {25, 2, "Shahid Kapoor Birthday", false},
    {3, 3, "Holika Dahan", true},
    {4, 3, "Holi", true},
    {14, 3, "Aamir Khan Birthday", false},
    {15, 3, "Alia Bhatt Birthday", false},
    {19, 3, "Ajay Birthday", false},
    {21, 3, "Id-ul-Fitr (Ramzan Id)", true},
    {23, 3, "Shaheed Diwas", false},
    {26, 3, "Ram Navami", true},
    {31, 3, "Mahavir Jayanti", true},
    {2, 4, "Ajay Devgn Birthday", false},
    {3, 4, "Good Friday", true},
    {5, 4, "Easter Sunday", true},
    {14, 4, "Ambedkar Jayanti / Baisakhi", true},
    {15, 4, "Vaisakhadi / Bahag Bihu", true},
    {24, 4, "Sachin Tendulkar Birthday", false},
    {1, 5, "May Day / Buddha Purnima", true},
    {9, 5, "Rabindranath Tagore Jayanti", false},
    {20, 5, "N.T. Rama Rao Jr. Birthday", false},
    {27, 5, "Id-ul-Zuha (Bakrid) / Nehru Death Anniversary", true},
    {26, 6, "Muharram / Arjun Kapoor Birthday", true},
    {7, 7, "M.S. Dhoni Birthday", false},
    {16, 7, "Rath Yatra / Katrina Kaif Birthday", true},
    {18, 7, "Priyanka Chopra & Smriti Mandhana Birthday", false},
    {15, 8, "Independence Day", true},
    {16, 8, "Saif Ali Khan Birthday", false},
    {26, 8, "Onam / Milad-un-Nabi", true},
    {28, 8, "Raksha Bandhan", true},
    {29, 8, "Major Dhyan Chand Birthday (National Sports Day)", false},
    {4, 9, "Janmashtami", true},
    {9, 9, "Akshay Kumar Birthday", false},
    {14, 9, "Ganesh Chaturthi", true},
    {17, 9, "Narendra Modi Birthday", false},
    {21, 9, "Kareena Kapoor Birthday", false},
    {28, 9, "Bhagat Singh & Ranbir Kapoor Birthday", false},
    {30, 9, "Ayan Birthday", false},
    {2, 10, "Gandhi Jayanti", true},
    {11, 10, "Amitabh Bachchan Birthday", false},
    {18, 10, "Maha Saptami", true},
    {19, 10, "Maha Ashtami / Maha Navami", true},
    {20, 10, "Dussehra", true},
    {23, 10, "Prabhas Birthday", false},
    {29, 10, "Karwa Chauth", true},
    {1, 11, "Aishwarya Rai Birthday", false},
    {2, 11, "Shah Rukh Khan Birthday", false},
    {5, 11, "Virat Kohli Birthday", false},
    {8, 11, "Diwali (Deepavali)", true},
    {9, 11, "Govardhan Puja", true},
    {11, 11, "Bhai Dooj", true},
    {14, 11, "Children's Day (Nehru Birthday)", false},
    {15, 11, "Chhath Puja", true},
    {24, 11, "Guru Nanak Jayanti", true},
    {6, 12, "Jasprit Bumrah & Ravindra Jadeja Birthday", false},
    {16, 12, "Aniket Birthday", false},
    {25, 12, "Christmas Day / Atal Bihari Vajpayee Birthday", true},
    {27, 12, "Salman Khan Birthday", false}};

const int holidayCount = sizeof(holidays) / sizeof(holidays[0]);

String calendarificApiKey = "9O07S1dri25aaqTwgbHHOi0vKFAv55Em";
int currentYear = 2026;
int currentMonth = 1;
int currentDay = 1;
float maxTemp = -100.0; // Set an extremely low initial value
float minTemp = 100.0;
float tempFloat1 = 0.0;
#define NUM_MAX 8

// for ESP-01 module
// #define DIN_PIN 2 // D4
// #define CS_PIN  3 // D9/RX
// #define CLK_PIN 0 // D3

// for NodeMCU 1.0
#define DIN_PIN 15 // D8
#define CS_PIN 13  // D7
#define CLK_PIN 12 // D6

#include "fonts.h"
#include "max7219.h"

#define RELAY_PIN 4

// Night Mode Settings
#define DAY_INTENSITY 1
#define NIGHT_INTENSITY 0
#define NIGHT_START 18 // 6 PM
#define NIGHT_END 9    // 9 AM

// Deep Night Mode (Display completely OFF)
#define DEEP_NIGHT_START 1 // 1 AM
#define DEEP_NIGHT_END 9   // 9 AM

// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char *ssid = "aniket_4g";       // SSID of local network
const char *password = "kolkata2003"; // Password on network

// =======================================================================

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);

  // Load persistent temperature data
  int magic;
  EEPROM.get(ADDR_MAGIC, magic);
  if (magic == EEPROM_MAGIC) {
    EEPROM.get(ADDR_MIN_TEMP, minTemp);
    EEPROM.get(ADDR_MAX_TEMP, maxTemp);
    Serial.print("Restored Temps from EEPROM: Min=");
    Serial.print(minTemp);
    Serial.print(", Max=");
    Serial.println(maxTemp);
  } else {
    // First time initialization
    EEPROM.put(ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.put(ADDR_MIN_TEMP, minTemp);
    EEPROM.put(ADDR_MAX_TEMP, maxTemp);
    EEPROM.commit();
  }

  Serial.print("Connecting WiFi ");
  WiFi.setSleepMode(WIFI_MODEM_SLEEP); // Enable Modem-Sleep for lifespan
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting ", 16);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;
int h, m, s;
// =======================================================================
void loop() {
  if (updCnt <= 0) { // Every 10 scrolls (~7.5 minutes)
    updCnt = 10;
    Serial.println("Getting data...");
    printStringWithShift("   Getting data ", 15);

    getTime(); // Fetch time
    getTemperature();
    getAQI();
    getHoliday();
    Serial.println("Data loaded");

    clkTime = millis();
  }

  // Show Date → Show Temperature → Show Time
  if (millis() - clkTime > 40000 && !del && dots) {
    clkTime = millis();

    for (int i = 1; i < 500; i++) {

      if (millis() - dotTime > 500) { // Blink dots every 500ms
        dotTime = millis();
        dots = !dots;
      }
      updateTime1();
      showAnimClock();
    }

    printStringWithShift(date.c_str(), 40); // Show Date: "TUE, 04/03/2025"
    delay(5000);
    printStringWithShift(temp.c_str(), 40);
    delay(5000); // Show current temperature
    printStringWithShift(("     Max: " + String(maxTemp) + " C").c_str(), 40);
    delay(2000);

    printStringWithShift(("     Min: " + String(minTemp) + " C").c_str(), 40);
    delay(2000);
    // printStringWithShift(tempMin.c_str(), 40);  // Show min temperature
    // delay(5000);

    // printStringWithShift(tempMax.c_str(), 40);  // Show max temperature
    // delay(5000);

    // printStringWithShift(weatherCondition.c_str(), 40);  // Show weather
    // condition (e.g., Clouds) delay(5000);

    printStringWithShift(weatherDescription.c_str(),
                         40); // Show description (e.g., scattered clouds)
    delay(5000);

    printStringWithShift(windSpeed.c_str(), 40); // Show wind speed
    delay(5000);
    printStringWithShift(aqiDisplay.c_str(), 40);
    delay(5000);

    if (holidayToday.length() > 0) {
      printStringWithShift(holidayToday.c_str(), 40);
      delay(3000);
      if (holidayDescription.length() > 0) {
        printStringWithShift(holidayDescription.c_str(), 40);
        delay(3000);
      }
    }

    updCnt--;
    clkTime = millis();
  }

  if (millis() - dotTime > 500) { // Blink dots every 500ms
    dotTime = millis();
    dots = !dots;
  }

  updateTime();       // Update the time
  updateBrightness(); // Adjust brightness based on time
  showAnimClock();
  // Show animated clock
}

// =======================================================================

void showSimpleClock() {
  dx = dy = 0;
  clr();
  showDigit(h / 10, 7, dig6x8);
  showDigit(h % 10, 15, dig6x8);
  showDigit(m / 10, 24, dig6x8);
  showDigit(m % 10, 32, dig6x8);
  // showDigit(s/10, 38, dig6x8);
  // showDigit(s%10, 46, dig6x8);
  setCol(22, dots ? B00100100 : 0);
  setCol(39, dots ? B00100100 : 0);
  refreshAll();
}

// =======================================================================

void showAnimClock() {
  byte digPos[6] = {7, 15, 24, 32, 42, 50}; // Shifted each by +2
  int digHt = 12;
  int num = 6;
  int i;
  if (del == 0) {
    del = digHt;
    for (i = 0; i < num; i++)
      digold[i] = dig[i];
    dig[0] = h / 10 ? h / 10 : 10;
    dig[1] = h % 10;
    dig[2] = m / 10;
    dig[3] = m % 10;
    dig[4] = s / 10;
    dig[5] = s % 10;
    for (i = 0; i < num; i++)
      digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
  } else
    del--;

  clr();
  for (i = 0; i < num; i++) {
    if (digtrans[i] == 0) {
      dy = 0;
      showDigit(dig[i], digPos[i], dig6x8);
    } else {
      dy = digHt - digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy = 0;
  setCol(22, dots ? B00100100 : 0);
  setCol(39, dots ? B00100100 : 0);
  refreshAll();
  delay(30);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data) {
  if (dy < -8 | dy > 8)
    return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy)
        scr[col + i] = v;
      else
        scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================

void setCol(int col, byte v) {
  if (dy < -8 | dy > 8)
    return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
    if (!dy)
      scr[col] = v;
    else
      scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data) {
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX * 8 + i] = 0;
  return w;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {

  if (c < ' ' || c > '~' + 25)
    return;
  c -= 32;
  int w = showChar(c, font);
  for (int i = 0; i < w + 1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char *s, int shiftDelay) {
  while (*s) {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
}

// =======================================================================

float utcOffset = 5.5; // OFSET WAKTU
long localEpoc = 0;
long localMillisAtUpdate = 0;

void getTime() {
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    // Serial.println(".");
    repeatCounter++;
  }

  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      String rawDate = line.substring(6, 22); // Extract "Tue, 04 Mar 2025"

      String dayOfWeek = rawDate.substring(0, 3); // "Tue"
      String day = rawDate.substring(5, 7);       // "04"
      String month = rawDate.substring(8, 11);    // "Mar"
      String year = rawDate.substring(12, 16);    // "2025"
      month.trim();
      // Convert month name to number
      String monthNum;
      if (month == "JAN")
        monthNum = "01";
      else if (month == "FEB")
        monthNum = "02";
      else if (month == "MAR")
        monthNum = "03";
      else if (month == "APR")
        monthNum = "04";
      else if (month == "MAY")
        monthNum = "05";
      else if (month == "JUN")
        monthNum = "06";
      else if (month == "JUL")
        monthNum = "07";
      else if (month == "AUG")
        monthNum = "08";
      else if (month == "SEP")
        monthNum = "09";
      else if (month == "OCT")
        monthNum = "10";
      else if (month == "NOV")
        monthNum = "11";
      else if (month == "DEC")
        monthNum = "12";

      else
        monthNum = "??";
      // Final formatted date: "TUE, 04/03/2025"
      // Ensure correct date formatting
      date = dayOfWeek + "," + day + "/" + month + "/" + year.substring(2, 4);

      // Debugging: Print date in Serial Monitor
      Serial.println("Formatted Date: " + date);

      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();

      currentDay = day.toInt();
      currentYear = year.toInt();

      // Daily Reset Logic for Temperature
      int lastDay;
      EEPROM.get(ADDR_LAST_DAY, lastDay);
      if (lastDay != currentDay && localEpoc != 0) {
        Serial.println("New day detected! Resetting min/max temps.");
        minTemp = tempFloat1 != 0 ? tempFloat1 : 100.0;
        maxTemp = tempFloat1 != 0 ? tempFloat1 : -100.0;
        EEPROM.put(ADDR_MIN_TEMP, minTemp);
        EEPROM.put(ADDR_MAX_TEMP, maxTemp);
        EEPROM.put(ADDR_LAST_DAY, currentDay);
        EEPROM.commit();
      }

      if (month == "JAN")
        currentMonth = 1;
      else if (month == "FEB")
        currentMonth = 2;
      else if (month == "MAR")
        currentMonth = 3;
      else if (month == "APR")
        currentMonth = 4;
      else if (month == "MAY")
        currentMonth = 5;
      else if (month == "JUN")
        currentMonth = 6;
      else if (month == "JUL")
        currentMonth = 7;
      else if (month == "AUG")
        currentMonth = 8;
      else if (month == "SEP")
        currentMonth = 9;
      else if (month == "OCT")
        currentMonth = 10;
      else if (month == "NOV")
        currentMonth = 11;
      else if (month == "DEC")
        currentMonth = 12;

      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();
}

// =======================================================================

void updateTime() {
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L);
  h = ((epoch % 86400L) / 3600) % 12;
  if (h == 0)
    h = 12; // Convert 0 to 12 for 12-hour format

  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

void updateTime1() {
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L);

  h = (epoch % 86400L) / 3600; // Keep 24-hour format
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

String apiKey = "aadde4019b15170339824527dc726672"; // Replace with your
                                                    // OpenWeatherMap API key
String city = "Kolkata";                            // Replace with your city
String country = "IN";                              // India country code

void getTemperature() {
  WiFiClient client;
  String server = "api.openweathermap.org";
  String request = "/data/2.5/weather?q=" + city + "," + country +
                   "&units=metric&appid=" + apiKey;

  Serial.println("Connecting to weather API...");

  if (!client.connect(server.c_str(), 80)) {
    Serial.println("Connection failed!");
    temp = " Temp: --C";
    tempMin = "--";
    tempMax = "--";
    weatherCondition = "Unknown";
    weatherDescription = "Unknown";
    windSpeed = "--";
    return;
  }

  client.print(String("GET ") + request + " HTTP/1.1\r\n" + "Host: " + server +
               "\r\n" + "Connection: close\r\n\r\n");

  delay(1000); // Wait for response
  String response;
  while (client.available()) {
    response += client.readString();
  }
  client.stop();

  // Extract temperature
  int tempIndex = response.indexOf("\"temp\":");
  if (tempIndex != -1) {
    int tempEnd = response.indexOf(",", tempIndex);
    String tempValue = response.substring(tempIndex + 7, tempEnd);
    float tempFloat = tempValue.toFloat() + 0.03;
    temp = "    Temp: " + String(tempFloat, 1) +
           " \xB0"
           "C";
    bool changed = updateTemperature(tempFloat);
    if (changed) {
      EEPROM.commit();
      Serial.println("Temperature record updated in EEPROM");
    }

  } else {
    temp = " Temp: --C";
  }

  // Extract min and max temperature
  int minIndex = response.indexOf("\"temp_min\":");
  if (minIndex != -1) {
    int minEnd = response.indexOf(",", minIndex);
    String minTempValue = response.substring(minIndex + 11, minEnd);
    float minTempFloat = minTempValue.toFloat() - 7.0; // <- change here
    tempMin = "      Min: " + String(minTempFloat, 1) +
              " \xB0"
              "C";
  }

  int maxIndex = response.indexOf("\"temp_max\":");
  if (maxIndex != -1) {
    int maxEnd = response.indexOf(",", maxIndex);
    String maxTempValue = response.substring(maxIndex + 11, maxEnd);
    float maxTempFloat = maxTempValue.toFloat() + 7.0; // <- change here
    tempMax = "       Max: " + String(maxTempFloat, 2) +
              " \xB0"
              "C";
  }

  // Extract weather condition (main)
  int weatherIndex = response.indexOf("\"main\":\"");
  if (weatherIndex != -1) {
    int weatherEnd = response.indexOf("\"", weatherIndex + 8);
    weatherCondition =
        "   Weather: " + response.substring(weatherIndex + 8, weatherEnd);
  } else {
    weatherCondition = "   Weather: Unknown";
  }

  // Extract weather description
  int descIndex = response.indexOf("\"description\":\"");
  if (descIndex != -1) {
    int descEnd = response.indexOf("\"", descIndex + 15);
    weatherDescription =
        "   Weather: " + response.substring(descIndex + 15, descEnd);
  } else {
    weatherDescription = "   Desc: Unknown";
  }

  // Extract wind speed
  int windIndex = response.indexOf("\"speed\":");
  if (windIndex != -1) {
    int windEnd = response.indexOf(",", windIndex);
    String windValue = response.substring(windIndex + 8, windEnd);
    float windKmph = windValue.toFloat() * 3.6; // Convert to km/h
    windSpeed = "    Wind speed: " + String(windKmph, 1) + " km/h";
  } else {
    windSpeed = "    Wind speed: -- km/h";
  }

  // Debug output
  Serial.println(temp);
  Serial.println(tempMin);
  Serial.println(tempMax);
  Serial.println(weatherCondition);
  Serial.println(weatherDescription);
  Serial.println(windSpeed);
}

bool updateTemperature(float newTemp) {
  tempFloat1 = newTemp; // Update current temperature
  bool changed = false;

  // Update maxTemp if the newTemp is higher
  if (tempFloat1 > maxTemp) {
    maxTemp = tempFloat1;
    EEPROM.put(ADDR_MAX_TEMP, maxTemp);
    changed = true;
  }

  // Update minTemp if the newTemp is lower
  if (tempFloat1 < minTemp) {
    minTemp = tempFloat1;
    EEPROM.put(ADDR_MIN_TEMP, minTemp);
    changed = true;
  }

  return changed;
}

void updateBrightness() {
  if (localEpoc == 0)
    return; // Wait until time is synced

  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L);
  int hour24 = ((epoch % 86400L) / 3600);

  static int lastIntensity = -1;
  static bool lastPowerState = true; // true = ON, false = OFF

  int currentIntensity = DAY_INTENSITY;
  bool currentPowerState = true;

  // Night Mode logic
  if (hour24 >= NIGHT_START || hour24 < NIGHT_END) {
    currentIntensity = NIGHT_INTENSITY;
  }

  // Deep Night Mode logic (Shut down display)
  if (hour24 >= DEEP_NIGHT_START && hour24 < DEEP_NIGHT_END) {
    currentPowerState = false;
  }

  // Handle Power State (Shutdown/Wakeup)
  if (currentPowerState != lastPowerState) {
    sendCmdAll(CMD_SHUTDOWN, currentPowerState ? 1 : 0);
    lastPowerState = currentPowerState;
    Serial.print("Display Power updated to: ");
    Serial.println(currentPowerState ? "ON" : "OFF");
  }

  // Handle Intensity (only if display is ON)
  if (currentPowerState && (currentIntensity != lastIntensity)) {
    sendCmdAll(CMD_INTENSITY, currentIntensity);
    lastIntensity = currentIntensity;
    Serial.print("Brightness updated to: ");
    Serial.println(currentIntensity);
  }
}

void checkRelayOff(int hour, int minute) {
  if (hour == 18 && minute == 19) {
    digitalWrite(RELAY_PIN, HIGH);
    delay(2000);
    digitalWrite(RELAY_PIN, LOW);
    delay(2000); // Turn off relay
  }
}
void getHoliday() {
  holidayToday = "";
  holidayDescription = "";

  Serial.print("Checking local holiday for: ");
  Serial.print(currentDay);
  Serial.print("/");
  Serial.println(currentMonth);

  for (int i = 0; i < holidayCount; i++) {
    // Debug print for 18/2 specifically
    if (holidays[i].day == 18 && holidays[i].month == 2) {
      Serial.print("Checking 18/2 entry: ");
      Serial.println(holidays[i].name);
    }

    if (holidays[i].day == currentDay && holidays[i].month == currentMonth) {
      Serial.print("Match found! Name: ");
      Serial.println(holidays[i].name);

      if (holidays[i].isHoliday) {
        holidayToday = "   Holiday: " + String(holidays[i].name);
      } else {
        holidayToday = "   Birthday: " + String(holidays[i].name);
      }
      break;
    }
  }

  if (holidayToday != "") {
    Serial.println("Local Event Found!");
  } else {
    Serial.println("No local event today.");
  }
}

void getAQI() {
  WiFiClient client;
  String server = "api.openweathermap.org";
  String request =
      "/data/2.5/air_pollution?lat=" + lat + "&lon=" + lon + "&appid=" + apiKey;

  Serial.println("Connecting to AQI API...");

  if (!client.connect(server.c_str(), 80)) {
    Serial.println("AQI Connection failed!");
    aqiDisplay = " AQI: --";
    return;
  }

  client.print(String("GET ") + request + " HTTP/1.1\r\n" + "Host: " + server +
               "\r\n" + "Connection: close\r\n\r\n");

  delay(1000);
  String response;
  while (client.available()) {
    response += client.readString();
  }
  client.stop();

  int aqiIndex = response.indexOf("\"aqi\":");
  if (aqiIndex != -1) {
    int aqiValue = response.substring(aqiIndex + 6, aqiIndex + 7).toInt();
    String aqiStatus = "Unknown";
    switch (aqiValue) {
    case 1:
      aqiStatus = "Good";
      break;
    case 2:
      aqiStatus = "Fair";
      break;
    case 3:
      aqiStatus = "Moderate";
      break;
    case 4:
      aqiStatus = "Poor";
      break;
    case 5:
      aqiStatus = "Very Poor";
      break;
    }
    aqiDisplay = "    AQI: " + String(aqiValue) + " (" + aqiStatus + ")";
  } else {
    aqiDisplay = " AQI: --";
  }
  Serial.println(aqiDisplay);
}

// ==========================================================