# 1 "/var/folders/nx/6sk5l7c120nftbm1k3sz87z00000gn/T/tmpqrm4lX"
#include <Arduino.h>
# 1 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
#include "definitions.h"
#include "version.h"



#include <ESP8266WiFi.h>


#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <EEPROM.h>

#include <WebSockets.h>
#include <WebSocketsServer.h>


#if defined(ENABLE_OTA) or defined(ENABLE_BUTTONS)
  #include <WiFiUdp.h>
#endif

#ifdef ENABLE_OTA
  #include <ArduinoOTA.h>
#endif

#ifdef ENABLE_BUTTONS
  #include <Bounce2.h>
#endif


#if !defined(ENABLE_HOMEASSISTANT) and defined(ENABLE_STATE_SAVE_SPIFFS)
  #include <ArduinoJson.h>
#endif


#ifdef ENABLE_MQTT
  #include <PubSubClient.h>
  #ifdef ENABLE_HOMEASSISTANT
    #include <ArduinoJson.h>
  #endif

  WiFiClient espClient;
  PubSubClient mqtt_client(espClient);
#endif

#ifdef ENABLE_AMQTT
  #include <AsyncMqttClient.h>

  #ifdef ENABLE_HOMEASSISTANT
    #include <ArduinoJson.h>
  #endif

  AsyncMqttClient amqttClient;
  WiFiEventHandler wifiConnectHandler;
  WiFiEventHandler wifiDisconnectHandler;
#endif

#ifdef ARDUINOJSON_VERSION
  #if !(ARDUINOJSON_VERSION_MAJOR == 6 and ARDUINOJSON_VERSION_MINOR >= 8)
    #error "Install ArduinoJson v6.8.0-beta or higher"
  #endif
#endif

#ifdef ENABLE_E131
  #include <ESPAsyncUDP.h>
  #include <ESPAsyncE131.h>
  ESPAsyncE131 e131(END_UNIVERSE - START_UNIVERSE + 1);
#endif

#ifdef USE_HTML_MIN_GZ
#include "html_gz.h"
#endif

#ifdef ENABLE_BUTTONS
  WiFiUDP Udp;
  Bounce debouncer1 = Bounce();
  Bounce debouncer2 = Bounce();
#endif




ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#ifdef HTTP_OTA
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater;
#endif





#include <WS2812FX.h>

WS2812FX* strip;

#if defined(USE_WS2812FX_DMA) or defined(USE_WS2812FX_UART1) or defined(USE_WS2812FX_UART2)
#include <NeoPixelBus.h>

#ifdef USE_WS2812FX_DMA
  #ifndef LED_TYPE_WS2811
    NeoEsp8266Dma800KbpsMethod* dma;
  #else
    NeoEsp8266Dma400KbpsMethod* dma;
  #endif
#endif
#ifdef USE_WS2812FX_UART1
  #ifndef LED_TYPE_WS2811
    NeoEsp8266Uart1800KbpsMethod* dma;
  #else
    NeoEsp8266Uart1400KbpsMethod* dma;
  #endif
#endif
#ifdef USE_WS2812FX_UART2
  #ifndef LED_TYPE_WS2811
    NeoEsp8266Uart0800KbpsMethod* dma;
  #else
    NeoEsp8266Uart0400KbpsMethod* dma;
  #endif
#endif

void initDMA(uint16_t stripSize = NUMLEDS){
  if (dma) delete dma;
#ifdef USE_WS2812FX_DMA
  #ifndef LED_TYPE_WS2811
    dma = new NeoEsp8266Dma800KbpsMethod(stripSize, 3);
  #else
    dma = new NeoEsp8266Dma400KbpsMethod(stripSize, 3);
  #endif
#endif
#ifdef USE_WS2812FX_UART1
  #ifndef LED_TYPE_WS2811
    dma = new NeoEsp8266Uart1800KbpsMethod(stripSize, 3);
  #else
    dma = new NeoEsp8266Uart1400KbpsMethod(stripSize, 3);
  #endif
#endif
#ifdef USE_WS2812FX_UART2
  #ifndef LED_TYPE_WS2811
    dma = new NeoEsp8266Uart0800KbpsMethod(stripSize, 3);
  #else
    dma = new NeoEsp8266Uart0400KbpsMethod(stripSize, 3);
  #endif
#endif
  dma->Initialize();
}
void DMA_Show(void);
void tick();
String getValue(String data, char separator, int index);
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback ();
void setup();
void loop();
#line 154 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
void DMA_Show(void) {
  if(dma->IsReadyToUpdate()) {
    memcpy(dma->getPixels(), strip->getPixels(), dma->getPixelsSize());
    dma->Update();
  }
}
#endif




#include <Ticker.h>
Ticker ticker;
#ifdef ENABLE_HOMEASSISTANT
  Ticker ha_send_data;
#endif
#ifdef ENABLE_AMQTT
  Ticker mqttReconnectTimer;
  Ticker wifiReconnectTimer;
#endif
#ifdef ENABLE_STATE_SAVE_SPIFFS
  Ticker spiffs_save_state;
#endif
void tick()
{

  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

#ifdef ENABLE_STATE_SAVE_EEPROM



  String readEEPROM(int offset, int len) {
    String res = "";
    for (int i = 0; i < len; ++i)
    {
      res += char(EEPROM.read(i + offset));

    }
    DBG_OUTPUT_PORT.printf("readEEPROM(): %s\n", res.c_str());
    return res;
  }

  void writeEEPROM(int offset, int len, String value) {
    DBG_OUTPUT_PORT.printf("writeEEPROM(): %s\n", value.c_str());
    for (int i = 0; i < len; ++i)
    {
      if (i < value.length()) {
        EEPROM.write(i + offset, value[i]);
      } else {
        EEPROM.write(i + offset, NULL);
      }
    }
  }
#endif





String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}





void configModeCallback (WiFiManager *myWiFiManager) {
  DBG_OUTPUT_PORT.println("Entered config mode");
  DBG_OUTPUT_PORT.println(WiFi.softAPIP());

  DBG_OUTPUT_PORT.println(myWiFiManager->getConfigPortalSSID());

  ticker.attach(0.2, tick);

  uint16_t i;
  for (i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, 0, 0, 255);
  }
  strip->show();
}


void saveConfigCallback () {
  DBG_OUTPUT_PORT.println("Should save config");
  shouldSaveConfig = true;
}




#include "spiffs_webserver.h"




#include "request_handlers.h"

#ifdef CUSTOM_WS2812FX_ANIMATIONS
  #include "custom_ws2812fx_animations.h"
#endif


void initStrip(uint16_t stripSize = WS2812FXStripSettings.stripSize, neoPixelType RGBOrder = WS2812FXStripSettings.RGBOrder, uint8_t pin = WS2812FXStripSettings.pin){
  if (strip) {
    delete strip;
    WS2812FXStripSettings.stripSize = stripSize;
    WS2812FXStripSettings.RGBOrder = RGBOrder;
    WS2812FXStripSettings.pin = pin;
  }
  #ifndef LED_TYPE_WS2811
    strip = new WS2812FX(stripSize, pin, RGBOrder + NEO_KHZ800);
  #else
    strip = new WS2812FX(stripSize, pin, RGBOrder + NEO_KHZ400);
  #endif
# 298 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
  strip->init();
  #if defined(USE_WS2812FX_DMA) or defined(USE_WS2812FX_UART1) or defined(USE_WS2812FX_UART2)
    initDMA(stripSize);
    strip->setCustomShow(DMA_Show);
  #endif
  strip->setBrightness(brightness);
  strip->setSpeed(convertSpeed(ws2812fx_speed));

  strip->setColor(main_color.red, main_color.green, main_color.blue);
  strip->setOptions(0, GAMMA);
#ifdef CUSTOM_WS2812FX_ANIMATIONS
  strip->setCustomMode(0, F("Fire 2012"), myCustomEffect);
#endif
  strip->start();
  if(mode != HOLD) mode = SET_MODE;
  saveWS2812FXStripSettings.once(3, writeStripConfigFS);
}




#ifdef ENABLE_LEGACY_ANIMATIONS
  #include "colormodes.h"
#endif




void setup() {


  DBG_OUTPUT_PORT.begin(115200);
  EEPROM.begin(512);


  pinMode(BUILTIN_LED, OUTPUT);

#ifdef ENABLE_BUTTONS
  debouncer1.attach(BUTTON_1, INPUT_PULLUP);
  debouncer1.interval(25);

  debouncer2.attach(BUTTON_2, INPUT_PULLUP);
  debouncer2.interval(25);
#endif

  ticker.attach(0.5, tick);




  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }

    FSInfo fs_info;
    SPIFFS.info(fs_info);
    DBG_OUTPUT_PORT.printf("FS Usage: %d/%d bytes\n\n", fs_info.usedBytes, fs_info.totalBytes);
  }

  wifi_station_set_hostname(const_cast<char*>(HOSTNAME));




  readStripConfigFS();
  initStrip();







  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT) or defined(ENABLE_BUTTONS)
    #if defined(ENABLE_STATE_SAVE_SPIFFS)
      (readConfigFS()) ? DBG_OUTPUT_PORT.println("WiFiManager config FS Read success!"): DBG_OUTPUT_PORT.println("WiFiManager config FS Read failure!");
    #else
      String settings_available = readEEPROM(204, 1);
      if (settings_available == "1") {
        #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
          readEEPROM(0, 64).toCharArray(mqtt_host, 64);
          readEEPROM(64, 6).toCharArray(mqtt_port, 6);
          readEEPROM(70, 32).toCharArray(mqtt_user, 32);
          readEEPROM(102, 32).toCharArray(mqtt_pass, 32);
          DBG_OUTPUT_PORT.printf("MQTT host: %s\n", mqtt_host);
          DBG_OUTPUT_PORT.printf("MQTT port: %s\n", mqtt_port);
          DBG_OUTPUT_PORT.printf("MQTT user: %s\n", mqtt_user);
          DBG_OUTPUT_PORT.printf("MQTT pass: %s\n", mqtt_pass);
        #endif

        #if defined(ENABLE_BUTTONS)
          readEEPROM(134, 64).toCharArray(udp_host, 64);
          readEEPROM(198, 6).toCharArray(udp_port, 6);
          DBG_OUTPUT_PORT.printf("UDP host: %s\n", udp_host);
          DBG_OUTPUT_PORT.printf("UDP port: %s\n", udp_port);
        #endif
      }
    #endif
  #endif

  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
    WiFiManagerParameter custom_mqtt_host("host", "MQTT hostname", mqtt_host, 64);
    WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 32, " maxlength=31");
    WiFiManagerParameter custom_mqtt_pass("pass", "MQTT pass", mqtt_pass, 32, " maxlength=31 type='password'");
  #endif

  #ifdef ENABLE_BUTTONS
    WiFiManagerParameter custom_udp_host("udp_target_ip", "UDP Receiver", udp_host, 64);
    WiFiManagerParameter custom_udp_port("udp_target_port", "UDP Port", udp_port, 5);
  #endif


  WiFiManager wifiManager;




  wifiManager.setAPCallback(configModeCallback);


  wifiManager.setSaveConfigCallback(saveConfigCallback);

  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)

    wifiManager.addParameter(&custom_mqtt_host);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
  #endif

  #ifdef ENABLE_BUTTONS
    wifiManager.addParameter(&custom_udp_host);
    wifiManager.addParameter(&custom_udp_port);
  #endif

  WiFi.setSleepMode(WIFI_NONE_SLEEP);



  #ifdef WIFIMGR_PORTAL_TIMEOUT
  wifiManager.setConfigPortalTimeout(WIFIMGR_PORTAL_TIMEOUT);
  #endif



  #ifdef WIFIMGR_SET_MANUAL_IP
  wifiManager.setSTAStaticIPConfig(IPAddress(_ip[0], _ip[1], _ip[2], _ip[3]), IPAddress(_gw[0], _gw[1], _gw[2], _gw[3]), IPAddress(_sn[0], _sn[1], _sn[2], _sn[3]));
  #endif





  if (!wifiManager.autoConnect(HOSTNAME)) {
    DBG_OUTPUT_PORT.println("failed to connect and hit timeout");

    ESP.reset();
    delay(1000);
  }

  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT) or defined(ENABLE_BUTTONS)

    #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
      strcpy(mqtt_host, custom_mqtt_host.getValue());
      strcpy(mqtt_port, custom_mqtt_port.getValue());
      strcpy(mqtt_user, custom_mqtt_user.getValue());
      strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    #endif

    #ifdef ENABLE_BUTTONS
      strcpy(udp_host, custom_udp_host.getValue());
      strcpy(udp_port, custom_udp_port.getValue());

      DBG_OUTPUT_PORT.print("UDP event target (buttons) = ");
      DBG_OUTPUT_PORT.print(udp_host);
      DBG_OUTPUT_PORT.print(":");
      DBG_OUTPUT_PORT.println(udp_port);
    #endif


    #if defined(ENABLE_STATE_SAVE_SPIFFS)
      (writeConfigFS(shouldSaveConfig)) ? DBG_OUTPUT_PORT.println("WiFiManager config FS Save success!"): DBG_OUTPUT_PORT.println("WiFiManager config FS Save failure!");
    #else if defined(ENABLE_STATE_SAVE_EEPROM)
      if (shouldSaveConfig) {
        DBG_OUTPUT_PORT.println("Saving WiFiManager config");

        #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
          writeEEPROM(0, 64, mqtt_host);
          writeEEPROM(64, 6, mqtt_port);
          writeEEPROM(70, 32, mqtt_user);
          writeEEPROM(102, 32, mqtt_pass);
        #endif

        #if defined(ENABLE_BUTTONS)
          writeEEPROM(134, 64, udp_host);
          writeEEPROM(198, 6, udp_port);
        #endif

        writeEEPROM(204, 1, "1");
        EEPROM.commit();
      }
    #endif
  #endif

  #ifdef ENABLE_AMQTT
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  #endif

  initStrip();


  DBG_OUTPUT_PORT.println("connected...yeey :)");
  ticker.detach();

  digitalWrite(BUILTIN_LED, LOW);





  #ifdef ENABLE_OTA
    DBG_OUTPUT_PORT.println("Arduino OTA activated.");


    ArduinoOTA.setPort(8266);


    ArduinoOTA.setHostname(HOSTNAME);
# 541 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
    ArduinoOTA.onStart([]() {
      DBG_OUTPUT_PORT.println("Arduino OTA: Start updating");
    });
    ArduinoOTA.onEnd([]() {
      DBG_OUTPUT_PORT.println("Arduino OTA: End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      DBG_OUTPUT_PORT.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      DBG_OUTPUT_PORT.printf("Arduino OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Auth Failed");
      else if (error == OTA_BEGIN_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Begin Failed");
      else if (error == OTA_CONNECT_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Receive Failed");
      else if (error == OTA_END_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: End Failed");
    });

    ArduinoOTA.begin();
    DBG_OUTPUT_PORT.println("");
  #endif





  #ifdef ENABLE_MQTT_HOSTNAME_CHIPID
    snprintf(mqtt_clientid, 64, "%s-%08X", HOSTNAME, ESP.getChipId());
  #endif

  #ifdef ENABLE_MQTT
    if (mqtt_host != "" && atoi(mqtt_port) > 0) {
      DBG_OUTPUT_PORT.printf("MQTT active: %s:%d\n", mqtt_host, String(mqtt_port).toInt());

      mqtt_client.setServer(mqtt_host, atoi(mqtt_port));
      mqtt_client.setCallback(mqtt_callback);
    }
  #endif

  #ifdef ENABLE_AMQTT
    if (mqtt_host != "" && atoi(mqtt_port) > 0) {
      amqttClient.onConnect(onMqttConnect);
      amqttClient.onDisconnect(onMqttDisconnect);
      amqttClient.onMessage(onMqttMessage);
      amqttClient.setServer(mqtt_host, atoi(mqtt_port));
      if (mqtt_user != "" or mqtt_pass != "") amqttClient.setCredentials(mqtt_user, mqtt_pass);
      amqttClient.setClientId(mqtt_clientid);
      amqttClient.setWill(mqtt_will_topic, 2, true, mqtt_will_payload, 0);

      connectToMqtt();
    }
  #endif
# 601 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
  bool mdns_result = MDNS.begin(HOSTNAME);

  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(WiFi.localIP());
  DBG_OUTPUT_PORT.println("/ to open McLighting.");

  DBG_OUTPUT_PORT.print("Use http://");
  DBG_OUTPUT_PORT.print(HOSTNAME);
  DBG_OUTPUT_PORT.println(".local/ when you have Bonjour installed.");

  DBG_OUTPUT_PORT.print("New users: Open http://");
  DBG_OUTPUT_PORT.print(WiFi.localIP());
  DBG_OUTPUT_PORT.println("/upload to upload the webpages first.");

  DBG_OUTPUT_PORT.println("");





  webSocket.begin();
  webSocket.onEvent(webSocketEvent);





  server.on("/list", HTTP_GET, handleFileList);

  server.on("/edit", HTTP_PUT, handleFileCreate);

  server.on("/edit", HTTP_DELETE, handleFileDelete);


  server.on("/edit", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.on("/esp_status", HTTP_GET, []() {
    DynamicJsonDocument jsonBuffer(JSON_OBJECT_SIZE(21) + 1500);
    JsonObject json = jsonBuffer.to<JsonObject>();

    json["HOSTNAME"] = HOSTNAME;
    json["version"] = SKETCH_VERSION;
    json["heap"] = ESP.getFreeHeap();
    json["sketch_size"] = ESP.getSketchSize();
    json["free_sketch_space"] = ESP.getFreeSketchSpace();
    json["flash_chip_size"] = ESP.getFlashChipSize();
    json["flash_chip_real_size"] = ESP.getFlashChipRealSize();
    json["flash_chip_speed"] = ESP.getFlashChipSpeed();
    json["sdk_version"] = ESP.getSdkVersion();
    json["core_version"] = ESP.getCoreVersion();
    json["cpu_freq"] = ESP.getCpuFreqMHz();
    json["chip_id"] = ESP.getFlashChipId();
    #if defined(USE_WS2812FX_DMA)
      json["animation_lib"] = "WS2812FX_DMA";
      json["pin"] = 3;
    #elif defined(USE_WS2812FX_UART1)
      json["animation_lib"] = "WS2812FX_UART1";
      json["pin"] = 1;
    #elif defined(USE_WS2812FX_UART2)
      json["animation_lib"] = "WS2812FX_UART2";
      json["pin"] = 2;
    #else
      json["animation_lib"] = "WS2812FX";
      json["pin"] = LED_PIN;
    #endif
    json["number_leds"] = NUMLEDS;
    #ifdef ENABLE_BUTTONS
      json["button_mode"] = "ON";
    #else
      json["button_mode"] = "OFF";
    #endif
    #ifdef ENABLE_AMQTT
      json["amqtt"] = "ON";
    #endif
    #ifdef ENABLE_MQTT
      json["mqtt"] = "ON";
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      json["home_assistant"] = "ON";
    #else
      json["home_assistant"] = "OFF";
    #endif
    #ifdef ENABLE_LEGACY_ANIMATIONS
      json["legacy_animations"] = "ON";
    #else
      json["legacy_animations"] = "OFF";
    #endif
    #ifdef HTTP_OTA
      json["esp8266_http_updateserver"] = "ON";
    #endif
    #ifdef ENABLE_OTA
      json["arduino_ota"] = "ON";
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      json["state_save"] = "SPIFFS";
    #endif
    #ifdef ENABLE_STATE_SAVE_EEPROM
      json["state_save"] = "EEPROM";
    #endif

    String json_str;
    serializeJson(json, json_str);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json_str);
  });

  server.on("/", HTTP_GET, [&](){
#ifdef USE_HTML_MIN_GZ
    server.sendHeader("Content-Encoding", "gzip", true);
    server.send_P(200, PSTR("text/html"), index_htm_gz, index_htm_gz_len);
#else
    if (!handleFileRead(server.uri()))
      handleNotFound();
#endif
  });

  server.on("/edit", HTTP_GET, [&](){
#ifdef USE_HTML_MIN_GZ
    server.sendHeader("Content-Encoding", "gzip", true);
    server.send_P(200, PSTR("text/html"), edit_htm_gz, edit_htm_gz_len);
#else
    if (!handleFileRead(server.uri()))
      handleNotFound();
#endif
  });



  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      handleNotFound();
  });

  server.on("/upload", handleMinimalUpload);

  server.on("/restart", []() {
    DBG_OUTPUT_PORT.printf("/restart\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "restarting..." );
    ESP.restart();
  });

  server.on("/reset_wlan", []() {
    DBG_OUTPUT_PORT.printf("/reset_wlan\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Resetting WLAN and restarting..." );
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/start_config_ap", []() {
    DBG_OUTPUT_PORT.printf("/start_config_ap\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Starting config AP ..." );
    WiFiManager wifiManager;
    wifiManager.startConfigPortal(HOSTNAME);
  });





  server.on("/set_brightness", []() {
    getArgs();
 mode = BRIGHTNESS;
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String(String("OK %") + String(brightness)).c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic, qospub, false, String(String("OK %") + String(brightness)).c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
      if(!ha_send_data.active()) ha_send_data.once(5, tickerSendState);
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
    #endif
    getStatusJSON();
  });

  server.on("/get_brightness", []() {
    String str_brightness = String((int) (brightness / 2.55));
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", str_brightness );
    DBG_OUTPUT_PORT.print("/get_brightness: ");
    DBG_OUTPUT_PORT.println(str_brightness);
  });

  server.on("/set_speed", []() {
    if (server.arg("d").toInt() >= 0) {
      ws2812fx_speed = server.arg("d").toInt();
      ws2812fx_speed = constrain(ws2812fx_speed, 0, 255);
      strip->setSpeed(convertSpeed(ws2812fx_speed));
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String(String("OK ?") + String(ws2812fx_speed)).c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String(String("OK ?") + String(ws2812fx_speed)).c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        if(!ha_send_data.active()) ha_send_data.once(5, tickerSendState);
      #endif
    }

    getStatusJSON();
  });

  server.on("/get_speed", []() {
    String str_speed = String(ws2812fx_speed);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", str_speed );
    DBG_OUTPUT_PORT.print("/get_speed: ");
    DBG_OUTPUT_PORT.println(str_speed);
  });

  server.on("/get_switch", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (mode == OFF) ? "0" : "1" );
    DBG_OUTPUT_PORT.printf("/get_switch: %s\n", (mode == OFF) ? "0" : "1");
  });

  server.on("/get_color", []() {
    char rgbcolor[7];
    snprintf(rgbcolor, sizeof(rgbcolor), "%02X%02X%02X", main_color.red, main_color.green, main_color.blue);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", rgbcolor );
    DBG_OUTPUT_PORT.print("/get_color: ");
    DBG_OUTPUT_PORT.println(rgbcolor);
  });

  server.on("/status", []() {
    getStatusJSON();
  });

  server.on("/pixelconf", []() {
# 855 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/McLighting.ino"
    bool updateStrip = false;
    if(server.hasArg("ct")){
      uint16_t pixelCt = server.arg("ct").toInt();
      if (pixelCt > 0) {
        WS2812FXStripSettings.stripSize = pixelCt;
        updateStrip = true;
        DBG_OUTPUT_PORT.printf("/pixels: Count# %d\n", pixelCt);
      }
    }
    if(server.hasArg("rgbo")){
      String RGBOrder = server.arg("rgbo");
      DBG_OUTPUT_PORT.print("/pixels: RGB Order# ");
      if (RGBOrder == "grb") {
        WS2812FXStripSettings.RGBOrder = NEO_GRB;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else if (RGBOrder == "gbr") {
        WS2812FXStripSettings.RGBOrder = NEO_GBR;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else if (RGBOrder == "rgb") {
        WS2812FXStripSettings.RGBOrder = NEO_RGB;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else if (RGBOrder == "rbg") {
        WS2812FXStripSettings.RGBOrder = NEO_RBG;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else if (RGBOrder == "brg") {
        WS2812FXStripSettings.RGBOrder = NEO_BRG;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else if (RGBOrder == "bgr") {
        WS2812FXStripSettings.RGBOrder = NEO_BGR;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(RGBOrder);
      } else {
        DBG_OUTPUT_PORT.println("invalid input!");
      }
    }
    if(server.hasArg("pin")){
      uint8_t pin = server.arg("pin").toInt();
      DBG_OUTPUT_PORT.print("/pixels: GPIO used# ");
      #if defined(USE_WS2812FX_DMA) or defined(USE_WS2812FX_UART1) or defined(USE_WS2812FX_UART2)
        #ifdef USE_WS2812FX_DMA
          DBG_OUTPUT_PORT.println("3");
        #endif
        #ifdef USE_WS2812FX_UART1
          DBG_OUTPUT_PORT.println("2");
        #endif
        #ifdef USE_WS2812FX_UART2
          DBG_OUTPUT_PORT.println("1");
        #endif
      #else
      if (pin == 16 or pin == 5 or pin == 4 or pin == 0 or pin == 2 or pin == 14 or pin == 12 or pin == 13 or pin == 15 or pin == 3 or pin == 1) {
        WS2812FXStripSettings.pin = pin;
        updateStrip = true;
        DBG_OUTPUT_PORT.println(pin);
      } else {
        DBG_OUTPUT_PORT.println("invalid input!");
      }
      #endif
    }

    if(updateStrip)
    {
      ws2812fx_mode = strip->getMode();
      if(strip->isRunning()) strip->stop();
      initStrip();
      strip->setMode(ws2812fx_mode);
      strip->trigger();
    }

    DynamicJsonDocument jsonBuffer(200);
    JsonObject json = jsonBuffer.to<JsonObject>();
    json["pixel_count"] = WS2812FXStripSettings.stripSize;
    json["rgb_order"] = WS2812FXStripSettings.RGBOrder;
    json["pin"] = WS2812FXStripSettings.pin;
    String json_str;
    serializeJson(json, json_str);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json_str);
  });

  server.on("/off", []() {
    #ifdef ENABLE_LEGACY_ANIMATIONS
      exit_func = true;
    #endif
    mode = OFF;
    getArgs();
    getStatusJSON();
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String("OK =off").c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =off").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = false;
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
    #endif
  });

  server.on("/all", []() {
    #ifdef ENABLE_LEGACY_ANIMATIONS
      exit_func = true;
    #endif
    ws2812fx_mode = FX_MODE_STATIC;
    mode = SET_MODE;

    getArgs();
    getStatusJSON();
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String("OK =all").c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =all").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
    #endif
  });

  #ifdef ENABLE_LEGACY_ANIMATIONS
    server.on("/wipe", []() {
      exit_func = true;
      mode = WIPE;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =wipe").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =wipe").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    server.on("/rainbow", []() {
      exit_func = true;
      mode = RAINBOW;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =rainbow").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =rainbow").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    server.on("/rainbowCycle", []() {
      exit_func = true;
      mode = RAINBOWCYCLE;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =rainbowCycle").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =rainbowCycle").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    server.on("/theaterchase", []() {
      exit_func = true;
      mode = THEATERCHASE;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =theaterchase").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =theaterchase").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    server.on("/twinkleRandom", []() {
      exit_func = true;
      mode = TWINKLERANDOM;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =twinkleRandom").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =twinkleRandom").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    server.on("/theaterchaseRainbow", []() {
      exit_func = true;
      mode = THEATERCHASERAINBOW;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =theaterchaseRainbow").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =theaterchaseRainbow").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });

    #ifdef ENABLE_E131
    server.on("/e131", []() {
      exit_func = true;
      if(strip->isRunning()) strip->stop();
      mode = E131;
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =e131").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =131").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });
    #endif

    server.on("/tv", []() {
      exit_func = true;
      mode = TV;
      getArgs();
      getStatusJSON();
      #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =tv").c_str());
      #endif
      #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic, qospub, false, String("OK =tv").c_str());
      #endif
      #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
      #endif
      #ifdef ENABLE_STATE_SAVE_SPIFFS
        if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
      #endif
    });
  #endif

  server.on("/get_modes", []() {
    getModesJSON();
  });

  server.on("/set_mode", []() {
    getArgs();
    mode = SET_MODE;
    getStatusJSON();

    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String(String("OK /") + String(ws2812fx_mode)).c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic, qospub, false, String(String("OK /") + String(ws2812fx_mode)).c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
      if(!ha_send_data.active()) ha_send_data.once(5, tickerSendState);
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      if(!spiffs_save_state.active()) spiffs_save_state.once(3, tickerSpiffsSaveState);
    #endif
  });

  #ifdef HTTP_OTA
    httpUpdater.setup(&server, "/update");
  #endif

  server.begin();


  if (mdns_result) {
    MDNS.addService("http", "tcp", 80);
  }

  #ifdef ENABLE_E131


  if (e131.begin(E131_MULTICAST, START_UNIVERSE, END_UNIVERSE))
      Serial.println(F("E1.31 mode setup complete."));
  else
      Serial.println(F("*** e131.begin failed ***"));
  #endif
  #ifdef ENABLE_STATE_SAVE_SPIFFS
    (readStateFS()) ? DBG_OUTPUT_PORT.println(" Success!") : DBG_OUTPUT_PORT.println(" Failure!");
  #endif
  #ifdef ENABLE_STATE_SAVE_EEPROM

    String saved_state_string = readEEPROM(256, 32);
    String chk = getValue(saved_state_string, '|', 0);
    if (chk == "STA") {
      DBG_OUTPUT_PORT.printf("Found saved state: %s\n", saved_state_string.c_str());
      setModeByStateString(saved_state_string);
    }
    sprintf(last_state, "STA|%2d|%3d|%3d|%3d|%3d|%3d|%3d", mode, ws2812fx_mode, ws2812fx_speed, brightness, main_color.red, main_color.green, main_color.blue);
  #endif
}

void loop() {
  #ifdef ENABLE_BUTTONS
    debouncer1.update();
    if (debouncer1.fell()) {
      button1();
    }
    debouncer2.update();
    if (debouncer2.fell()) {
      button2();
    }
  #endif
  server.handleClient();
  webSocket.loop();

  #ifdef ENABLE_OTA
    ArduinoOTA.handle();
  #endif

  #ifdef ENABLE_MQTT
    if (WiFi.status() != WL_CONNECTED) {
      #ifdef ENABLE_HOMEASSISTANT
         ha_send_data.detach();
      #endif
      DBG_OUTPUT_PORT.println("WiFi disconnected, reconnecting!");
      WiFi.disconnect();
      WiFi.setSleepMode(WIFI_NONE_SLEEP);
      WiFi.mode(WIFI_STA);
      WiFi.begin();
    } else {
      if (mqtt_host != "" && String(mqtt_port).toInt() > 0 && mqtt_reconnect_retries < MQTT_MAX_RECONNECT_TRIES) {
        if (!mqtt_client.connected()) {
          #ifdef ENABLE_HOMEASSISTANT
           ha_send_data.detach();
          #endif
          DBG_OUTPUT_PORT.println("MQTT disconnected, reconnecting!");
          mqtt_reconnect();
        } else {
          mqtt_client.loop();
        }
      }
    }
  #endif
  #ifdef ENABLE_HOMEASSISTANT

   if (new_ha_mqtt_msg) sendState();
  #endif


  if (mode == SET_MODE) {
    DBG_OUTPUT_PORT.printf("SET_MODE: %d %d\n", ws2812fx_mode, mode);
    if(strip) strip->setMode(ws2812fx_mode);
    if(strip) strip->trigger();
    prevmode = SET_MODE;
    mode = SETCOLOR;
  }
  if (mode == OFF) {
    if(strip){
      if(strip->isRunning()) strip->stop();

    }
  }
  if (mode == SETCOLOR) {
    if(strip) strip->setColor(main_color.red, main_color.green, main_color.blue);
    if(strip) strip->trigger();
    mode = (prevmode == SET_MODE) ? SETSPEED : HOLD;
  }
  if (mode == SETSPEED) {
    if(strip) strip->setSpeed(convertSpeed(ws2812fx_speed));
    if(strip) strip->trigger();
    mode = (prevmode == SET_MODE) ? BRIGHTNESS : HOLD;
  }
  if (mode == BRIGHTNESS) {
    if(strip) strip->setBrightness(brightness);
    if(strip) strip->trigger();
    if (prevmode == SET_MODE) prevmode = HOLD;
    mode = HOLD;
  }
  #ifdef ENABLE_LEGACY_ANIMATIONS
    if (mode == WIPE) {
      if(strip) strip->setColor(main_color.red, main_color.green, main_color.blue);
      if(strip) strip->setMode(FX_MODE_COLOR_WIPE);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    if (mode == RAINBOW) {
      if(strip) strip->setMode(FX_MODE_RAINBOW);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    if (mode == RAINBOWCYCLE) {
      if(strip) strip->setMode(FX_MODE_RAINBOW_CYCLE);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    if (mode == THEATERCHASE) {
      if(strip) strip->setColor(main_color.red, main_color.green, main_color.blue);
      if(strip) strip->setMode(FX_MODE_THEATER_CHASE);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    if (mode == TWINKLERANDOM) {
      if(strip) strip->setColor(main_color.red, main_color.green, main_color.blue);
      if(strip) strip->setMode(FX_MODE_TWINKLE_RANDOM);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    if (mode == THEATERCHASERAINBOW) {
      if(strip) strip->setMode(FX_MODE_THEATER_CHASE_RAINBOW);
      if(strip) strip->trigger();
      mode = HOLD;
    }
    #ifdef ENABLE_E131
    if (mode == E131) {
      handleE131();
    }
    #endif
  #endif
  if (mode == HOLD || mode == CUSTOM) {
    if(strip)
    {
      if(!strip->isRunning()) strip->start();
    }
    #ifdef ENABLE_LEGACY_ANIMATIONS
      if (exit_func) {
        exit_func = false;
      }
    #endif
    if (prevmode == SET_MODE) prevmode = HOLD;
  }
  #ifdef ENABLE_LEGACY_ANIMATIONS
    if (mode == TV) {
      if(strip)
      {
        if(!strip->isRunning()) strip->start();
      }
      tv();
    }
  #endif


  #ifdef ENABLE_LEGACY_ANIMATIONS
  if (mode != TV && mode != CUSTOM) {
  #else
  if (mode != CUSTOM) {
  #endif
    if(strip) strip->service();
  }

  #ifdef ENABLE_STATE_SAVE_SPIFFS
    if (updateStateFS) {
      (writeStateFS()) ? DBG_OUTPUT_PORT.println(" Success!") : DBG_OUTPUT_PORT.println(" Failure!");
    }
  #endif

  #ifdef ENABLE_STATE_SAVE_EEPROM

    sprintf(current_state, "STA|%2d|%3d|%3d|%3d|%3d|%3d|%3d", mode, strip->getMode(), ws2812fx_speed, brightness, main_color.red, main_color.green, main_color.blue);

    if (strcmp(current_state, last_state) != 0) {

      strcpy(last_state, current_state);
      time_statechange = millis();
      state_save_requested = true;
    }
    if (state_save_requested && time_statechange + timeout_statechange_save <= millis()) {
      time_statechange = 0;
      state_save_requested = false;
      writeEEPROM(256, 32, last_state);
      EEPROM.commit();
    }
  #endif
}
# 1 "/Users/matthiaskleine/Dev/PlatformIO/McLighting/Arduino/McLighting/version_info.ino"