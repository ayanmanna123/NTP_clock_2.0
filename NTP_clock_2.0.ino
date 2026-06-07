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
#include <ESP8266WebServer.h>

WiFiClient client;
ESP8266WebServer server(80);

bool show12HourClock = true;
bool show24HourClock = true;
bool showDate = true;
bool showWeather = true;
bool showHoliday = true;
bool autoBrightness = true;
int manualBrightness = 1; // Default DAY_INTENSITY
bool showCustomText = false;
String customText = "";
bool displayPower = true;

// Stopwatch State
bool showStopwatch = false;
bool stopwatchActive = false;
unsigned long stopwatchStartMillis = 0;
unsigned long stopwatchElapsedMillis = 0;

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
int updCnt = 0; // Move updCnt here so it's globally accessible

// EEPROM Addresses
#define ADDR_MIN_TEMP 0
#define ADDR_MAX_TEMP 4
#define ADDR_LAST_DAY 8
#define EEPROM_MAGIC 12345 // To check if EEPROM has been initialized
#define ADDR_MAGIC 12
#define EEPROM_MAGIC_V2 54321
#define ADDR_DISPLAY_POWER 16
#define ADDR_SHOW_12H 17
#define ADDR_SHOW_24H 18
#define ADDR_SHOW_DATE 19
#define ADDR_SHOW_WEATHER 20
#define ADDR_SHOW_HOLIDAY 21
#define ADDR_AUTO_BRIGHT 22
#define ADDR_MANUAL_BRIGHT 24

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


#define DAY_INTENSITY 1
#define NIGHT_INTENSITY 0
#define NIGHT_START 18 // 6 PM
#define NIGHT_END 9    // 9 AM

// Deep Night Mode (Display completely OFF)
#define DEEP_NIGHT_START 3 // 1 AM
#define DEEP_NIGHT_END 7   // 9 AM

// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char *ssid = "aniket_4g";       // SSID of local network
const char *password = "kolkata2003"; // Password on network

// =======================================================================

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    yield();
  }
}

const char ROOT_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>NTP Matrix Clock Control</title>
  <link href='https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap' rel='stylesheet'>
  <style>
    :root {
      --bg-color: #08090c;
      --card-bg: rgba(20, 22, 33, 0.7);
      --card-border: rgba(255, 255, 255, 0.08);
      --text-main: #f3f4f6;
      --text-sub: #9ca3af;
      --accent-cyan: #00f2fe;
      --accent-blue: #4facfe;
      --accent-glow: rgba(0, 242, 254, 0.3);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Outfit', sans-serif;
      background-color: var(--bg-color);
      background-image: radial-gradient(circle at 10% 20%, rgba(30, 20, 50, 0.2) 0%, transparent 40%),
                        radial-gradient(circle at 90% 80%, rgba(20, 40, 60, 0.2) 0%, transparent 40%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 30px 15px;
    }
    .container {
      width: 100%;
      max-width: 900px;
      display: flex;
      flex-direction: column;
      gap: 25px;
    }
    header {
      text-align: center;
      margin-bottom: 15px;
    }
    h1 {
      font-size: 2.2rem;
      font-weight: 800;
      background: linear-gradient(135deg, #00f2fe 0%, #4facfe 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      text-transform: uppercase;
      letter-spacing: 2px;
      margin-bottom: 5px;
      text-shadow: 0 0 30px rgba(0, 242, 254, 0.2);
    }
    .uptime {
      font-size: 0.95rem;
      color: var(--text-sub);
      background: rgba(255, 255, 255, 0.04);
      padding: 5px 12px;
      border-radius: 20px;
      display: inline-block;
      border: 1px solid rgba(255, 255, 255, 0.05);
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 25px;
    }
    @media (max-width: 768px) {
      .grid { grid-template-columns: 1fr; }
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 25px;
      backdrop-filter: blur(20px);
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
      transition: all 0.3s ease;
      display: flex;
      flex-direction: column;
    }
    .card:hover {
      border-color: rgba(0, 242, 254, 0.2);
      box-shadow: 0 10px 35px rgba(0, 242, 254, 0.05);
    }
    .card.full-width {
      grid-column: 1 / -1;
    }
    h2 {
      font-size: 1.35rem;
      font-weight: 600;
      margin-bottom: 20px;
      display: flex;
      align-items: center;
      gap: 10px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
      padding-bottom: 12px;
    }
    h2::before {
      content: '';
      display: inline-block;
      width: 4px;
      height: 18px;
      background: linear-gradient(#00f2fe, #4facfe);
      border-radius: 2px;
    }
    .form-group {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 18px;
    }
    .form-group label {
      font-size: 1.05rem;
      color: var(--text-main);
      cursor: pointer;
      flex: 1;
      padding-right: 15px;
    }
    
    .switch-container {
      position: relative;
      display: inline-block;
      width: 50px;
      height: 26px;
    }
    .switch-container input { opacity: 0; width: 0; height: 0; position: absolute; }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: rgba(255, 255, 255, 0.1);
      transition: .4s;
      border-radius: 34px;
      border: 1px solid rgba(255, 255, 255, 0.05);
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 18px; width: 18px;
      left: 3px; bottom: 3px;
      background-color: #fff;
      transition: .4s;
      border-radius: 50%;
      box-shadow: 0 2px 5px rgba(0,0,0,0.3);
    }
    input:checked + .slider {
      background: linear-gradient(135deg, #00f2fe 0%, #4facfe 100%);
      border: none;
    }
    input:checked + .slider:before {
      transform: translateX(24px);
    }
    
    .range-container {
      margin-top: 15px;
    }
    .range-label {
      display: flex;
      justify-content: space-between;
      font-size: 0.95rem;
      color: var(--text-sub);
      margin-bottom: 8px;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      background: rgba(255, 255, 255, 0.1);
      height: 6px;
      border-radius: 3px;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 18px; height: 18px;
      border-radius: 50%;
      background: #00f2fe;
      cursor: pointer;
      box-shadow: 0 0 10px #00f2fe;
      transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:hover {
      transform: scale(1.2);
    }
    input[type=range]:disabled {
      opacity: 0.3;
    }
    input[type=range]:disabled::-webkit-slider-thumb {
      background: #888;
      box-shadow: none;
      cursor: not-allowed;
    }

    input[type=text] {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid rgba(255, 255, 255, 0.1);
      background: rgba(0, 0, 0, 0.2);
      color: #fff;
      font-family: inherit;
      outline: none;
      transition: border-color 0.2s;
      margin-top: 8px;
    }
    input[type=text]:focus {
      border-color: #00f2fe;
    }

    input[type=submit], button.btn {
      width: 100%;
      padding: 14px;
      border-radius: 12px;
      border: none;
      color: white;
      font-weight: 600;
      font-size: 1rem;
      cursor: pointer;
      transition: all 0.2s ease;
      background: linear-gradient(135deg, #00f2fe 0%, #4facfe 100%);
      box-shadow: 0 4px 15px rgba(0, 242, 254, 0.2);
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    input[type=submit]:hover, button.btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0, 242, 254, 0.3);
    }
    input[type=submit]:active, button.btn:active {
      transform: translateY(0);
    }

    .stopwatch-display {
      font-size: 3rem;
      font-weight: 800;
      font-family: monospace;
      text-align: center;
      background: rgba(0, 0, 0, 0.3);
      padding: 15px;
      border-radius: 15px;
      border: 1px solid rgba(255, 255, 255, 0.05);
      color: #00f2fe;
      text-shadow: 0 0 15px rgba(0, 242, 254, 0.4);
      margin-bottom: 20px;
      letter-spacing: 2px;
    }
    .stopwatch-controls {
      display: flex;
      gap: 12px;
      margin-bottom: 20px;
    }
    .sw-btn {
      flex: 1;
      padding: 12px;
      border: none;
      border-radius: 10px;
      color: white;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
    }
    .sw-btn-start {
      background: linear-gradient(135deg, #10b981 0%, #059669 100%);
      box-shadow: 0 4px 12px rgba(16, 185, 129, 0.2);
    }
    .sw-btn-pause {
      background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
      box-shadow: 0 4px 12px rgba(245, 158, 11, 0.2);
    }
    .sw-btn-reset {
      background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
      box-shadow: 0 4px 12px rgba(239, 68, 68, 0.2);
    }
    .sw-btn:hover {
      transform: translateY(-2px);
      filter: brightness(1.1);
    }
    .sw-btn:active {
      transform: translateY(0);
    }
  </style>
</head>
<body>
  <div class='container'>
    <header>
      <h1>Matrix Clock Console</h1>
      <div class='uptime'>Uptime: %UPTIME%</div>
    </header>

    <div class='grid'>
      <div class='card'>
        <h2>Display Settings</h2>
        <form action='/update' method='GET'>
          <div class='form-group'>
            <label for='power'>System Display Power</label>
            <label class='switch-container'>
              <input type='checkbox' id='power' name='power' value='1' %POWER_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>
          
          <div style='height: 1px; background: rgba(255,255,255,0.08); margin: 15px 0;'></div>

          <div class='form-group'>
            <label for='c12h'>12-Hour Clock Mode</label>
            <label class='switch-container'>
              <input type='checkbox' id='c12h' name='12h' value='1' %C12H_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>
          
          <div class='form-group'>
            <label for='c24h'>24-Hour Clock Mode</label>
            <label class='switch-container'>
              <input type='checkbox' id='c24h' name='24h' value='1' %C24H_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>

          <div class='form-group'>
            <label for='show_date'>Show Calendar Date</label>
            <label class='switch-container'>
              <input type='checkbox' id='show_date' name='date' value='1' %DATE_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>

          <div class='form-group'>
            <label for='show_weather'>Show Weather Details</label>
            <label class='switch-container'>
              <input type='checkbox' id='show_weather' name='weather' value='1' %WEATHER_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>

          <div class='form-group'>
            <label for='show_holiday'>Show Holiday/Birthdays</label>
            <label class='switch-container'>
              <input type='checkbox' id='show_holiday' name='holiday' value='1' %HOLIDAY_CHECKED%>
              <span class='slider'></span>
            </label>
          </div>
          
          <input type='hidden' name='save' value='1'>
          <input type='submit' value='Save Settings' style='margin-top: 10px;'>
        </form>
      </div>

      <div class='card'>
        <h2>Stopwatch</h2>
        <div class='stopwatch-display' id='sw-time'>00:00.00</div>
        
        <div class='stopwatch-controls'>
          <button class='sw-btn sw-btn-start' onclick='controlSW("start")'>Start</button>
          <button class='sw-btn sw-btn-pause' onclick='controlSW("stop")'>Pause</button>
          <button class='sw-btn sw-btn-reset' onclick='controlSW("reset")'>Reset</button>
        </div>

        <div class='form-group'>
          <label for='sw-show-toggle'>Show Stopwatch on Matrix</label>
          <label class='switch-container'>
            <input type='checkbox' id='sw-show-toggle' onchange='toggleSWDisplay(this.checked)' %SW_SHOW_CHECKED%>
            <span class='slider'></span>
          </label>
        </div>
      </div>

      <div class='card full-width'>
        <h2>Advanced Configuration</h2>
        <form action='/update' method='GET'>
          <div class='form-group' style='flex-direction: column; align-items: flex-start; gap: 5px; margin-bottom: 25px;'>
            <div style='display: flex; align-items: center; justify-content: space-between; width: 100%;'>
              <span style='font-size: 1.05rem;'>Display Custom Banner Text</span>
              <label class='switch-container'>
                <input type='checkbox' id='show_custom' name='show_custom' value='1' %CUSTOM_CHECKED%>
                <span class='slider'></span>
              </label>
            </div>
            <input type='text' name='custom_text' value='%CUSTOM_TEXT%' placeholder='Enter marquee text...'>
          </div>

          <div style='height: 1px; background: rgba(255,255,255,0.08); margin: 15px 0 20px 0;'></div>

          <div class='form-group' style='flex-direction: column; align-items: flex-start; gap: 5px;'>
            <div style='display: flex; align-items: center; justify-content: space-between; width: 100%;'>
              <span style='font-size: 1.05rem;'>Auto Brightness Level</span>
              <label class='switch-container'>
                <input type='checkbox' id='auto_bright' name='auto_bright' value='1' onchange='document.getElementById("mb_slider").disabled = this.checked;' %AUTO_BRIGHT_CHECKED%>
                <span class='slider'></span>
              </label>
            </div>
            <div class='range-container' style='width: 100%;'>
              <div class='range-label'>
                <span>Manual Intensity</span>
                <span id='brightness-val'>%BRIGHTNESS_VAL%</span>
              </div>
              <input type='range' id='mb_slider' name='brightness' min='0' max='15' value='%BRIGHTNESS_VAL%' oninput='document.getElementById("brightness-val").innerText=this.value;' %BRIGHTNESS_DISABLED%>
            </div>
          </div>

          <input type='hidden' name='save' value='1'>
          <input type='submit' value='Save Adjustments' style='margin-top: 25px;'>
        </form>
      </div>
    </div>
  </div>

  <script>
    let swActive = false;
    let swElapsed = 0;
    let lastSyncTime = 0;
    let swInterval = null;

    function formatTime(ms) {
      const isHours = ms >= 3600000;
      if (isHours) {
        const hours = Math.floor(ms / 3600000);
        const mins = Math.floor((ms % 3600000) / 60000);
        const secs = Math.floor((ms % 60000) / 1000);
        return String(hours).padStart(2, '0') + ':' + 
               String(mins).padStart(2, '0') + ':' + 
               String(secs).padStart(2, '0');
      } else {
        const mins = Math.floor(ms / 60000);
        const secs = Math.floor((ms % 60000) / 1000);
        const cs = Math.floor((ms % 1000) / 10);
        return String(mins).padStart(2, '0') + ':' + 
               String(secs).padStart(2, '0') + '.' + 
               String(cs).padStart(2, '0');
      }
    }

    function updateLocalDisplay() {
      let currentElapsed = swElapsed;
      if (swActive) {
        currentElapsed += (Date.now() - lastSyncTime);
      }
      document.getElementById('sw-time').innerText = formatTime(currentElapsed);
    }

    function syncStatus() {
      fetch('/stopwatch/status')
        .then(response => response.json())
        .then(data => {
          swActive = data.active;
          swElapsed = data.elapsed;
          lastSyncTime = Date.now();
          document.getElementById('sw-show-toggle').checked = data.show;
          updateLocalDisplay();
          
          if (swActive && !swInterval) {
            swInterval = setInterval(updateLocalDisplay, 10);
          } else if (!swActive && swInterval) {
            clearInterval(swInterval);
            swInterval = null;
          }
        });
    }

    function controlSW(action) {
      fetch('/stopwatch?action=' + action)
        .then(() => {
          setTimeout(syncStatus, 50);
        });
    }

    function toggleSWDisplay(show) {
      fetch('/stopwatch?action=toggle_show&show=' + (show ? '1' : '0'))
        .then(() => {
          setTimeout(syncStatus, 50);
        });
    }

    syncStatus();
    setInterval(syncStatus, 2000);
  </script>
</body>
</html>
)rawhtml";

void handleRoot() {
  unsigned long uptimeMillis = millis();
  unsigned long days = uptimeMillis / 86400000;
  unsigned long hours = (uptimeMillis / 3600000) % 24;
  unsigned long mins = (uptimeMillis / 60000) % 60;
  unsigned long secs = (uptimeMillis / 1000) % 60;
  String uptimeStr = String(days) + "d " + String(hours) + "h " + String(mins) + "m " + String(secs) + "s";

  String html = FPSTR(ROOT_HTML);
  
  html.replace("%UPTIME%", uptimeStr);
  html.replace("%POWER_CHECKED%", displayPower ? "checked" : "");
  html.replace("%C12H_CHECKED%", show12HourClock ? "checked" : "");
  html.replace("%C24H_CHECKED%", show24HourClock ? "checked" : "");
  html.replace("%DATE_CHECKED%", showDate ? "checked" : "");
  html.replace("%WEATHER_CHECKED%", showWeather ? "checked" : "");
  html.replace("%HOLIDAY_CHECKED%", showHoliday ? "checked" : "");
  html.replace("%SW_SHOW_CHECKED%", showStopwatch ? "checked" : "");
  html.replace("%CUSTOM_CHECKED%", showCustomText ? "checked" : "");
  html.replace("%CUSTOM_TEXT%", customText);
  html.replace("%AUTO_BRIGHT_CHECKED%", autoBrightness ? "checked" : "");
  html.replace("%BRIGHTNESS_VAL%", String(manualBrightness));
  html.replace("%BRIGHTNESS_DISABLED%", autoBrightness ? "disabled" : "");

  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (!server.hasArg("save")) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }

  displayPower = server.hasArg("power");
  show12HourClock = server.hasArg("12h");
  show24HourClock = server.hasArg("24h");
  showDate = server.hasArg("date");
  showWeather = server.hasArg("weather");
  showHoliday = server.hasArg("holiday");
  showCustomText = server.hasArg("show_custom");
  if (server.hasArg("custom_text")) {
    customText = server.arg("custom_text");
  }
  autoBrightness = server.hasArg("auto_bright");
  if (server.hasArg("brightness")) {
    manualBrightness = server.arg("brightness").toInt();
  }
  
  EEPROM.put(ADDR_DISPLAY_POWER, displayPower);
  EEPROM.put(ADDR_SHOW_12H, show12HourClock);
  EEPROM.put(ADDR_SHOW_24H, show24HourClock);
  EEPROM.put(ADDR_SHOW_DATE, showDate);
  EEPROM.put(ADDR_SHOW_WEATHER, showWeather);
  EEPROM.put(ADDR_SHOW_HOLIDAY, showHoliday);
  EEPROM.put(ADDR_AUTO_BRIGHT, autoBrightness);
  EEPROM.put(ADDR_MANUAL_BRIGHT, manualBrightness);
  EEPROM.commit();

  updCnt = 0; // Force data fetch immediately after settings are updated


  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStopwatch() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "start") {
      if (!stopwatchActive) {
        stopwatchActive = true;
        stopwatchStartMillis = millis() - stopwatchElapsedMillis;
      }
    } else if (action == "stop") {
      if (stopwatchActive) {
        stopwatchActive = false;
        stopwatchElapsedMillis = millis() - stopwatchStartMillis;
      }
    } else if (action == "reset") {
      stopwatchActive = false;
      stopwatchElapsedMillis = 0;
    } else if (action == "toggle_show") {
      if (server.hasArg("show")) {
        showStopwatch = (server.arg("show") == "1");
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStopwatchStatus() {
  unsigned long elapsed = stopwatchElapsedMillis;
  if (stopwatchActive) {
    elapsed = millis() - stopwatchStartMillis;
  }
  
  String json = "{";
  json += "\"active\":" + String(stopwatchActive ? "true" : "false") + ",";
  json += "\"elapsed\":" + String(elapsed) + ",";
  json += "\"show\":" + String(showStopwatch ? "true" : "false");
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void showStaticIP(String ip) {
  clr();
  int totalWidth = 0;
  for (int i = 0; i < ip.length(); i++) {
    char c = ip[i];
    if (c >= '0' && c <= '9') {
      int digitIndex = c - '0';
      int len = pgm_read_byte(dig3x8);
      int w = pgm_read_byte(dig3x8 + 1 + digitIndex * len);
      totalWidth += w + 1;
    } else if (c == '.') {
      totalWidth += 2;
    }
  }
  if (totalWidth > 0) totalWidth--;

  int col = (64 - totalWidth) / 2;
  if (col < 0) col = 0;

  for (int i = 0; i < ip.length(); i++) {
    char c = ip[i];
    if (c >= '0' && c <= '9') {
      int digitIndex = c - '0';
      showDigit(digitIndex, col, dig3x8);
      int len = pgm_read_byte(dig3x8);
      int w = pgm_read_byte(dig3x8 + 1 + digitIndex * len);
      col += w + 1;
    } else if (c == '.') {
      setCol(col, B01000000);
      col += 2;
    }
  }
  refreshAll();
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);

  // Load persistent configuration and temperature data
  int magic;
  EEPROM.get(ADDR_MAGIC, magic);
  if (magic == EEPROM_MAGIC || magic == EEPROM_MAGIC_V2) {
    EEPROM.get(ADDR_MIN_TEMP, minTemp);
    EEPROM.get(ADDR_MAX_TEMP, maxTemp);
    if (magic == EEPROM_MAGIC_V2) {
      EEPROM.get(ADDR_DISPLAY_POWER, displayPower);
      EEPROM.get(ADDR_SHOW_12H, show12HourClock);
      EEPROM.get(ADDR_SHOW_24H, show24HourClock);
      EEPROM.get(ADDR_SHOW_DATE, showDate);
      EEPROM.get(ADDR_SHOW_WEATHER, showWeather);
      EEPROM.get(ADDR_SHOW_HOLIDAY, showHoliday);
      EEPROM.get(ADDR_AUTO_BRIGHT, autoBrightness);
      EEPROM.get(ADDR_MANUAL_BRIGHT, manualBrightness);
      Serial.println("Restored configs from EEPROM (V2)");
    } else {
      // Upgrade EEPROM to V2
      EEPROM.put(ADDR_MAGIC, EEPROM_MAGIC_V2);
      EEPROM.put(ADDR_DISPLAY_POWER, displayPower);
      EEPROM.put(ADDR_SHOW_12H, show12HourClock);
      EEPROM.put(ADDR_SHOW_24H, show24HourClock);
      EEPROM.put(ADDR_SHOW_DATE, showDate);
      EEPROM.put(ADDR_SHOW_WEATHER, showWeather);
      EEPROM.put(ADDR_SHOW_HOLIDAY, showHoliday);
      EEPROM.put(ADDR_AUTO_BRIGHT, autoBrightness);
      EEPROM.put(ADDR_MANUAL_BRIGHT, manualBrightness);
      EEPROM.commit();
    }
    Serial.print("Restored Temps from EEPROM: Min=");
    Serial.print(minTemp);
    Serial.print(", Max=");
    Serial.println(maxTemp);
  } else {
    // First time initialization
    EEPROM.put(ADDR_MAGIC, EEPROM_MAGIC_V2);
    EEPROM.put(ADDR_MIN_TEMP, minTemp);
    EEPROM.put(ADDR_MAX_TEMP, maxTemp);
    EEPROM.put(ADDR_DISPLAY_POWER, displayPower);
    EEPROM.put(ADDR_SHOW_12H, show12HourClock);
    EEPROM.put(ADDR_SHOW_24H, show24HourClock);
    EEPROM.put(ADDR_SHOW_DATE, showDate);
    EEPROM.put(ADDR_SHOW_WEATHER, showWeather);
    EEPROM.put(ADDR_SHOW_HOLIDAY, showHoliday);
    EEPROM.put(ADDR_AUTO_BRIGHT, autoBrightness);
    EEPROM.put(ADDR_MANUAL_BRIGHT, manualBrightness);
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

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.on("/stopwatch", handleStopwatch);
  server.on("/stopwatch/status", handleStopwatchStatus);
  server.begin();
  Serial.println("HTTP server started");

  // Show static IP address on MAX7219 matrix for 15 seconds on boot
  showStaticIP(WiFi.localIP().toString());
  smartDelay(15000);
  clr();
  refreshAll();
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;
int h, m, s;
// =======================================================================
void loop() {
  server.handleClient();

  if (showStopwatch) {
    if (displayPower) {
      showStopwatchTime();
    } else {
      clr();
      refreshAll();
    }
    updateBrightness();
    smartDelay(50); // Refresh at ~20Hz to keep centiseconds looking fluid
    return;
  }

  if (updCnt <= 0) { 
    // If only clock (or clock+date) is selected, make the interval roughly 1 hour (90 cycles of 40-45s)
    // If weather/holiday is selected, keep it at 10 cycles (~10-15 mins)
    if (!showWeather && !showHoliday) {
      updCnt = 90; 
    } else {
      updCnt = 10;
    }
    
    Serial.println("Getting data...");
    // Only show "Getting data" on the matrix if we're actually fetching weather/holiday
    // to avoid interrupting the clock display unnecessarily
    if (showWeather || showHoliday) {
      printStringWithShift("   Getting data ", 15);
    }

    getTime(); // Always fetch time to stay in sync
    
    if (showWeather) {
      getTemperature();
      getAQI();
    }
    
    if (showHoliday) {
      getHoliday();
    }
    
    Serial.println("Data loaded");
    clkTime = millis();
  }

  // Show Date → Show Temperature → Show Time
  unsigned long carouselDelay = (show12HourClock || show24HourClock) ? 40000 : 5000;
  if (millis() - clkTime > carouselDelay && !del && dots && displayPower) {
    clkTime = millis();

    if (show12HourClock || show24HourClock) {
      for (int i = 1; i < 500; i++) {
        server.handleClient();
        if (millis() - dotTime > 500) { // Blink dots every 500ms
          dotTime = millis();
          dots = !dots;
        }
        if (show12HourClock) {
          updateTime();
        } else {
          updateTime1();
        }
        showAnimClock();
      }
    }

    if (showDate) {
      showCenteredText(date);
      smartDelay(5000);
    }

    if (showWeather) {
      printStringWithShift(temp.c_str(), 40);
      smartDelay(5000); // Show current temperature
      printStringWithShift(("     Max: " + String(maxTemp) + " C").c_str(), 40);
      smartDelay(2000);

      printStringWithShift(("     Min: " + String(minTemp) + " C").c_str(), 40);
      smartDelay(2000);
      // printStringWithShift(tempMin.c_str(), 40);  // Show min temperature
      // smartDelay(5000);

      // printStringWithShift(tempMax.c_str(), 40);  // Show max temperature
      // smartDelay(5000);

      // printStringWithShift(weatherCondition.c_str(), 40);  // Show weather
      // condition (e.g., Clouds) smartDelay(5000);

      printStringWithShift(weatherDescription.c_str(),
                           40); // Show description (e.g., scattered clouds)
      smartDelay(5000);

      printStringWithShift(windSpeed.c_str(), 40); // Show wind speed
      smartDelay(5000);
      printStringWithShift(aqiDisplay.c_str(), 40);
      smartDelay(5000);
    }

    if (showHoliday && holidayToday.length() > 0) {
      printStringWithShift(holidayToday.c_str(), 40);
      smartDelay(3000);
      if (holidayDescription.length() > 0) {
        printStringWithShift(holidayDescription.c_str(), 40);
        smartDelay(3000);
      }
    }

    if (showCustomText && customText.length() > 0) {
      printStringWithShift(("   " + customText).c_str(), 40);
      smartDelay(3000);
    }

    updCnt--;
    clkTime = millis();
  }

  if (millis() - dotTime > 500) { // Blink dots every 500ms
    dotTime = millis();
    dots = !dots;
  }

  updateBrightness(); // Adjust brightness based on time

  if (displayPower) {
    if (show12HourClock || show24HourClock) {
      if (show12HourClock) {
        updateTime();       // Update the time
      } else {
        updateTime1();      // Fallback to 24h format if 12h is disabled
      }
      showAnimClock();
    } else {
      del = 0;            // Release animation lock
      clr();              // Clear display if both clocks are disabled
      refreshAll();
      smartDelay(50);     // Prevent SPI hardware lockup from continuous high-speed loop
    }
  } else {
    clr();
    refreshAll();
    smartDelay(100);      // Throttle SPI when display is powered down
  }
}

// =======================================================================

void showStopwatchTime() {
  unsigned long elapsed = stopwatchElapsedMillis;
  if (stopwatchActive) {
    elapsed += millis() - stopwatchStartMillis;
  }

  unsigned long hours = (elapsed / 3600000) % 100;
  unsigned long minutes = (elapsed / 60000) % 60;
  unsigned long seconds = (elapsed / 1000) % 60;
  unsigned long centiseconds = (elapsed / 10) % 100;

  int d1 = hours / 10;
  int d2 = hours % 10;
  int d3 = minutes / 10;
  int d4 = minutes % 10;
  int d5 = seconds / 10;
  int d6 = seconds % 10;
  int d7 = centiseconds / 10;
  int d8 = centiseconds % 10;

  clr();
  
  // Draw all 8 digits using the 5x8 rounded font (dig5x8rn)
  showDigit(d1, 5, dig5x8rn);
  showDigit(d2, 11, dig5x8rn);
  showDigit(d3, 19, dig5x8rn);
  showDigit(d4, 25, dig5x8rn);
  showDigit(d5, 33, dig5x8rn);
  showDigit(d6, 39, dig5x8rn);
  showDigit(d7, 47, dig5x8rn);
  showDigit(d8, 53, dig5x8rn);

  // Separators
  setCol(17, B00100100); // Colon between Hours and Minutes
  setCol(31, B00100100); // Colon between Minutes and Seconds
  setCol(45, B01000000); // Decimal point between Seconds and Centiseconds

  refreshAll();
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
    byte newDig[6];
    newDig[0] = h / 10 ? h / 10 : 10;
    newDig[1] = h % 10;
    newDig[2] = m / 10;
    newDig[3] = m % 10;
    newDig[4] = s / 10;
    newDig[5] = s % 10;
    
    bool changed = false;
    for (i = 0; i < num; i++) {
      if (newDig[i] != dig[i]) {
        changed = true;
      }
    }
    
    if (changed) {
      del = digHt;
      for (i = 0; i < num; i++) {
        digold[i] = dig[i];
        dig[i] = newDig[i];
        digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
      }
    }
  } else {
    del--;
  }

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
  smartDelay(30);
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
    smartDelay(shiftDelay);
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

void showCenteredText(String text) {
  clr();
  int totalWidth = 0;
  for (int i = 0; i < text.length(); i++) {
    unsigned char c = text[i];
    if (c < ' ' || c > '~' + 25) continue;
    c -= 32;
    int len = pgm_read_byte(font);
    totalWidth += pgm_read_byte(font + 1 + c * len) + 1;
  }
  if (totalWidth > 0) totalWidth--;
  
  int startCol = (NUM_MAX * 8 - totalWidth) / 2;
  if (startCol < 0) startCol = 0;
  
  int currentCol = startCol;
  for (int i = 0; i < text.length(); i++) {
    unsigned char c = text[i];
    if (c < ' ' || c > '~' + 25) continue;
    c -= 32;
    int len = pgm_read_byte(font);
    int w = pgm_read_byte(font + 1 + c * len);
    
    for (int j = 0; j < w; j++) {
      if (currentCol + j >= 0 && currentCol + j < 8 * NUM_MAX) {
        scr[currentCol + j] = pgm_read_byte(font + 1 + c * len + 1 + j);
      }
    }
    currentCol += w + 1;
  }
  refreshAll();
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
      // Final formatted date fits perfectly on an 8x8 matrix (e.g. "04/03/2025")
      date = day + "/" + monthNum + "/" + year;

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
  
  if (h < 0) h = 0;
  if (h > 99) h = 99;
}

void updateTime1() {
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L);

  h = (epoch % 86400L) / 3600; // Keep 24-hour format
  m = (epoch % 3600) / 60;
  s = epoch % 60;
  
  if (h < 0) h = 0;
  if (h > 99) h = 99;
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
  bool currentPowerState = displayPower;

  if (autoBrightness && currentPowerState) {
    // Night Mode logic
    if (hour24 >= NIGHT_START || hour24 < NIGHT_END) {
      currentIntensity = NIGHT_INTENSITY;
    }

    // Deep Night Mode logic (Shut down display)
    if (hour24 >= DEEP_NIGHT_START && hour24 < DEEP_NIGHT_END) {
      currentPowerState = false;
    }
  } else if (currentPowerState) {
    currentIntensity = manualBrightness;
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