/*
for ESP8266 or ESP32
ResetPin - a pin for setting, when it is connected to the ground, the board goes into WiFI AP mode,
connect to it and go through the browser to set the SSID and pasword of the network.
They are saved to files in flash memory.
Add lib AsyncWebServer
*/

#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <SPIFFS.h>
#else
  #include <ESP8266WiFi.h>
  #include <FS.h>
#endif

#include <ESPAsyncWebServer.h>

// Pin for activate WiFi AP mode, start webserver for WiFi configuration
  #ifdef ESP32
    #define ResetPin 23
  #else
    #define ResetPin D0
  #endif

//------------------------------------------------------------------------------------------------------------------------------------------------

// Web server on default port
AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* PARAM_SSID = "inputssid";
const char* PARAM_PASS = "inputpass";

// HTML web page to handle 2 input fields (inputssid, inputpass)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>AsyncServer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" charset="utf-8">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    inputssid (current value %inputssid%): <input type="text" name="inputssid">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputpass (current value %inputpass%): <input type="text" name="inputpass">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String& var){
  Serial.println(var);
  if(var == "inputssid"){
    return readFile(SPIFFS, "/inputssid.txt");
  } else if(var == "inputpass"){
    return readFile(SPIFFS, "/inputpass.txt");
  }
  return String();
}

void setup() {
  // Initialize pins
  pinMode(ResetPin, INPUT_PULLUP);

  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif

  Serial.begin(9600);

  delay(3000); // wait for console opening
  Serial.println();

  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif
  
  // WIFI_AP / WIFI_STA
  if (digitalRead(ResetPin) == LOW){
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      Serial.print("Setting soft-AP ... ");
      Serial.println(WiFi.softAP("Setup_", "") ? "Ready" : "Failed!");

      // Send web page with input fields to client
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
      });

      // Send a GET request to <ESP_IP>/get?inputssid=<inputMessage>
      server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      // GET inputssid value on <ESP_IP>/get?inputssid=<inputMessage>
      if (request->hasParam(PARAM_SSID)) {
        inputMessage = request->getParam(PARAM_SSID)->value();
        writeFile(SPIFFS, "/inputssid.txt", inputMessage.c_str());
      }
      // GET inputpass value on <ESP_IP>/get?inputpass=<inputMessage>
      else if (request->hasParam(PARAM_PASS)) {
        inputMessage = request->getParam(PARAM_PASS)->value();
        writeFile(SPIFFS, "/inputpass.txt", inputMessage.c_str());
      }
      else {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/text", inputMessage);
    });
    server.onNotFound(notFound);
    // Start Webserver
    server.begin();
    } else {
      Serial.print("Connecting to: "); 
      String PARAM_SSID_ = readFile(SPIFFS, "/inputssid.txt");
      const char* PARAM_SSID__ = PARAM_SSID_.c_str();
      Serial.println(PARAM_SSID__);
      String PARAM_PASS_ = readFile(SPIFFS, "/inputpass.txt");
      const char* PARAM_PASS__ = PARAM_PASS_.c_str();
    
      WiFi.mode(WIFI_STA);
      WiFi.begin(PARAM_SSID__, PARAM_PASS__);
      while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("Wi-Fi connected");  
      Serial.println("IP-address: "); 
      Serial.println(WiFi.localIP());
    }
}
 
void loop() {

}
