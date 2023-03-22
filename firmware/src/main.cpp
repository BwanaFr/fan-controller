/**
 *  Simple dual PWM FAN controller with two Dallas temperature
 *  sensor (inlet, outlet)
**/


#include <Arduino.h>

#include "inout.h"
#include "pin_config.h"

//ESPEasyCfg includes
#include <ESPEasyCfg.h>

//MQTT includes
#include <PubSubClient.h>
#include <ArduinoJson.h>

//ESPEasyCfg objects
AsyncWebServer server(80);
ESPEasyCfg captivePortal(&server, "Fan");
ESPEasyCfgParameterGroup mqttParamGrp("MQTT");
ESPEasyCfgParameter<String> mqttServer(mqttParamGrp, "mqttServer", "MQTT server", "server.local");
ESPEasyCfgParameter<String> mqttUser(mqttParamGrp, "mqttUser", "MQTT user", "user");
ESPEasyCfgParameter<String> mqttPass(mqttParamGrp, "mqttPass", "MQTT password", "");
ESPEasyCfgParameter<int> mqttPort(mqttParamGrp, "mqttPort", "MQTT port", 1883);
ESPEasyCfgParameter<String> mqttName(mqttParamGrp, "mqttName", "MQTT name", "Fan", "", "{\"required\":\"\"}");

ESPEasyCfgParameterGroup fanParamGrp("FAN");
ESPEasyCfgEnumParameter operMode(fanParamGrp, "opMode", "Operating mode", "Off;Auto;Forced");
ESPEasyCfgParameter<double> onTemperature(fanParamGrp, "onTemperature", "Automatic on temperature [C]", 30.0);
ESPEasyCfgParameter<double> offTemperature(fanParamGrp, "offTemperature", "Automatic off temperature [C]", 20.0);
ESPEasyCfgParameter<double> fanPercent(fanParamGrp, "fanPercent", "Fan percentage [%]", 100.0, "{\"min\":\"0\", \"max\":\"100\"}");
ESPEasyCfgParameter<double> fan1Offset(fanParamGrp, "fan1Offset", "Fan 1 offset [%]", 0.0, "{\"min\":\"0\", \"max\":\"100\"}");
ESPEasyCfgParameter<double> fan2Offset(fanParamGrp, "fan2Offset", "Fan 2 offset [%]", 0.0, "{\"min\":\"0\", \"max\":\"100\"}");

//MQTT objects
WiFiClient espClient;                                   // TCP client
PubSubClient client(espClient);                         // MQTT object
const unsigned long mqttPostingInterval = 10L * 1000L;  // Delay between updates, in milliseconds
static unsigned long mqttLastPostTime = 0;              // Last time you sent to the server, in milliseconds
String mqttStatusService;                               // Status publishing service.
String mqttFanService;                                  // Fan 1 MQTT service prefix

uint32_t lastMQTTConAttempt = 0;                        // Last MQTT connection attempt
enum class MQTTConState {Connecting, Connected, Disconnected, NotUsed};
MQTTConState mqttState = MQTTConState::Disconnected;

//Process variables
enum class OperatingMode {Off /*No operating mode */, Auto /* Automatic mode */, Forced /* Forced mode */};
OperatingMode operatingMode = OperatingMode::Off;         // Fan operating mode
static unsigned long lastParameterChange = 0;             // Last time one saved parameter was changed (to delay the saving)
const unsigned long delayedParameterSaving = 10L * 1000L; // Delay before saving parameters to flash
bool fansOn = false;                                      // Are fans on?
double temperature1 = 0.0;                                // Temperature 1 sensor value
double temperature2 = 0.0;                                // Temperature 2 sensor value
bool autoOnReached = false;                               // Threshold reached in auto mode
/**
 * Callback to get captive-portal messages
*/
void captive_portal_message(const char* msg, ESPEasyCfgMessageType type) {
  Serial.println(msg);
}

/**
 * Change current working mode
 * @param newMode New mode to be set
 * @return true on success
*/
bool changeMode(const String& newMode) {
  bool ret = false;
  if(newMode == "Off"){
    operatingMode = OperatingMode::Off;
    autoOnReached = false;
    ret = true;
  }else if(newMode == "Auto"){
    operatingMode = OperatingMode::Auto;
    ret = true;
  }else if(newMode == "Forced"){
    operatingMode = OperatingMode::Forced;
    autoOnReached = false;
    ret = true;
  }
  if(ret){
    if(operMode.toString() != newMode){
      operMode.setValue(newMode.c_str());
        //Delay saving to flash
        lastParameterChange = millis();
    }
  }
  return ret;
}

/**
 * Callback of MQTT
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String data;
  data.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  String strTopic(topic);
  bool saveSettings = false;
  if(strTopic == (mqttFanService + "/mode")){
    changeMode(data);
  }else if(strTopic == (mqttFanService + "/percentage")){
    fanPercent.setValue(data.toDouble());
    saveSettings = true;
  }else if(strTopic == (mqttFanService + "/auto_off_temp")){
    offTemperature.setValue(data.toDouble());
    saveSettings = true;
  }else if(strTopic == (mqttFanService + "/auto_on_temp")){
    onTemperature.setValue(data.toDouble());
    saveSettings = true;
  }else if(strTopic == (mqttFanService + "/set")){
    if(operatingMode != OperatingMode::Auto){
      if(data == "on" || data == "true"){
        changeMode("Forced");
      }else{
        changeMode("Off");        
      }
    }
  }

  if(saveSettings){
    //Delay saving to flash
    lastParameterChange = millis();
  }
  mqttLastPostTime = 0;
}

/**
 * Performs setup of MQTT server
*/
void mqtt_setup(){
  if(!mqttServer.getValue().isEmpty()){
      //Build MQTT service names
    mqttStatusService =  mqttName.getValue() +  "/status";
    mqttFanService = mqttName.getValue();
    
    //Setup MQTT client callbacks and port
    client.setServer(mqttServer.getValue().c_str(), mqttPort.getValue());
    client.setCallback(mqtt_callback);
    mqttState = MQTTConState::Connecting;
  }else{
    mqttState = MQTTConState::NotUsed;
  }
}

/**
 * Call back on parameter change from captive portal
 */
void new_cp_state(ESPEasyCfgState state) {
  if(state == ESPEasyCfgState::Reconfigured){
    changeMode(operMode.toString());
    //Don't use MQTT if server is not filled
    if(mqttServer.getValue().isEmpty()){
      mqttState = MQTTConState::NotUsed;
    }else{
      mqtt_setup();
      mqttLastPostTime = 0;
      client.disconnect();
    }    
  }
}

/**
 * Performs setup of captive portal
*/
void captive_portal_setup() {
 //Configure captive portal
  //The MQTT password is a password HTML type
  mqttPass.setInputType("password");
  //Finally, add our parameter group to the captive portal
  captivePortal.addParameterGroup(&mqttParamGrp);
  captivePortal.addParameterGroup(&fanParamGrp);
  // captivePortal.setLedPin(PIN_USER_LED);
  captivePortal.setMessageHandler(captive_portal_message);
  captivePortal.setStateHandler(new_cp_state);
  //Start our captive portal (if not configured)
  //At first usage, you will find a new WiFi network named "Heater"
  captivePortal.begin();
  //Serve web pages
  server.begin();
}

void mqtt_reconnect() {
  //Don't use MQTT if server is not filled
  if(mqttServer.getValue().isEmpty()){
    return;
  }
  // Loop until we're reconnected
  if (!client.connected() && ((millis()-lastMQTTConAttempt)>5000)) {   
    mqttState = MQTTConState::Connecting;
    IPAddress mqttServerIP;
    int ret = WiFi.hostByName(mqttServer.getValue().c_str(), mqttServerIP);
    if(ret != 1){
      Serial.print("Unable to resolve hostname: ");
      Serial.print(mqttServer.getValue().c_str());
      Serial.println(" try again in 5 seconds");
      lastMQTTConAttempt = millis();
      return;
    }
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqttServer.getValue().c_str());
    Serial.print(':');
    Serial.print(mqttPort.getValue());
    Serial.print('(');
    Serial.print(mqttServerIP);
    Serial.print(")...");
    // Create a Client ID baased on MAC address
    byte mac[6];                     // the MAC address of your Wifi shield
    WiFi.macAddress(mac);
    String clientId = "FanController-";
    clientId += String(mac[3], HEX);
    clientId += String(mac[4], HEX);
    clientId += String(mac[5], HEX);
    // Attempt to connect
    client.setServer(mqttServerIP, mqttPort.getValue());
    if((ret == 1) && (client.connect(clientId.c_str(), mqttUser.getValue().c_str(), mqttPass.getValue().c_str()))) {
      Serial.println("connected");
      mqttState = MQTTConState::Connected;
      //Subscribe to MQTT topics
      client.subscribe((mqttFanService + "/mode").c_str());
      client.subscribe((mqttFanService + "/percentage").c_str());
      client.subscribe((mqttFanService + "/set").c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      client.disconnect();
      mqttState = MQTTConState::Disconnected;
      lastMQTTConAttempt = millis();
    }
  }
}

/**
 * This method publishes acquisition values over MQTT
 * Values are :
 * - Fan speed
 * - Temperature 1 sensor value
 * - Temperature 2 sensor value
 * - System operating mode (AUTO, FORCED, DISABLED)
*/
void publishValuesToMQTT(){
  //Publish to MQTT clients
  if(client.connected()){
    String msg;
    StaticJsonDocument<210> root;
    root["percentage"] = fanPercent.getValue();
    root["temperature1"] = temperature1;
    root["temperature2"] = temperature2;
    root["offTemperature"] = offTemperature.getValue();
    root["onTemperature"] = onTemperature.getValue();
    root["mode"] = operMode.toString();
    root["state"] = fansOn ? "on" : "off";
    serializeJson(root, msg);
    client.publish(mqttStatusService.c_str(), msg.c_str());
  }
}

/**
 * Main process for fan
*/
void fan_process() {
  temperature1 = getTemperature1();
  temperature2 = getTemperature2();

  if(getForcedInput() && (operatingMode != OperatingMode::Forced)){
    //User switched to forced mode by using button
    changeMode("Forced");
  }else if(getOffInput() && (operatingMode != OperatingMode::Off)){
    //User switched to off mode by using button
    changeMode("Off");
  }

  switch (operatingMode)
  {
  case OperatingMode::Off:
    fansOn = false;
    break;
  case OperatingMode::Auto:
    //Automatic mode, do hysteresis
    if(temperature1 >= onTemperature.getValue()){
      autoOnReached = true;
    }else if(autoOnReached && (temperature1 < offTemperature.getValue())){
      autoOnReached = false;
    }
    fansOn = autoOnReached;
    break;
  case OperatingMode::Forced:
    fansOn = true;
    break;  
  default:
    break;
  }
  if(fansOn){
    double percentFan1 = fanPercent.getValue() + fan1Offset.getValue();
    if(percentFan1 > 100.0){
      percentFan1 = 100.0;
    }else if(percentFan1 < 0.0){
      percentFan1 = 0.0;
    }
    double percentFan2 = fanPercent.getValue() + fan2Offset.getValue();
    if(percentFan2 > 100.0){
      percentFan2 = 100.0;
    }else if(percentFan2 < 0.0){
      percentFan2 = 0.0;
    }
    set_fan_speed(1, percentFan1);
    set_fan_speed(2, percentFan2);
  }else{
    set_fan_speed(1, 0.0);
    set_fan_speed(2, 0.0);
  }
}

/**
 * Arduino Setup method
*/
void setup() {
  Serial.begin(115200);
  Serial.println("Starting awesome fan controller!");
  //Setup I/O
  setup_inputs_outputs();
  //Initialize captive portal
  captive_portal_setup();
  //Initialize MQTT
  mqtt_setup();
  //Restore the previous mode
  changeMode(operMode.toString());
}

void loop() {
  unsigned long now = millis();
  bool publishToMQTT = false;
  
  //MQTT loop
  if (mqttState != MQTTConState::NotUsed){
    publishToMQTT = client.loop();
    if(!publishToMQTT) {
      //Not connected of problem with updates
      mqtt_reconnect();
    }
  }
  
  // Call process
  fan_process();

  //Publish to MQTT if it's time to do
  if(publishToMQTT && ((now-mqttLastPostTime)>mqttPostingInterval)){
    publishValuesToMQTT();
    mqttLastPostTime = now;
  }

  //Delayed parameter saving
  if((lastParameterChange != 0) && ((now-lastParameterChange)>delayedParameterSaving)){
    lastParameterChange = 0;
    captivePortal.saveParameters();
  }

  yield();
}