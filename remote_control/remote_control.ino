#include <SoftwareSerial.h>

#define SPEED 57600

#define INIT 0
#define RESET 1
#define MODE 2

SoftwareSerial wifi(2, 3); //RX, TX


int state = INIT;

void setup() {
  Serial.begin(SPEED);
  wifi.bregin(SPEED);
  wifi.write("AT\r\n");
}  

void loop() {
  state = process();
}

int process() {
  switch(state) {
    case INIT: return processInit();
    case RESET: return processReset();
    default return state;
  }
}

int processError() {
  Serial.println("ERROR");
  wifi.write("AT+RST\r\n");
  return RESET;
}

int processInit() {
  String text = load();
  if(has(text, "OK")) {
    Serial.println("AT OK");
    delay(1000);
    wifi.write("AT+RST\r\n");
    return RESET;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return INIT;
}  

int processReset() {
  String text = load();
  if (has(text, "ready")) {
    Serial.println("AT+RST ready");
    delay(1000);
    wifi(write("AT+CWMODE=1\r\n");
    return MODE;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return RESET;
}

String load() {
  String text = "";
  while(wifi.available()) {
    char c = wifi.read();
    text+=c;
  }
  return text;
}

boolean has(String text, String search) {
  return text.indexOf(search) >= 0; 
}
