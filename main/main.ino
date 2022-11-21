#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <LEDMatrixDriver.hpp>
#include "time.h"




const uint8_t LEDMATRIX_CS_PIN = 5;

// Number of 8x8 segments you are connecting
const int LEDMATRIX_SEGMENTS = 4;
const int LEDMATRIX_WIDTH = LEDMATRIX_SEGMENTS * 8;

// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);


// Marquee speed (lower nubmers = faster)
const int ANIM_DELAY = 30;

int x = 0, y = 0;

const char* ssid = "Cathedrale";
const char* password = "surlemodem";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
int currentMin = -1;
int intensity = 2;
String finalText;

struct tm currentTime;


//const char* fingerprint = "90:6F:12:F3:36:A5:56:7B:94:83:47:5F:BC:A1:9C:D1:0C:83:5D:03";
String url = "https://stibmivb.opendatasoft.com/api/records/1.0/search/?dataset=waiting-time-rt-production&q=pointid+%3D+2734+or+pointid+%3D+2735+or+pointid+%3D+5120+or+pointid+%3D+5151&rows=20&timezone=Europe%2FBrussels";

char text[24] = "NO DATA";

bool isOn = true;


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  setupButton();
  lmd.setEnabled(true);
  lmd.setIntensity(intensity);
  init();
}


void loop() {
  getLocalTime(&currentTime);
  //update Time every minute
  if (currentMin != currentTime.tm_min) {
    clearText();
    currentMin = currentTime.tm_min;
    isOn = checkIsOn();
    if (isOn) {
      fetchData();
    }
  }
  if (isOn) {
    displayText();
  }
}

void init() {
  clearText();
  initWiFi();
  setupTime();
  currentMin = currentTime.tm_min;
  Serial.println("min : " + String(currentMin));
  fetchData();
}


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(" ");
  Serial.println(WiFi.localIP());
}




void fetchData() {
  if ((WiFi.status() == WL_CONNECTED)) {  //Check the current connection status

    HTTPClient http;
    JSONVar data;
    JSONVar timeData;

    http.begin(url.c_str());
    http.addHeader("Authorization", "apiKey ff8ced4a55aa246af099fbce1900ecf5224b4ab228df9aff3d88facd");
    int httpCode = http.GET();
    int parsingCount = 0;

    if (httpCode > 0) {
      String payload = http.getString();
      data = JSON.parse(payload);
      int dataIsParsed = -1;
      int recordLen = data["nhits"];
      finalText = String();
      for (int i = 0; i < recordLen; i++) {
        timeData = JSON.parse(data["records"][i]["fields"]["passingtimes"]);
        for (int j = 0; j < 1; j++) {
          String expTime = jsonVarToString(timeData[j]["expectedArrivalTime"]);
          String dest = jsonVarToString(timeData[j]["destination"]["fr"]);
          String lineId = jsonVarToString(timeData[j]["lineId"]);
          if (filterLineAndDest(lineId, dest) == false) {
            break;
          }
          parsingCount += 1;

          if (parsingCount > 1) {
            finalText += " - ";
          }
          tm arrivalTime = convertExpectedArrivalTime(expTime);
          calcTime(arrivalTime, dest, lineId);
        }
      }
      copyText();

    } else {
      Serial.println(httpCode);
      Serial.println(http.getString());
      Serial.println("Error on HTTP request");
      http.end();
      delay(500);
      fetchData();
    }

    http.end();
  } else {
    init();
  }
}


int calcTime(tm arrivalTime, String dest, String lineId) {

  Serial.println("START PARSING : " + lineId + " " + dest);

  int diff = 0;
  bool now = false;

  if (arrivalTime.tm_hour == currentTime.tm_hour) {
    diff = arrivalTime.tm_min - currentTime.tm_min;
  } else {
    // 15:04 14:45 -> 19
    if (arrivalTime.tm_hour - currentTime.tm_hour == 1) {
      diff = 60 - currentTime.tm_min;
      diff += arrivalTime.tm_min;
    } else {
      diff = (60 * (arrivalTime.tm_hour - currentTime.tm_hour)) - currentTime.tm_hour;
      diff += arrivalTime.tm_min;
    }
    //hour > current time .. normally
    Serial.println("HOUR PARSING ERROR : " + String(arrivalTime.tm_hour) + " , " + String(currentTime.tm_hour));
  }
  Serial.println("diff : " + String(diff));

  //TODO ADD NOW
  if (diff <= 0) {
    now = true;
  }

  // if (!now) {
  if (lineId == "52") {
    finalText += "B" + lineId;
    //+ " " + String(diff) + "M";
  } else {
    finalText += "T" + lineId;
    // + " " + String(diff) + "M";
  }
  if (!now) {
    finalText += " " + String(diff) + "M";
  } else {
    finalText += " NOW";
  }
  Serial.println("finalText :" + finalText);
  return 1;
}


void copyText() {
  int str_len = finalText.length() + 1;
  char charBuf[str_len];
  finalText.toCharArray(charBuf, str_len);
  mempcpy(text, charBuf, str_len);
}




 
SemaphoreHandle_t semaphore = nullptr;
void IRAM_ATTR handler(void* arg) {
  xSemaphoreGiveFromISR(semaphore, NULL);
}
 
void button_task(void* arg) {
  for(;;) {
    if(xSemaphoreTake(semaphore,portMAX_DELAY) == pdTRUE) {
      Serial.println("Oh, button pushed!\n");
      increaseBrightness();
    }
  }
}

// int ESP_INTR_FLAG_DEFAULT = 0;
// int BUTTON_PIN = 0;
 
void setupButton() {
  // Create a binary semaphore
  semaphore = xSemaphoreCreateBinary();
 
 // Setup the button GPIO pin
 gpio_pad_select_gpio(0 );
 
 // Quite obvious, a button is a input
 gpio_set_direction((gpio_num_t)0, GPIO_MODE_INPUT);
 
 // Trigger the interrupt when going from HIGH -> LOW ( == pushing button)
 gpio_set_intr_type((gpio_num_t)0, GPIO_INTR_NEGEDGE);
 
 // Associate button_task method as a callback
 xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
 
 // Install default ISR service 
 gpio_install_isr_service(0);
 
 // Add our custom button handler to ISR
 gpio_isr_handler_add((gpio_num_t)0, handler, NULL);
}

