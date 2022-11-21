void clearText() {
  mempcpy(text, "", 1);
  lmd.clear();
  displayText();
}

tm convertExpectedArrivalTime(String expHour) {
  struct tm arrivalTime;

  String year = expHour.substring(0, 4);
  String month = expHour.substring(5, 7);
  if (month.startsWith("0")) {
    month = month.substring(1);
  }
  String day = expHour.substring(8, 10);
  if (expHour.startsWith("0")) {
    day = expHour.substring(1);
  }
  arrivalTime.tm_year = year.toInt() - 1900;
  arrivalTime.tm_mon = month.toInt() - 1;
  arrivalTime.tm_mday = day.toInt();
  arrivalTime.tm_hour = expHour.substring(11, 13).toInt();
  arrivalTime.tm_min = expHour.substring(14, 16).toInt();
  arrivalTime.tm_sec = expHour.substring(17, 19).toInt();
  arrivalTime.tm_isdst = -1;
  mktime(&arrivalTime);
  return arrivalTime;
}

String jsonVarToString(JSONVar value) {
  String result = JSON.stringify(value);
  result.remove(0, 1);
  result.remove(result.length() - 1);
  return result;
}

bool filterLineAndDest(String lineId, String dest) {
  if (lineId == "82" && (dest == "QUATRE VENTS" || dest == "GARE DE BERCHEM")) {
    return true;
  }
  if (lineId == "52" && dest == "GARE CENTRALE") {
    return true;
  }
  return false;
}

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while (!getLocalTime(&currentTime)) {
    Serial.println("Failed to obtain time, retry");
    delay(1000);
  }
}

bool checkIsOn() {
  //1:5
  if (currentTime.tm_hour >= 1 && currentTime.tm_hour <= 5 && currentTime.tm_min > 25) {
    return false;
  } else {
    return true;
  }
  return true;
}

void increaseBrightness(){
  intensity += 1;
  if (intensity > 10){
    intensity = 0;
  }
  lmd.setIntensity(intensity);  // 0 = low, 10 = high
}
