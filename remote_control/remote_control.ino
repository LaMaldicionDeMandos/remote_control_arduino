#include <SoftwareSerial.h>

#define SPEED 9600

// Strings
//COMMANDS
#define COMMAND_AT "AT\r\n"
#define COMMAND_RESET "AT+RST\r\n"
#define COMMAND_MODE "AT+CWMODE=1\r\n"
#define COMMAND_LAP "AT+CWLAP\r\n"
#define COMMAND_JAP "AT+CWJAP="
#define COMMAND_FSR "AT+CIFSR\r\n"
#define COMMAND_MUX "AT+CIPMUX=1\r\n"
#define COMMAND_SERVER "AT+CIPSERVER=1,80\r\n"
#define COMMAND_SEND "AT+CIPSEND="
#define COMMAND_CLOSE "AT+CIPCLOSE="

//RESP
#define OK "OK"
#define NOTHING ""
#define READY "ready"
#define NO_CHANGE "no change"
#define RES_WAIT ">"

//HTTP
#define HTTP_INIT "+IPD,"
#define HTTP_GET "GET"
#define HTTP_PUT "PUT"
#define HTTP_HEAD "HTTP/1.1 200 OK\r\nContent-Type: text/text\r\nContent-Length: "
#define HTTP_HEAD_NO_CHANGE "HTTP/1.1 304 Not Modified\r\nContent-Type: text/text\r\nContent-Length: "
#define HTTP_END_HEAD "\r\n\r\n"
#define HTTP_BAD_RESPONSE "HTTP/1.1 404 Bad Request\r\nContent-Type: text/text\r\nContent-Length: 0\r\n\r\n"

//REQUESTS
#define REQ_PING "ping"
#define REQ_LEDS "leds"
#define REQ_STATUS "status"
#define REQ_LED "led"

//MISELANEUS
#define COMMA ","
#define ON "on"
#define OFF "off"
#define SLASH "/"
#define COLON ":"
#define SPACE " "

#define CHAR_SLASH '/'
#define CHAR_SPACE ' '
#define CHAR_COMMA ','

#define LED 4

int ledList[] = {LOW, LOW, LOW, LOW, LOW};

SoftwareSerial wifi(3, 2); //RX, TX

String ip;

void setup() {
  Serial.begin(SPEED);
  wifi.begin(SPEED);
  delay(10);
  configure();
}  

void loop() {
  if(wifi.available()) {
    listen();
  }
}

void configure() {
  configureInit();
  configureReset();
  configureMode();
  findEndpoint();
  configureEndpoint();
  getIp();
  configureMux();
  configureServer();
}

void configureInit() {
  command(COMMAND_AT, OK, NOTHING, 200);
}

void configureReset() {
  command(COMMAND_RESET, READY, NOTHING, 3000);
}

void configureMode() {
  command(COMMAND_MODE, OK, NO_CHANGE, 500);
}

void findEndpoint() {
  String text = command(COMMAND_LAP, OK, NOTHING, 5000);
  if (!has(text, "La Maldicion de Mandos")) {
    Serial.println("No encontrado Endpoint, está prendido?");
  }
}

void configureEndpoint() {
  String cmd = "";
  cmd+= COMMAND_JAP;
  cmd+= "\"La Maldicion de Mandos\",\"spuenci1\"\r\n";
  command(cmd, OK, NOTHING, 5000);
}

void getIp() {
  String text = command(COMMAND_FSR, OK, NOTHING, 1000);
  ip = findIp(text);
}

void configureMux() {
  command(COMMAND_MUX, OK, NOTHING, 500); 
}  

void configureServer() {
  command(COMMAND_SERVER, OK, NOTHING, 1000);
}

String command(String command, String expected, String expected2, int timeout) {
  String text = send(command, timeout);
  if (!expected2.equals(NOTHING)) {
    if (!has(text, expected) && !has(text, expected2)) {
      Serial.print(command);
      Serial.print(" No responde ni ");
      Serial.print(expected);
      Serial.print(" ni ");
      Serial.print(expected2);
      Serial.println(" :(");
    }
  } else {
    if(!has(text, expected)) {
      Serial.print(command);
      Serial.print(" No responde ");
      Serial.print(expected);
      Serial.println(" :(");
    }
  }
  return text;
}

void listen() {
  if (wifi.find(HTTP_INIT)) {
    processRequest(); 
  } else {
    load(500);
  } 
}

void processRequest() {
  String cip = wifi.readStringUntil(CHAR_COMMA);
  wifi.find(COLON);
  String method = wifi.readStringUntil(CHAR_SPACE);
  String message;
  wifi.find(SLASH);
  if (method.equals(HTTP_PUT)) {
    message = putRequest();
  } else if (method.equals(HTTP_GET)) {
    message = getRequest();
  } else {
    message = HTTP_BAD_RESPONSE;
  }
  load(500);
  int messageSize = message.length();
  wifi.print(COMMAND_SEND);
  wifi.print(cip);
  wifi.print(COMMA);
  wifi.println(messageSize);
  if (wifi.find(RES_WAIT)) {
    wifi.println(message);
    Serial.println("Send:");
    Serial.println(message);
    if(wifi.find("SEND OK")) {
      Serial.println("SEND OK closing");
      wifi.print(COMMAND_CLOSE);
      wifi.println(cip);
    }
  }
}

String putRequest() {
  String command = wifi.readStringUntil(CHAR_SLASH);
  if (String(REQ_LED).equals(command)) {
    return led();
  }  
  return HTTP_BAD_RESPONSE;
}

String getRequest() {
  String command = wifi.readStringUntil(CHAR_SLASH);
  if (String(REQ_PING).equals(command)) {
    return ping();
  }
  if (String(REQ_LEDS).equals(command)) {
    return leds();
  }
  if (String(REQ_STATUS).equals(command)) {
    return ledStatus();
  }
  return HTTP_BAD_RESPONSE;
}

String header(String code, String body) {
  String header = "";
  header+= code;
  header+= String(body.length());
  header+= HTTP_END_HEAD;
  header+= body;
  return header;
}

String headerOk(String body) {
  String header = "";
  header+= HTTP_HEAD;
  header+= String(body.length());
  header+= HTTP_END_HEAD;
  header+= body;
  return header;
}

String headerNoChange(String body) {
  String header = "";
  header+= HTTP_HEAD_NO_CHANGE;
  header+= String(body.length());
  header+= HTTP_END_HEAD;
  header+= body;
  return header;
}

String headerBad() {
  return HTTP_BAD_RESPONSE;
}

String ping() {
  return headerOk(NOTHING);
}

String leds() {
  String response = "[";
  response+= LED;
  response+= "]";
  return headerOk(response); 
}

String ledStatus() {
  String ledString = wifi.readStringUntil(CHAR_SPACE);
  int led = ledString.toInt();
  if (led == LED) {
    String state = OFF;
    if (ledList[led] == HIGH) {
      state = ON;
    }
    return headerOk(state);
  }
  return headerBad();
}

String led() {
  String ledString = wifi.readStringUntil(CHAR_SLASH);
  String comm = wifi.readStringUntil(CHAR_SPACE);
  int led = ledString.toInt();
  if (led == LED) {
    String state = OFF;
    if (ledList[led] == HIGH) {
      state = ON;
    }
    if (state.equals(comm)) {
      return headerNoChange("");
    } else {
      if (ledList[led] == HIGH) {
        digitalWrite(led, LOW);
        ledList[led] = LOW;
      } else {
        digitalWrite(led, HIGH);
        ledList[led] = HIGH;  
      }
    }
    return headerOk("");
  }
  return headerBad();
}

String send(String command, const int timeout) {
  return sendData(command, timeout, true);
}

String sendData(String command, const int timeout, boolean debug) {
    wifi.print(command); // send the read character to the esp8266
    return load(timeout);
}

String load(const int timeout) {
  String text = "";
  long int time = millis();
    while( (time+timeout) > millis())
    {
      while(wifi.available())
      {   
        // The esp has data so display its output to the serial window 
        char c = wifi.read(); // read the next character.
        text+=c;
      }  
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
