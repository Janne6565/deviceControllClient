#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const String PASSWORD = "censored";
const String DEVICE_NAME = "censored";
const int PIN_RELAY = 0;
const int BUTTON_TOGGLE_DELAY = 500;

const int TOGGLE_TIME = 1000;
const int PORT = 80;

// Generic catch-all implementation.
template<typename T_ty> struct TypeInfo { static const char* name; };
template<typename T_ty> const char* TypeInfo<T_ty>::name = "unknown";

// Handy macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handy macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type) template<> const char* TypeInfo<type>::name = #type;

const char* ssid = "censored";
const char* password = "censored";

bool currentlyOn = false;

// Set web server port number to 80
WiFiServer server(PORT);
HTTPClient sender;
WiFiClientSecure wifiClient;

// Variable to store the HTTP request
String header;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

unsigned long lastTimeIpSend = millis();
const long ipSendInterval = 50000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_RELAY, OUTPUT);

  // Connect to Wi-Fi network with SSID and password
  Serial.println("Connecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: " + WiFi.localIP().toString());
  sendIp();
  server.begin();
  Serial.println("Server Online on: " + WiFi.localIP().toString() + ":" + PORT);
}

void sendIp() {
  String response = sendGetRequest("https://valorantcrazyclips69.com/ipRegistry/set/" + DEVICE_NAME + "?ip=http://" + WiFi.localIP().toString() + "&secret=" + PASSWORD);
  Serial.println("Registered ip address: " + response);
}

String sendGetRequest(String url) {
  Serial.println("Sending GET request to url: " + url);
  wifiClient.setInsecure();
  if (sender.begin(wifiClient, url)) {
    int httpCode = sender.GET();
    if (httpCode > 0) {

      // Anfrage wurde gesendet und Server hat geantwortet
      // Info: Der HTTP-Code für 'OK' ist 200
      if (httpCode == HTTP_CODE_OK) {
        // Hier wurden die Daten vom Server empfangen
        // String vom Webseiteninhalt speichern
        String payload = sender.getString();
        // Hier kann mit dem Wert weitergearbeitet werden
        // ist aber nicht unbedingt notwendig
        return payload;
      }
    } else {
      // Falls HTTP-Error
      Serial.printf("HTTP-Error: ", sender.errorToString(httpCode).c_str());
    }
  }
  return "";
}

void checkForIpResend() {
  if (lastTimeIpSend + ipSendInterval < millis()) {
    lastTimeIpSend = millis();
    sendIp();
  }
}


void loop() {
  checkForIpResend();
  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {                     // If a new client connects,
    Serial.println("New Client.");  // print a message out in the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: text/html");
            client.println("Connection: close");
            client.println();
            process(header, client);

            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.println();
    client.stop();
    delay(2);
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}


void process(String message, WiFiClient client) {
  String expectingString = "POST /" + PASSWORD + "/";
  String stringExisting = message.substring(0, expectingString.length());
  Serial.print("Received: ");
  Serial.println(stringExisting);
  if (expectingString != stringExisting) {
    client.println("401");
    return;
  }

  int lengthFirstLine = message.indexOf("\n");
  if (lengthFirstLine == -1) {
    return;
  }
  String path = message.substring(expectingString.length() - 1, lengthFirstLine - 10);

  if (path == "/toggle") {
    if (currentlyOn) {
      turnOff();
      currentlyOn = false;
      client.println("0");
    } else {
      turnOn();
      currentlyOn = true;
      client.println("1");
    }
  } else if (path == "/start") {
    currentlyOn = true;
    turnOn();
    client.println("1");
  } else if (path == "/stop") {
    currentlyOn = false;
    turnOff();
    client.println("0");
  } else if (path == "/status") {
    client.println(currentlyOn);
  } else {
    client.println("use /start, /stop or /toggle");
  }

  Serial.println(path);
  Serial.print("Current setting: ");
  Serial.println(currentlyOn);
}

void turnOn() {
  digitalWrite(PIN_RELAY, HIGH);
}

void turnOff() {
  digitalWrite(PIN_RELAY, LOW);
}