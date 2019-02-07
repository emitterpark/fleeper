#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include <jled.h>

String ssid, password, fleep, place, button;
String webhookPayload;
bool configMode, opResult, serverHandled;
String opStatus = "init";
String buttonPressed;
unsigned long inTime = 0;
const unsigned long timeOut20 = 20000;
const unsigned long timeOut60 = 60000;

#define BUTTON 12
#define RED_LED 2
//#define BLUE_LED 
//#define GREEN_LED 

EasyButton but(BUTTON, 25, true, true); //pull up enabled, active low
// redLed = JLed(RED_LED).Blink(100, 300).Forever().LowActive();
JLed redLed = JLed(RED_LED).On().LowActive();
//JLed blueLed = JLed(BLUE_LED).Off().LowActive();
//JLed greenLed = JLed(GREEN_LED).Off().LowActive();
ESP8266WebServer server(80);
BearSSL::WiFiClientSecure clients;
HTTPClient webhook;

void setup() {  
  Serial.begin(115200);
  Serial.println("hello");     
  but.begin();
  but.onPressed(onPressed);
  but.onPressedFor(5000, onPressedLong); 
  SPIFFS.begin();  
  clients.setInsecure();   
  configSet();  
}
void wifiConnect() {  
  opStatus = "wifi";
  inTime = millis();
  loadConfig();
  char buf1[30];
  ssid.toCharArray(buf1, sizeof(buf1));
  char buf2[30];
  password.toCharArray(buf2, sizeof(buf2));      
  WiFi.begin(buf1, buf2);  
}
void webhookBegin() {  
  opStatus = "webhook";
  webhook.begin(clients, fleep);    
  webhook.addHeader("Content-Type", "application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["message"] = place;
  json["user"] = button;
  webhookPayload = "";
  json.prettyPrintTo(webhookPayload);
}
void webhookSend() {   
  if (webhook.POST(webhookPayload) == HTTP_CODE_OK) {      
      opResult = true;
      exitMe(); 
  }
}
void configSet() {  
  configMode = true;
  inTime = millis();    
  server.on("/configs", handleConfigs);  
  server.serveStatic("/", SPIFFS, "/index.html");
  server.begin();      
  JLed redLed = JLed(RED_LED).On().LowActive();
  //JLed blueLed = JLed(BLUE_LED).Off().LowActive();
  //JLed greenLed = JLed(GREEN_LED).Off().LowActive(); 
}
void handleConfigs() {
  inTime = millis();
  serverHandled = true;  
  if (server.hasArg("plain") == true) {        
    if (server.hasArg("test")) {      
      wifiConnect();
    } else {      
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["ssid"] = server.arg("ssid");
      json["password"] = server.arg("password");
      json["fleep"] = server.arg("fleep");
      json["place"] = server.arg("place");
      json["button"] = server.arg("button");      
      File configFile = SPIFFS.open("/config.json", "w");
      json.printTo(configFile);    
      configFile.close();
      loadConfig();
    }      
  } else {    
    loadConfig();
  }    
}
void onPressed() {
  /*
  Serial.println("pressed!");  
  buttonPressed = "pressed";
  */
}
void onPressedLong() {
  /*
  Serial.println("pressed long!");
  buttonPressed = "pressedlong";
  */
}
void loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  size_t size = configFile.size();  
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());  
  json.printTo(Serial);    
  ssid = (const char*)json["ssid"];
  password = (const char*)json["password"];
  fleep = (const char*)json["fleep"];
  place = (const char*)json["place"];
  button = (const char*)json["button"];
  configFile.close();   
  if (serverHandled) {    
    serverHandled = false;
    String jsonString;
    json.prettyPrintTo(jsonString);
    server.send(200, "application/json", jsonString);    
  }  
}
void exitMe() {
  inTime = millis();
  opStatus = "exit"; 
  webhook.end();
  clients.stop();
  if (opResult) {
    Serial.print("result true");
    JLed redLed = JLed(RED_LED).Off().LowActive();
  //JLed blueLed = JLed(BLUE_LED).Off().LowActive();
  //JLed greenLed = JLed(GREEN_LED).Blink(100,300).Forever().LowActive();
    
  } else {
    Serial.print("result false");
    JLed redLed = JLed(RED_LED).Blink(100,300).Forever().LowActive();
  //JLed blueLed = JLed(BLUE_LED).Off().LowActive();
  //JLed greenLed = JLed(GREEN_LED).Off().LowActive();    
  }   
  Serial.println("exit me "+opResult);  
} 
void shutDown() {
  Serial.println("shut down");  
} 
void loop() { 
  if (buttonPressed == "pressed") {
    buttonPressed = "";
    if (opStatus == "init") {
      wifiConnect();                  
    } else if (opStatus == "exit") {
      shutDown();
    }  
  } else if (buttonPressed == "pressedlong") {
    buttonPressed = "";
    if (opStatus == "init") {             
      configSet(); 
    } else if (opStatus == "exit") {
      shutDown();                 
    }    
  }      
  if (opStatus == "wifi") {
    if (WiFi.status() == WL_CONNECTED) {
      webhookBegin();
    }      
  } else if (opStatus == "webhook") {    
    if (!webhook.connected()) {
      webhookSend(); 
    } 
  }        
  unsigned long currentTime = millis();
  if (configMode) {        
    if (currentTime - inTime > timeOut60) {
      opResult = false;
      exitMe();          
    }
    server.handleClient(); 
  } else if (currentTime - inTime > timeOut20) {
    opResult = false;
    exitMe();      
  } 
  but.read();
  redLed.Update();
  //blueLed.update();
  //greenLed.update();  
  yield();     
}  


