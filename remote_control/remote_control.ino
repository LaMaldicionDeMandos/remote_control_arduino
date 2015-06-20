#include <SoftwareSerial.h>

#define SPEED 9600

//ESTADOS
#define INIT 0
#define RESET 1
#define MODE 2
#define GET_ENDPOINT 3
#define CONNECT 4
#define GET_IP 5
#define CONFIG 6
#define LISTEN 7
#define LISTENING 8
#define WAITING_RESPONSE 9
#define WAITING_MESSAGE_OK 10

//COMANDOS
#define FAVICON -1
#define BAD 0
#define PING 1
#define LEDS 2

#define LED 4

SoftwareSerial wifi(3, 2); //RX, TX

int state = INIT;
String message = "";
String ip;
int cip;

void setup() {
  Serial.begin(SPEED);
  wifi.begin(SPEED);
  wifi.println("AT");
  delay(10);
}  

void loop() {
  if(wifi.available()) {
    state = process();
  }  
  delay(300);
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
    case WAITING_RESPONSE: return processResponse(0);
    case WAITING_MESSAGE_OK: return processClose(0);
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
  if (has(text, "OK") || has(text, "no change")) {
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
  delay(3000);
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
    ip = findIp(text);
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
  if (has(text, "+IPD")) {
    int index = text.indexOf("+IPD,") + 5;
    String sub = text.substring(index);
    index = sub.indexOf(",");
    String cipText = sub.substring(0, index);
    cip = cipText.toInt();
    String request = sub.substring(sub.indexOf(":")+1, sub.indexOf(" HTTP/1.1"));
    int command = findCommand(request);
    message = processRequest(command, request);
    String mensaje = "MENSAJE: ";
    mensaje+=message;
    Serial.println(mensaje);
    wifi.print("AT+CIPSEND=");
    wifi.print(cip);
    wifi.print(",");
    wifi.println(message.length());
    if ( command == FAVICON) {
      Serial.println("Processing FAVICON");
      delay(100);
      wifi.println("AT+CIPCLOSE=0");
      return LISTENING;
    }
    return WAITING_RESPONSE;
  }
  return LISTENING;
}

int findCommand(String request) {
  if ( request.startsWith("GET")) {
    return GET(request);
  }
}

String processRequest(int command, String request) {
  switch(command) {
    case PING: return ping(); 
    case LEDS: return leds();
    default: return "HTTP/1.1 404 Bad Request\r\nContent-Type: text/text\r\nContent-Length: 0\r\n\r\n";
  }
}

int GET(String request) {
  Serial.println("Processing GET");
  String sub = request.substring(5);
  String command = sub;
  int pathIndex = sub.indexOf("/");
  if (pathIndex >=0) {
    command = sub.substring(0, pathIndex);
  }
  Serial.println("Processing: " + command);
  if (String("ping").equals(command)) {
    return PING;
  }
  if (String("leds").equals(command)) {
    return LEDS;
  }
  return BAD;
}

String ping() {
  Serial.println("Processing PING");
  return "HTTP/1.1 200 OK\r\nContent-Type: text/text\r\nContent-Length: 0\r\n\r\n";  
}

String leds() {
  Serial.println("Processing LEDS");
  String leds = "[";
  leds+= LED;
  leds+= "]";
  String message = "HTTP/1.1 200 OK\r\nContent-Type: text/text\r\nContent-Length: ";
  message+= leds.length();
  message+= "\r\n\r\n";
  message+= leds;  
  return String(message); 
}

int processResponse(int ttl) {
  Serial.println("WAITING MESSAGE");
  String text = load();
  Serial.println(text);
  if(has(text, ">") || ttl > 6) {
    Serial.println("Sending message " + message);
    wifi.println(String(message));
    delay(10);
    return WAITING_MESSAGE_OK;
  }
  if(has(text, "Unlink")) {
    return LISTENING;
  }
  delay(500);
  return processResponse(ttl+1);
}

int processClose(int ttl) {
  String text = load();
  if(has(text, "SEND OK") || ttl > 6) {
    Serial.println("Receive OK");
    wifi.println("AT+CIPCLOSE=0");
    return LISTENING;
  }
  delay(500);
  return processClose(ttl+1);
}

String load() {
  String text = "";
  while(wifi.available()) {
    char c = wifi.read();
    text+=c;
  }
  Serial.print(text);
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
