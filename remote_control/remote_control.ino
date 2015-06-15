#include <SoftwareSerial.h>

#define SPEED 57600

#define INIT 0
#define RESET 1
#define MODE 2
#define CONNECT 3
#define GET_IP 4
#define CONFIG 5
#define LISTEN 6
#define LISTENING 7

SoftwareSerial wifi(2, 3); //RX, TX


int state = INIT;

void setup() {
  Serial.begin(SPEED);
  wifi.begin(SPEED);
  wifi.write("AT\r\n");
}  

void loop() {
  state = process();
}

int process() {
  switch(state) {
    case INIT: return processInit();
    case RESET: return processReset();
    case MODE: return processMode();
    case CONNECT: return processConnect();
    case GET_IP: return processGetIp();
    case CONFIG: return processConfig();
    case LISTEN: return processListen();
    case LISTENING: return processListening();
    default: return state;
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
    wifi.write("AT+CWMODE=1\r\n");
    return MODE;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return RESET;
}

int processMode() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CWMODE=1 OK");
    delay(1000);
    wifi.write("AT+CWJAP=\"La Maldicion de Mandos\",\"spuenci1\"");
    return CONNECT;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return MODE;
}

int processConnect() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CWJAP OK");
    delay(1000);
    wifi.write("AT+CIFSR");
    return GET_IP;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return CONNECT;
}

int processGetIp() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CIFSR OK");
    Serial.println(text);
    delay(1000);
    wifi.write("AT+CIPMUX=1");
    return CONFIG;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return GET_IP;
}

int processConfig() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CIPMUX=1 OK");
    delay(1000);
    wifi.write("AT+CIPSERVER=1,80");
    return LISTEN;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return CONFIG;
}

int processListen() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CIPSERVER=1,80 OK");
    return LISTENING;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  return LISTEN;
}

int processListening() {
  String text = load();
  Serial.println(text);
  return LISTENING;
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
