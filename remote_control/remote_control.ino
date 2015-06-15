#include <SoftwareSerial.h>

#define SPEED 57600

#define INIT 0
#define RESET 1
#define MODE 2
#define GET_ENDPOINT 3
#define CONNECT 4
#define GET_IP 5
#define CONFIG 6
#define LISTEN 7
#define LISTENING 8

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
    case GET_ENDPOINT: return processGetEndpoint();
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
  Serial.print(text);
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
  Serial.print(text);  
  return RESET;
}

int processMode() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CWMODE=1 OK");
    delay(1000);
    wifi.write("AT+CWLAP\r\n");
    return GET_ENDPOINT;
  } else if (has(text, "ERROR")) {
    return processError();
  }
  Serial.print(text);  
  return MODE;
}

int processGetEndpoint() {
  delay(5000);
  String text = load();
  Serial.println(text);
  wifi.write("AT+CWJAP=\"La Maldicion de Mandos\",\"spuenci1\"\r\n");
  return CONNECT;
}

int processConnect() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CWJAP OK");
    delay(1000);
    wifi.write("AT+CIFSR\r\n");
    return GET_IP;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  Serial.print(text);
  return CONNECT;
}

int processGetIp() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CIFSR OK");
    String ip = findIp(text);
    Serial.println(ip);
    delay(1000);
    wifi.write("AT+CIPMUX=1\r\n");
    return CONFIG;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  Serial.print(text);
  return GET_IP;
}

int processConfig() {
  String text = load();
  if (has(text, "OK")) {
    Serial.println("AT+CIPMUX=1 OK");
    delay(1000);
    wifi.write("AT+CIPSERVER=1,80\r\n");
    return LISTEN;
  } else if (has(text, "ERROR")) {
    return processError();
  }  
  Serial.print(text);
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
  Serial.print(text);
  return LISTEN;
}

int processListening() {
  String text = load();
  Serial.print(text);
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

String findIp(String text) {
  int index = text.indexOf("STAIP,\"") + 7;
  String beginIp = text.substring(index);
  return beginIp.substring(0, beginIp.indexOf("\""));
}
