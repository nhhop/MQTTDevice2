bool loadConfig()
{
  DEBUG_MSG("%s\n", "------ loadConfig started ------");
  File configFile = LittleFS.open("/config.txt", "r");
  if (!configFile)
  {
    DEBUG_MSG("%s\n", "Failed to open config file\n");
    DEBUG_MSG("%s\n", "------ loadConfig aborted ------\n");
    return false;
  }

  size_t size = configFile.size();
  if (size > 2048)
  {
    DEBUG_MSG("%s\n", "Config file size is too large");
    DEBUG_MSG("%s\n", "------ loadConfig aborted ------");
    if (startBuzzer)
      sendAlarm(ALARM_ERROR);
    return false;
  }

  DynamicJsonDocument doc(2500);
  DeserializationError error = deserializeJson(doc, configFile);
  if (error)
  {
    DEBUG_MSG("Conf: Error Json %s\n", error.c_str());
    if (startBuzzer)
      sendAlarm(ALARM_ERROR);
    return false;
  }

  JsonArray actorsArray = doc["actors"];
  numberOfActors = actorsArray.size();
  if (numberOfActors > numberOfActorsMax)
    numberOfActors = numberOfActorsMax;
  int i = 0;
  for (JsonObject actorObj : actorsArray)
  {
    if (i < numberOfActors)
    {
      String actorPin = actorObj["PIN"];
      String actorScript = actorObj["TOPIC"];
      String actorName = actorObj["NAME"];
      bool actorInv = false;
      bool actorSwitch = false;
      bool actorRemote = false;

      if (actorObj["INV"] || actorObj["INV"] == "1")
        actorInv = true;
      if (actorObj["SW"] || actorObj["SW"] == "1")
        actorSwitch = true;
      if (actorObj["REMOTE"] || actorObj["REMOTE"] == "1")
        actorRemote = true;

      actors[i].change(actorPin, actorScript, actorName, actorInv, actorSwitch, actorRemote);
      DEBUG_MSG("Actor #: %d Name: %s MQTT: %s PIN: %s INV: %d SW: %d REMOTE: %d\n", (i + 1), actorName.c_str(), actorScript.c_str(), actorPin.c_str(), actorInv, actorSwitch, actorRemote);
      i++;
    }
  }

  if (numberOfActors == 0)
  {
    DEBUG_MSG("Actors: %d\n", numberOfActors);
  }
  DEBUG_MSG("%s\n", "--------------------");

  JsonArray sensorsArray = doc["sensors"];
  numberOfSensors = sensorsArray.size();

  if (numberOfSensors > numberOfSensorsMax)
    numberOfSensors = numberOfSensorsMax;
  i = 0;
  for (JsonObject sensorObj : sensorsArray)
  {
    if (i < numberOfSensors)
    {
      String sensorAddress = sensorObj["ADDRESS"];
      String sensorTopic = sensorObj["TOPIC"];
      String sensorName = sensorObj["NAME"];
      bool sensorSwitch = false;
      bool sensorRemote = false;
      float sensorOffset = 0.0;
      if (sensorObj["SW"] || sensorObj["SW"] == "1")
        sensorSwitch = true;
      if (sensorObj["REMOTE"] || sensorObj["REMOTE"] == "1")
        sensorRemote = true;
      if (sensorObj.containsKey("OFFSET"))
        sensorOffset = sensorObj["OFFSET"];

      sensors[i].change(sensorAddress, sensorTopic, sensorName, sensorOffset, sensorSwitch, sensorRemote);
      DEBUG_MSG("Sensor #: %d Name: %s Address: %s MQTT: %s Offset: %f SW: %d\n", (i + 1), sensorsName.c_str(), sensorsAddress.c_str(), sensorsScript.c_str(), sensorsOffset, sensorsSwitch);
      i++;
    }
    else
      sensors[i].change("", "", "", 0.0, false, false);
  }
  DEBUG_MSG("%s\n", "--------------------");

  JsonArray indArray = doc["induction"];
  JsonObject indObj = indArray[0];
  if (indObj.containsKey("ENABLED"))
  {
    inductionStatus = 1;
    bool indEnabled = true;
    bool indRemote = false;
    String indPinWhite = indObj["PINWHITE"];
    String indPinYellow = indObj["PINYELLOW"];
    String indPinBlue = indObj["PINBLUE"];
    String indTopic = indObj["TOPIC"];
    long indDelayOff = DEF_DELAY_IND; //default delay
    int indPowerLevel = 100;
    unsigned char ids_type = 1;

    if (indObj.containsKey("IDS_TYPE"))
      ids_type = indObj["IDS_TYPE"];
    if (indObj["REMOTE"] || indObj["REMOTE"] == "1")
      indRemote = true;
    if (indObj.containsKey("PL"))
      indPowerLevel = indObj["PL"];
    if (indObj.containsKey("DELAY"))
      indDelayOff = indObj["DELAY"];

    inductionCooker.change(ids_type ,StringToPin(indPinWhite), StringToPin(indPinYellow), StringToPin(indPinBlue), indTopic, indDelayOff, indEnabled, indPowerLevel, indRemote);
    DEBUG_MSG("Induction: %d MQTT: %s Relais (WHITE): %s Command channel (YELLOW): %s Backchannel (BLUE): %s Delay after power off %d Power level on error: %d Remote: %d\n", inductionStatus, indScript.c_str(), indPinWhite.c_str(), indPinYellow.c_str(), indPinBlue.c_str(), (indDelayOff / 1000), indPowerLevel, indRemote);
  }
  else
  {
    inductionStatus = 0;
    DEBUG_MSG("Induction: %d\n", inductionStatus);
  }
  DEBUG_MSG("%s\n", "--------------------");
  JsonArray displayArray = doc["display"];
  JsonObject displayObj = displayArray[0];
  useDisplay = false;
  if (displayObj["ENABLED"] || displayObj["ENABLED"] == "1")
    useDisplay = true;

  if (useDisplay)
  {
    String dispAddress = displayObj["ADDRESS"];
    dispAddress.remove(0, 2);
    char copy[4];
    dispAddress.toCharArray(copy, 4);
    int address = strtol(copy, 0, 16);
    if (displayObj.containsKey("updisp"))
      DISP_UPDATE = displayObj["updisp"];

    oledDisplay.isEnabled = true;
    oledDisplay.change(address, oledDisplay.isEnabled);
    DEBUG_MSG("OLED display: %d Address: %s Update: %d\n", oledDisplay.isEnabled, dispAddress.c_str(), (DISP_UPDATE / 1000));
    TickerDisp.config(DISP_UPDATE, 0);
    TickerDisp.start();
  }
  else
  {
    oledDisplay.isEnabled = false;
    DEBUG_MSG("OLED Display: %d\n", oledDisplay.isEnabled);
    TickerDisp.stop();
  }
  DEBUG_MSG("%s\n", "--------------------");

  // Misc Settings
  JsonArray miscArray = doc["misc"];
  JsonObject miscObj = miscArray[0];

  if (miscObj.containsKey("del_sen_act"))
    wait_on_Sensor_error_actor = miscObj["del_sen_act"];
  if (miscObj.containsKey("del_sen_ind"))
    wait_on_Sensor_error_induction = miscObj["del_sen_ind"];
  DEBUG_MSG("Wait on sensor error actors: %d sec\n", wait_on_Sensor_error_actor / 1000);
  DEBUG_MSG("Wait on sensor error induction: %d sec\n", wait_on_Sensor_error_induction / 1000);

  startBuzzer = false;
  if (miscObj["buzzer"] || miscObj["buzzer"] == "1")
    startBuzzer = true;
  DEBUG_MSG("Buzzer: %d\n", startBuzzer);
  cbpi = false;   // default cbpi3
  if (miscObj["cbpi"] || miscObj["cbpi"] == "1")
    cbpi = true;
  DEBUG_MSG("CBPi Version: %d\n", cbpi);

  if (miscObj.containsKey("mdns_name"))
    strlcpy(nameMDNS, miscObj["mdns_name"], sizeof(nameMDNS));
  startMDNS = false;
  if (miscObj["mdns"] || miscObj["mdns"] == "1")
    startMDNS = true;
  DEBUG_MSG("mDNS: %d name: %s\n", startMDNS, nameMDNS);

  if (miscObj.containsKey("upsen"))
    SEN_UPDATE = miscObj["upsen"];
  if (miscObj.containsKey("upact"))
    ACT_UPDATE = miscObj["upact"];
  if (miscObj.containsKey("upind"))
    IND_UPDATE = miscObj["upind"];

  TickerSen.config(SEN_UPDATE, 0);
  TickerAct.config(ACT_UPDATE, 0);
  TickerInd.config(IND_UPDATE, 0);

  if (numberOfSensors > 0)
    TickerSen.start();
  if (numberOfActors > 0)
    TickerAct.start();
  if (inductionCooker.isEnabled)
    TickerInd.start();

  DEBUG_MSG("Sensors update intervall: %d sec\n", (SEN_UPDATE / 1000));
  DEBUG_MSG("Actors update intervall: %d sec\n", (ACT_UPDATE / 1000));
  DEBUG_MSG("Induction update intervall: %d sec\n", (IND_UPDATE / 1000));

  
  JsonArray mqttArray = doc["mqtt"];
  JsonObject mqttObj = mqttArray[0];

  StopOnMQTTError = false;
  if (mqttObj["MQTTERRSTOP"] || mqttObj["MQTTERRSTOP"] == "1")
    StopOnMQTTError = true;
  if (mqttObj.containsKey("MQTTDELAY"))
    wait_on_error_mqtt = mqttObj["MQTTDELAY"];
  DEBUG_MSG("Switch off actors on MQTT error: %d after %d sec\n", StopOnMQTTError, (wait_on_error_mqtt / 1000));

  strlcpy(mqttConnection.host, mqttObj["MQTTHOST"] | "", maxHostSign);
  strlcpy(mqttConnection.user, mqttObj["MQTTUSER"] | "", maxUserSign);
  strlcpy(mqttConnection.password, mqttObj["MQTTPASS"] | "", maxPassSign);
  mqttConnection.port = mqttObj["MQTTPORT"] | 1883;
  mqttConnection.isEnabled = mqttObj["MQTTENABLED"] | 1;
  if (mqttConnection.isEnabled) {
    DEBUG_MSG("MQTT server IP: %s Port: %d User: %s Pass: %s Enabled: %d\n", mqttConnection.host, mqttConnection.port, mqttConnection.user, mqttConnection.password, mqttConnection.isEnabled);
  }

  DEBUG_MSG("%s\n", "------ loadConfig finished ------");
  configFile.close();
  DEBUG_MSG("Config file size %d\n", size);
  size_t len = measureJson(doc);
  DEBUG_MSG("JSON config length: %d\n", len);
  int memoryUsed = doc.memoryUsage();
  DEBUG_MSG("JSON memory usage: %d\n", memoryUsed);

  if (startBuzzer)
    sendAlarm(ALARM_ON);

  return true;
}

void saveConfigCallback()
{

  if (LittleFS.begin())
  {
    saveConfig();
    shouldSaveConfig = true;
  }
  else
  {
    Serial.println("*** SYSINFO: WiFiManager failed to save MQTT broker IP");
  }
}

bool saveConfig()
{
  DEBUG_MSG("%s\n", "------ saveConfig started ------");
  DynamicJsonDocument doc(2500);

  // Write Actors
  JsonArray actorsArray = doc.createNestedArray("actors");
  for (int i = 0; i < numberOfActors; i++)
  {
    JsonObject actorsObj = actorsArray.createNestedObject();
    actorsObj["PIN"] = PinToString(actors[i].pin_actor);
    actorsObj["NAME"] = actors[i].name;
    actorsObj["TOPIC"] = actors[i].mqtttopic;
    actorsObj["INV"] = (int)actors[i].isInverted;
    actorsObj["SW"] = (int)actors[i].switchable;
    actorsObj["REMOTE"] = (int)actors[i].remote;

    DEBUG_MSG("Actor #: %d Name: %s MQTT: %s PIN: %s INV: %d SW: %d REMOTE: %d\n", (i + 1), actors[i].name.c_str(), actors[i].mqtttopic.c_str(), PinToString(actors[i].pin_actor).c_str(), actors[i].isInverted, actors[i].switchable, actors[i].remote);
  }
  if (numberOfActors == 0)
  {
    DEBUG_MSG("Actors: %d\n", numberOfActors);
  }
  DEBUG_MSG("%s\n", "--------------------");

  // Write Sensors
  JsonArray sensorsArray = doc.createNestedArray("sensors");
  for (int i = 0; i < numberOfSensors; i++)
  {
    JsonObject sensorsObj = sensorsArray.createNestedObject();
    sensorsObj["ADDRESS"] = sensors[i].getSens_adress_string();
    sensorsObj["NAME"] = sensors[i].getName();
    sensorsObj["OFFSET"] = sensors[i].getOffset();
    sensorsObj["TOPIC"] = sensors[i].getTopic();
    sensorsObj["SW"] = (int)sensors[i].getSw();
    sensorsObj["REMOTE"] = (int)sensors[i].getRemote();
    DEBUG_MSG("Sensor #: %d Name: %s Address: %s MQTT: %s Offset: %f SW: %d\n", (i + 1), sensors[i].getName().c_str(), sensors[i].getSens_adress_string().c_str(), sensors[i].getTopic().c_str(), sensors[i].getOffset(), sensors[i].getSw());
  }

  DEBUG_MSG("%s\n", "--------------------");

  // Write Induction
  JsonArray indArray = doc.createNestedArray("induction");
  if (inductionCooker.isEnabled)
  {
    inductionStatus = 1;
    JsonObject indObj = indArray.createNestedObject();
    indObj["IDS_TYPE"] = (unsigned char)inductionCooker.IDS_TYPE;
    indObj["PINWHITE"] = PinToString(inductionCooker.PIN_WHITE);
    indObj["PINYELLOW"] = PinToString(inductionCooker.PIN_YELLOW);
    indObj["PINBLUE"] = PinToString(inductionCooker.PIN_INTERRUPT);
    indObj["TOPIC"] = inductionCooker.mqtttopic;
    indObj["DELAY"] = inductionCooker.delayAfteroff;
    indObj["ENABLED"] = (int)inductionCooker.isEnabled;
    indObj["PL"] = inductionCooker.powerLevelOnError;
    indObj["REMOTE"] = (int)inductionCooker.remote;
    DEBUG_MSG("Induction: %d MQTT: %s Relais (WHITE): %s Command channel (YELLOW): %s Backchannel (BLUE): %s Delay after power off %d Power level on error: %d\n", inductionCooker.isEnabled, inductionCooker.mqtttopic.c_str(), PinToString(inductionCooker.PIN_WHITE).c_str(), PinToString(inductionCooker.PIN_YELLOW).c_str(), PinToString(inductionCooker.PIN_INTERRUPT).c_str(), (inductionCooker.delayAfteroff / 1000), inductionCooker.powerLevelOnError);
  }
  else
  {
    inductionStatus = 0;
    DEBUG_MSG("Induction: %d\n", inductionCooker.isEnabled);
  }
  DEBUG_MSG("%s\n", "--------------------");

  // Write Display
  JsonArray displayArray = doc.createNestedArray("display");
  if (oledDisplay.isEnabled)
  {
    JsonObject displayObj = displayArray.createNestedObject();
    displayObj["ENABLED"] = 1;
    displayObj["ADDRESS"] = String(decToHex(oledDisplay.address, 2));
    displayObj["updisp"] = DISP_UPDATE;

    if (oledDisplay.address == 0x3C || oledDisplay.address == 0x3D)
    {
      // Display mit SDD1306 Chip:
      // display.ssd1306_command(SSD1306_DISPLAYON);

      // Display mit SH1106 Chip:
      display.SH1106_command(SH1106_DISPLAYON);
      queueEventSystem(EM_DISPUP);
    }
    else
    {
      displayObj["ENABLED"] = 0;
      oledDisplay.isEnabled = false;
      useDisplay = false;
    }
    DEBUG_MSG("OLED display: %d Address: %s Update: %d\n", oledDisplay.isEnabled, String(decToHex(oledDisplay.address, 2)).c_str(), (DISP_UPDATE / 1000));
    TickerDisp.config(DISP_UPDATE, 0);
    TickerDisp.start();
  }
  else
  {
    // Display mit SSD1306 Chip:
    // display.ssd1306_command(SSD1306_DISPLAYOFF);

    // Display mit SH1106 Chip:
    display.SH1106_command(SH1106_DISPLAYOFF);
    DEBUG_MSG("OLED display: %d\n", oledDisplay.isEnabled);
    TickerDisp.stop();
  }
  DEBUG_MSG("%s\n", "--------------------");

  // Write MQTT stuff  
  JsonArray mqttArray = doc.createNestedArray("mqtt");
  JsonObject mqttObj = mqttArray.createNestedObject();

  mqttObj["MQTTDELAY"] = wait_on_error_mqtt;
  mqttObj["MQTTERRSTOP"] = (int)StopOnMQTTError;
  DEBUG_MSG("Switch off actors on error enabled after %d sec\n", (wait_on_error_mqtt / 1000));
  mqttObj["MQTTHOST"] = mqttConnection.host;
  mqttObj["MQTTPORT"] = mqttConnection.port;
  mqttObj["MQTTUSER"] = mqttConnection.user;
  mqttObj["MQTTPASS"] = mqttConnection.password;
  mqttObj["MQTTENABLED"] = (int)mqttConnection.isEnabled;

  DEBUG_MSG("MQTT broker IP: %s Port: %d User: %s Pass: %s Enabled: %d\n", mqttConnection.host, mqttConnection.port, mqttConnection.user, mqttConnection.password, mqttConnection.isEnabled);
  DEBUG_MSG("%s\n", "--------------------");


  // Write Misc Stuff
  JsonArray miscArray = doc.createNestedArray("misc");
  JsonObject miscObj = miscArray.createNestedObject();

  miscObj["del_sen_act"] = wait_on_Sensor_error_actor;
  miscObj["del_sen_ind"] = wait_on_Sensor_error_induction;
  DEBUG_MSG("Wait on sensor error actors: %d sec\n", wait_on_Sensor_error_actor / 1000);
  DEBUG_MSG("Wait on sensor error induction: %d sec\n", wait_on_Sensor_error_induction / 1000);

  miscObj["buzzer"] = (int)startBuzzer;
  miscObj["cbpi"] = (int)cbpi;
  miscObj["mdns_name"] = nameMDNS;
  miscObj["mdns"] = (int)startMDNS;
   
  miscObj["upsen"] = SEN_UPDATE;
  miscObj["upact"] = ACT_UPDATE;
  miscObj["upind"] = IND_UPDATE;

  TickerSen.config(SEN_UPDATE, 0);
  TickerAct.config(ACT_UPDATE, 0);
  TickerInd.config(IND_UPDATE, 0);

  if (numberOfSensors > 0)
    TickerSen.start();
  else
    TickerSen.stop();
  if (numberOfActors > 0)
    TickerAct.start();
  else
    TickerAct.stop();
  if (inductionCooker.isEnabled)
    TickerInd.start();

  DEBUG_MSG("Sensor update interval %d sec\n", (SEN_UPDATE / 1000));
  DEBUG_MSG("Actors update interval %d sec\n", (ACT_UPDATE / 1000));
  DEBUG_MSG("Induction update interval %d sec\n", (IND_UPDATE / 1000));
  DEBUG_MSG("CBPi Version: %d\n", cbpi);
  DEBUG_MSG("MQTT broker IP: %s\n", mqtthost);

  size_t len = measureJson(doc);
  int memoryUsed = doc.memoryUsage();

  if (len > 2048 || memoryUsed > 2500)
  {
    DEBUG_MSG("JSON config length: %d\n", len);
    DEBUG_MSG("JSON memory usage: %d\n", memoryUsed);
    DEBUG_MSG("%s\n", "Failed to write config file - config too large");
    DEBUG_MSG("%s\n", "------ saveConfig aborted ------");
    if (startBuzzer)
      sendAlarm(ALARM_ERROR);
    return false;
  }

  File configFile = LittleFS.open("/config.txt", "w");
  if (!configFile)
  {
    DEBUG_MSG("%s\n", "Failed to open config file for writing");
    DEBUG_MSG("%s\n", "------ saveConfig aborted ------");
    if (startBuzzer)
      sendAlarm(ALARM_ERROR);
    return false;
  }
  serializeJson(doc, configFile);
  configFile.close();
  DEBUG_MSG("%s\n", "------ saveConfig finished ------");
  String Network = WiFi.SSID();
  DEBUG_MSG("ESP8266 device IP Address: %s\n", WiFi.localIP().toString().c_str());
  DEBUG_MSG("Configured WLAN SSID: %s\n", Network.c_str());
  DEBUG_MSG("%s\n", "---------------------------------");
  if (startBuzzer)
    sendAlarm(ALARM_ON);
  return true;
}
