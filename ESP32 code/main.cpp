#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <esp_sntp.h>

#include "font_data.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN     4
#define DATA_PIN    6
#define CS_PIN      7

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

const int BOOT_BUTTON_PIN = 9;
unsigned long buttonPressStart = 0;
bool isButtonPressed = false;

WebServer server(80);
Preferences preferences;

String ssid = "";
String password = "";
String timeZone = "";
String ntpServer = "";
int colonMode = 1;
bool showSeconds = false;
int dayBrightness = 5;
int nightBrightness = 2;

IPAddress local_ip(192, 168, 50, 1);
IPAddress gateway(192, 168, 50, 1);
IPAddress subnet(255, 255, 255, 0);

bool isApMode = false;
unsigned long lastTimeUpdate = 0;
unsigned long ntpSyncInterval = 86400000;

int lastNtpSyncYday = -1;
int lastDayBrightnessSetYday = -1;
int lastNightBrightnessSetYday = -1;

const char HTML_PAGE[] = 
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>NTP Clock Settings</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f4f4f9;
      color: #303030;
      text-align: center;
      padding: 20px;
    }

    .card {
      background: white;
      max-width: 350px;
      margin: 0 auto;
      padding-top: 20px;
      padding-bottom: 20px;
      padding-left: 25px;
      padding-right: 25px;
      border-radius: 20px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }

    input {
      width: 100%;
      padding: 10px;
      margin-top: 10px;
      margin-bottom: 10px;
      border: 1px solid #ccc;
      border-radius: 8px;
      box-sizing: border-box;
      color: #505050;
      font-size: 15px;
    }

    input:hover { border: 1px solid #c15400; }
    input:focus { outline: 2px solid #ff6f00; }

    button {
      background: #ff6f00;
      color: white;
      padding: 10px;
      margin-top: 10px;
      margin-bottom: 10px;
      border: none;
      border-radius: 20px;
      width: 30%;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
    }

    button:hover { background: #c15400; }

    h2 {
      color: #303030;
      margin-top: 5px;
      margin-bottom: 25px;
      text-align: center;
    }

    h4 {
      color: #303030;
      margin-top: 10px;
      margin-bottom: 0px;
      text-align: left;
    }

    .password-wrapper {
      position: relative;
      width: 100%;
      margin-top: 10px;
      margin-bottom: 20px;
    }

    .password-wrapper input {
      margin-top: 0;
      margin-bottom: 0;
      padding-right: 40px;
    }

    .lock-icon {
      position: absolute;
      right: 9px;
      top: 50%;
      transform: translateY(-50%);
      cursor: pointer;
      font-size: 14px;
      user-select: none;
      filter: contrast(0);
    }

    .custom-select-container {
      position: relative;
      width: 100%;
      margin-top: 10px;
      margin-bottom: 20px;
      text-align: left;
    }

    .select-box {
      width: 100%;
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 8px;
      background: white;
      box-sizing: border-box;
      cursor: pointer;
      font-size: 15px;
      color: #505050;
      position: relative;
    }

    .select-box:hover { border: 1px solid #c15400; }
    .select-box:focus { outline: 2px solid #ff6f00; }
    
    .select-box::after {
      content: "\25BC";
      font-size: 15px;
      position: absolute;
      right: 10px;
      top: 45%;
      transform: translateY(-50%);
      color: #737373;
    }

    .select-options {
      display: none;
      position: absolute;
      top: 100%;
      left: 0;
      right: 0;
      background: white;
      border: 1px solid #ccc;
      border-radius: 8px;
      margin-top: 4px;
      max-height: 240px;
      overflow-y: auto;
      z-index: 1000;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }

    .select-options.show { display: block; }

    .option-item {
      padding: 10px;
      cursor: pointer;
      font-size: 15px;
      color: #505050;
    }

    .option-item:hover, .option-item.highlighted {
      background-color: #ff6f00;
      color: white;
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>Main Setup</h2>
    <form action="/save" method="POST">
      <h4><label>Wi-Fi SSID:</label></h4>
      <input type="text" name="ssid" id="ssid-input" value="Note: 2.4GHz only" required>
      
      <h4><label>Wi-Fi Password:</label></h4>
      <div class="password-wrapper">
        <input type="password" name="password" id="password-input" value="some_password_value" required>
        <span class="lock-icon" id="lock-toggle" onclick="togglePassword()">&#128275;</span>
      </div>
      
      <h4><label>Timezone:</label></h4>
      <input type="hidden" name="tz" id="tz-hidden-value" value="EET-2EEST,M3.5.0/3,M10.5.0/4">
      
      <div class="custom-select-container">
        <div class="select-box" id="tz-select-trigger" tabindex="0">Eastern Europe (Greece, Ukraine - Auto DST)</div>
        <div class="select-options" id="tz-options-list">
          <div class="option-item" data-value="GMT0">GMT/UTC (No DST)</div>
          <div class="option-item" data-value="WET0WEST,M3.5.0/1,M10.5.0">Western Europe (UK, Portugal - Auto DST)</div>
          <div class="option-item" data-value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe (Germany, France - Auto DST)</div>
          <div class="option-item highlighted" data-value="EET-2EEST,M3.5.0/3,M10.5.0/4">Eastern Europe (Greece, Ukraine - Auto DST)</div>
          <div class="option-item" data-value="EST5EDT,M3.2.0,M11.1.0">US Eastern (New York - Auto DST)</div>
          <div class="option-item" data-value="CST6CDT,M3.2.0,M11.1.0">US Central (Chicago - Auto DST)</div>
          <div class="option-item" data-value="MST7MDT,M3.2.0,M11.1.0">US Mountain (Denver - Auto DST)</div>
          <div class="option-item" data-value="PST8PDT,M3.2.0,M11.1.0">US Pacific (Los Angeles - Auto DST)</div>
          <div class="option-item" data-value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia Eastern (Sydney - Auto DST)</div>
        </div>
      </div>

      <h4><label>NTP Server Address:</label></h4>
      <input type="text" name="ntp" value="ua.pool.ntp.org" required>

      <h4><label>Colon Style:</label></h4>
      <input type="hidden" name="colon" id="colon-hidden-value" value="1">
      <div class="custom-select-container">
        <div class="select-box" id="colon-select-trigger" tabindex="0">Blink every second</div>
        <div class="select-options" id="colon-options-list">
          <div class="option-item highlighted" data-value="1">Blink every second</div>
          <div class="option-item" data-value="2">Always on</div>
          <div class="option-item" data-value="0">Always off</div>
        </div>
      </div>

      <h4><label>Show Seconds:</label></h4>
      <input type="hidden" name="secs" id="secs-hidden-value" value="0">
      <div class="custom-select-container">
        <div class="select-box" id="secs-select-trigger" tabindex="0">Disabled</div>
        <div class="select-options" id="secs-options-list">
          <div class="option-item highlighted" data-value="0">Disabled</div>
          <div class="option-item" data-value="1">Enabled</div>
        </div>
      </div>

      <h4><label>Day Brightness (starting at 06:00):</label></h4>
      <input type="hidden" name="brightDay" id="brightday-hidden-value" value="5">
      <div class="custom-select-container">
        <div class="select-box" id="brightday-select-trigger" tabindex="0">Level 5</div>
        <div class="select-options" id="brightday-options-list">
          <div class="option-item" data-value="1">Level 1</div>
          <div class="option-item" data-value="2">Level 2</div>
          <div class="option-item" data-value="3">Level 3</div>
          <div class="option-item" data-value="4">Level 4</div>
          <div class="option-item highlighted" data-value="5">Level 5</div>
          <div class="option-item" data-value="6">Level 6</div>
          <div class="option-item" data-value="7">Level 7</div>
          <div class="option-item" data-value="8">Level 8</div>
          <div class="option-item" data-value="9">Level 9</div>
          <div class="option-item" data-value="10">Level 10</div>
          <div class="option-item" data-value="11">Level 11</div>
          <div class="option-item" data-value="12">Level 12</div>
          <div class="option-item" data-value="13">Level 13</div>
          <div class="option-item" data-value="14">Level 14</div>
          <div class="option-item" data-value="15">Level 15</div>
        </div>
      </div>

      <h4><label>Night Brightness (starting at 22:00):</label></h4>
      <input type="hidden" name="brightNight" id="brightnight-hidden-value" value="2">
      <div class="custom-select-container">
        <div class="select-box" id="brightnight-select-trigger" tabindex="0">Level 2</div>
        <div class="select-options" id="brightnight-options-list">
          <div class="option-item" data-value="1">Level 1</div>
          <div class="option-item highlighted" data-value="2">Level 2</div>
          <div class="option-item" data-value="3">Level 3</div>
          <div class="option-item" data-value="4">Level 4</div>
          <div class="option-item" data-value="5">Level 5</div>
          <div class="option-item" data-value="6">Level 6</div>
          <div class="option-item" data-value="7">Level 7</div>
          <div class="option-item" data-value="8">Level 8</div>
          <div class="option-item" data-value="9">Level 9</div>
          <div class="option-item" data-value="10">Level 10</div>
          <div class="option-item" data-value="11">Level 11</div>
          <div class="option-item" data-value="12">Level 12</div>
          <div class="option-item" data-value="13">Level 13</div>
          <div class="option-item" data-value="14">Level 14</div>
          <div class="option-item" data-value="15">Level 15</div>
        </div>
      </div>

      <button type="submit">Save</button>
    </form>
  </div>

  <script>
    function togglePassword() {
      const passInput = document.getElementById('password-input');
      const lockIcon = document.getElementById('lock-toggle');
      if (passInput.type === 'password') {
        passInput.type = 'text';
      } else {
        passInput.type = 'password';
      }
    }

    function setupClearOnFocus(inputId, defaultValue) {
      const input = document.getElementById(inputId);
      input.addEventListener('focus', function() {
        if (this.value === defaultValue) {
          this.value = '';
        }
      });
      input.addEventListener('blur', function() {
        if (this.value === '') {
          this.value = defaultValue;
        }
      });
    }

    setupClearOnFocus('ssid-input', 'Note: 2.4GHz only');
    setupClearOnFocus('password-input', 'some_password_value');

    function setupCustomSelect(triggerId, listId, hiddenInputId) {
      const trigger = document.getElementById(triggerId);
      const list = document.getElementById(listId);
      const hiddenInput = document.getElementById(hiddenInputId);
      const options = list.querySelectorAll('.option-item');

      trigger.addEventListener('click', () => {
        document.querySelectorAll('.select-options').forEach(el => {
          if (el !== list) el.classList.remove('show');
        });
        list.classList.toggle('show');
      });

      options.forEach(option => {
        option.addEventListener('click', () => {
          options.forEach(opt => opt.classList.remove('highlighted'));
          
          option.classList.add('highlighted');
          trigger.innerText = option.innerText;
          hiddenInput.value = option.getAttribute('data-value');
          
          list.classList.remove('show');
        });
      });
    }

    setupCustomSelect('tz-select-trigger', 'tz-options-list', 'tz-hidden-value');
    setupCustomSelect('colon-select-trigger', 'colon-options-list', 'colon-hidden-value');
    setupCustomSelect('secs-select-trigger', 'secs-options-list', 'secs-hidden-value');
    setupCustomSelect('brightday-select-trigger', 'brightday-options-list', 'brightday-hidden-value');
    setupCustomSelect('brightnight-select-trigger', 'brightnight-options-list', 'brightnight-hidden-value');

    document.addEventListener('click', (e) => {
      if (!e.target.closest('.custom-select-container')) {
        document.querySelectorAll('.select-options').forEach(el => {
          el.classList.remove('show');
        });
      }
    });
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  timeZone = server.arg("tz");
  ntpServer = server.arg("ntp");
  colonMode = server.arg("colon").toInt();
  showSeconds = (server.arg("secs") == "1");
  dayBrightness = server.arg("brightDay").toInt();
  nightBrightness = server.arg("brightNight").toInt();

  preferences.begin("clock-cfg", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("tz", timeZone);
  preferences.putString("ntp", ntpServer);
  preferences.putInt("colon", colonMode);
  preferences.putBool("secs", showSeconds);
  preferences.putInt("brightDay", dayBrightness);
  preferences.putInt("brightNight", nightBrightness);
  preferences.end();

  String successHtml = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Settings Saved</title>
  <style>
      body {
        font-family: Arial, sans-serif;
        background: #f4f4f9;
        color: #333;
        text-align: center;
        padding: 20px;
      }

      .card {
        background: white;
        max-width: 350px;
        margin: 0 auto;
        padding: 20px;
        border-radius: 20px;
        box-shadow: 0 4px 8px rgba(0,0,0,0.1);
      }

      .success-icon {
        font-size: 48px;
        color: #ff6f00;
      }

      button {
        background: #ff6f00;
        color: white;
        padding: 10px;
        margin-top: 6px;
        margin-bottom: 10px;
        border: none;
        border-radius: 20px;
        width: 30%;
        font-size: 16px;
        font-weight: bold;
        cursor: pointer;
      }

      button:hover { background: #c15400; }

      h2 {
        color: #303030;
        margin-top: 0;
      }

      p {
        color: #505050;
        font-size: 15px;
        line-height: 1.6;
      }
  </style>
  </head>
  <body>
  <div class="card">
      <div class="success-icon">&#10004;</div>
      <h2>Settings Saved</h2>
      <p>You can close this tab and wait for the clock to reboot and connect to your home network.</p>
      <button type="button" onclick="window.close(); setTimeout(function(){ window.location.href='about:blank'; }, 200);">Close</button>
  </div>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", successHtml);
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  preferences.begin("clock-cfg", true);
  ssid = preferences.getString("ssid");
  password = preferences.getString("password");
  timeZone = preferences.getString("tz");
  ntpServer = preferences.getString("ntp");
  colonMode = preferences.getInt("colon");
  showSeconds = preferences.getBool("secs");
  dayBrightness = preferences.getInt("brightDay", 5);
  nightBrightness = preferences.getInt("brightNight", 2);
  preferences.end();

  myDisplay.begin();
  myDisplay.displayClear();
  myDisplay.setZone(0, 0, 3);
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.setIntensity(5);

  if (ssid != "") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    isApMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP("NTPClock-Setup", "12345678");
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    myDisplay.print("Setup");
  } else {
    myDisplay.print("WiFi on");
    delay(2000);
    myDisplay.print("Syncing");
    configTzTime(timeZone.c_str(), ntpServer.c_str());
    sntp_set_sync_interval(ntpSyncInterval);
    sntp_restart();
    delay(2000);
    myDisplay.begin(4);
  }

  if (showSeconds) {
    myDisplay.setZone(0, 0, 0);
    myDisplay.setZone(1, 1, 3);
    myDisplay.setFont(0, numeric7Seg);
    myDisplay.setFont(1, numeric7Se);
  } else {
    myDisplay.setZone(0, 0, 3);
    myDisplay.setFont(0, numeric7Se);
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 22) {
      myDisplay.setIntensity(dayBrightness);
    } else {
      myDisplay.setIntensity(nightBrightness);
    }
  }
}

void loop() {
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    
    if (!isButtonPressed) {
      isButtonPressed = true;
      buttonPressStart = millis();
    } else {
      unsigned long heldTime = millis() - buttonPressStart;
      myDisplay.setFont(0, nullptr);

      if (heldTime < 3000) {
        myDisplay.displayZoneText(0, "Hold", PA_CENTER, 75, 0, PA_PRINT, PA_NO_EFFECT);
      }
      if (heldTime >= 3000) {
        myDisplay.displayClear();
        myDisplay.displayZoneText(0, "Reset", PA_CENTER, 75, 0, PA_PRINT, PA_NO_EFFECT);
        preferences.begin("clock-cfg", false);
        preferences.clear();
        preferences.end();
        unsigned long resetDisplayTimer = millis();
        while (millis() - resetDisplayTimer < 2000) {
          myDisplay.displayAnimate();
        }
        ESP.restart();
      }
    }
  } else {
    if (isButtonPressed) {
      isButtonPressed = false;
      myDisplay.displayClear();
      delay(100);
      ESP.restart();
    }
  }

  if (isApMode) {
    server.handleClient();
  } else {
    myDisplay.displayAnimate();

    if (millis() - lastTimeUpdate >= 1000) {
      lastTimeUpdate = millis();

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char timeStr[9];
        char secStr[4];

        if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_yday != lastNtpSyncYday) {
          sntp_restart();
          lastNtpSyncYday = timeinfo.tm_yday;
        }
        
        if (timeinfo.tm_hour == 6 && timeinfo.tm_min == 0 && timeinfo.tm_yday != lastDayBrightnessSetYday) {
          myDisplay.setIntensity(dayBrightness);
          lastDayBrightnessSetYday = timeinfo.tm_yday;
        }

        if (timeinfo.tm_hour == 22 && timeinfo.tm_min == 0 && timeinfo.tm_yday != lastNightBrightnessSetYday) {
          myDisplay.setIntensity(nightBrightness);
          lastNightBrightnessSetYday = timeinfo.tm_yday;
        }

        char colonChar = ' ';
        if (colonMode == 2) {
          colonChar = ':';
        } else if (colonMode == 1) {
          colonChar = (timeinfo.tm_sec % 2 == 0) ? ':' : ' ';
        } else {
          colonChar = ' ';
        }

        if (showSeconds) {
          sprintf(secStr, "%02d", timeinfo.tm_sec);
          sprintf(timeStr, "%02d%c%02d", timeinfo.tm_hour, colonChar, timeinfo.tm_min);
          myDisplay.displayZoneText(0, secStr, PA_LEFT, 75, 0, PA_PRINT, PA_NO_EFFECT);
          myDisplay.displayZoneText(1, timeStr, PA_CENTER, 75, 0, PA_PRINT, PA_NO_EFFECT);
        } else {
          sprintf(timeStr, "%02d%c%02d", timeinfo.tm_hour, colonChar, timeinfo.tm_min);
          myDisplay.displayZoneText(0, timeStr, PA_CENTER, 75, 0, PA_PRINT, PA_NO_EFFECT);
        }
      }
    }
  }
}