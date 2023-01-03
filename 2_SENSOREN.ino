class TemperatureSensor
{
  int err = 0;
  bool sw = false;          // Events aktivieren
  bool state = true;        // Fehlerstatus ensor
  bool isConnected;         // ist der Sensor verbunden
  float offset = 0.0;       // Offset - Temp kalibrieren
  float value = -127.0;     // Aktueller Wert
  unsigned char address[8]; // 1-Wire Adresse

public:
  bool remote = false;
  String name;              // Name für Anzeige auf Website
  String mqtttopic = "";       // Für MQTT Kommunikation
  // moved to private and get methods. change as set method

  String getSens_adress_string()
  {
    return SensorAddressToString(address);
  }

  TemperatureSensor(String new_address, String new_mqtttopic, String new_name, float new_offset, bool new_sw, bool new_remote)
  {
    change(new_address, new_mqtttopic, new_name, new_offset, new_sw, new_remote);
  }

  void Update()
  {
    if (remote) 
    {
        //value = 9999;
        return;
    }

    DS18B20.requestTemperatures();                        // new conversion to get recent temperatures
    isConnected = DS18B20.isConnected(address); // attempt to determine if the device at the given address is connected to the bus
    isConnected ? value = DS18B20.getTempC(address) : value = -127.0;

    if (!isConnected && address[0] != 0xFF && address[0] != 0x00) // double check on !isConnected. Billig Tempfühler ist manchmal für 1-2 loops nicht connected. 0xFF default address. 0x00 virtual test device (adress 00 00 00 00 00)
    {
      millis2wait(PAUSEDS18);                               // Wartezeit ca 750ms bevor Lesen vom Sensor wiederholt wird (Init Zeit)
      isConnected = DS18B20.isConnected(address); // hat der Sensor ene Adresse und ist am Bus verbunden?
      isConnected ? value = DS18B20.getTempC(address) : value = -127.0;
    }

    if (value == 85.0)
    {                         // 85 Grad ist Standard Temp Default Reset. Wenn das Kabel zu lang ist, kommt als Fehler 85 Grad
      millis2wait(PAUSEDS18); // Wartezeit 750ms vor einer erneuten Sensorabfrage
      DS18B20.requestTemperatures();
    }
    sensorsStatus = 0;
    state = true;
    if (OneWire::crc8(address, 7) != address[7])
    {
      sensorsStatus = EM_CRCER;
      state = false;
    }
    else if (value == -127.00 || value == 85.00)
    {
      if (isConnected && address[0] != 0xFF)
      { // Sensor connected AND sensor address exists (not default FF)
        sensorsStatus = EM_DEVER;
        state = false;
      }
      else if (!isConnected && address[0] != 0xFF)
      { // Sensor with valid address not connected
        sensorsStatus = EM_UNPL;
        state = false;
      }
      else // not connected and unvalid address
      {
        sensorsStatus = EM_SENER;
        state = false;
      }
    } // value -127 || +85
    else
    {
      sensorsStatus = EM_OK;
      state = true;
    }
    err = sensorsStatus;
    mqtt_publish();
  } // void Update

  void change(const String &new_address, const String &new_mqtttopic, const String &new_name, float new_offset, const bool &new_sw, const bool &new_remote)
  {
    mqtt_unsubscribe();
    
    mqtttopic = new_mqtttopic;
    name = new_name;
    offset = new_offset;
    remote = new_remote;
    sw = new_sw;

    if (remote && pubsubClient.connected())
    {      
      mqtt_subscribe();
    }

    if (new_address.length() == 16)
    {
      char address_char[16];

      new_address.toCharArray(address_char, 17);

      char hexbyte[2];
      int octets[8];

      for (int d = 0; d < 16; d += 2)
      {
        // Assemble a digit pair into the hexbyte string
        hexbyte[0] = address_char[d];
        hexbyte[1] = address_char[d + 1];

        // Convert the hex pair to an integer
        sscanf(hexbyte, "%x", &octets[d / 2]);
        yield();
      }
      for (int i = 0; i < 8; i++)
      {
        address[i] = octets[i];
      }
    }
    DS18B20.setResolution(address, 10);
  }

  void mqtt_subscribe()
  {
    if (pubsubClient.connected())
    {
      char subscribemsg[50];
      mqtttopic.toCharArray(subscribemsg, 50);
      DEBUG_MSG("Act: Subscribing to %s\n", subscribemsg);
      pubsubClient.subscribe(subscribemsg);
    }
  }

  void mqtt_unsubscribe()
  {
    if (pubsubClient.connected())
    {
      char subscribemsg[50];
      mqtttopic.toCharArray(subscribemsg, 50);
      DEBUG_MSG("Act: Unsubscribing from %s\n", subscribemsg);
      pubsubClient.unsubscribe(subscribemsg);
    }
  }

  void mqtt_publish()
  {
    if (pubsubClient.connected())
    {
      StaticJsonDocument<256> doc;
      JsonObject sensorsObj = doc.createNestedObject("Sensor");
      sensorsObj["Name"] = name;
      if (sensorsStatus == 0)
      {
        sensorsObj["Value"] = (value + offset);
      }
      else
      {
        sensorsObj["Value"] = value;
      }
      sensorsObj["Type"] = "1-wire";
      char jsonMessage[100];
      serializeJson(doc, jsonMessage);
      char topic[50];
      mqtttopic.toCharArray(topic, 50);
      pubsubClient.publish(topic, jsonMessage);
    }
  }

  void handlemqtt(char *payload)
  {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, (const char *)payload);
    if (error)
    {
      DEBUG_MSG("Act: handlemqtt deserialize Json error %s\n", error.c_str());
      return;
    }
    Serial.print("Remote Sensor: ");
    Serial.println(payload);
    value = doc["Sensor"]["Value"];
  }

  int getErr()
  {
    return err;
  }
  bool getRemote()
  {
    return remote;
  }
  bool getSw()
  {
    return sw;
  }
  bool getState()
  {
    return state;
  }
  float getOffset()
  {
    return offset;
  }
  float getValue()
  {
    return value;
  }
  String getName()
  {
    return name;
  }
  String getTopic()
  {
    return mqtttopic;
  }
  char buf[5];
  char *getValueString()
  {
    dtostrf(value, 2, 1, buf);
    return buf;
  }
  char *getTotalValueString()
  {
    sprintf(buf, "%s", "0.0");
    if (value == -127.0)
      return buf;

    dtostrf((value + offset), 2, 1, buf);
    return buf;
  }
};

// Initialisierung des Arrays -> max 6 Sensoren
TemperatureSensor sensors[numberOfSensorsMax] = {
    TemperatureSensor("", "", "", 0.0, false, false),
    TemperatureSensor("", "", "", 0.0, false, false),
    TemperatureSensor("", "", "", 0.0, false, false),
    TemperatureSensor("", "", "", 0.0, false, false),
    TemperatureSensor("", "", "", 0.0, false, false),
    TemperatureSensor("", "", "", 0.0, false, false)};

// Funktion für Loop im Timer Objekt
void handleSensors()
{
  int max_status = 0;
  for (int i = 0; i < numberOfSensors; i++)
  {
    sensors[i].Update();

    // get max sensorstatus
    if (sensors[i].getSw() && max_status < sensors[i].getErr())
      max_status = sensors[i].getErr();

    yield();
  }
  sensorsStatus = max_status;
}

unsigned char searchSensors()
{
  unsigned char i;
  unsigned char n = 0;
  unsigned char addr[8];

  while (oneWire.search(addr))
  {

    if (OneWire::crc8(addr, 7) == addr[7])
    {
      for (i = 0; i < 8; i++)
      {
        addressesFound[n][i] = addr[i];
      }
      n += 1;
    }
    yield();
  }
  return n;
  oneWire.reset_search();
}

String SensorAddressToString(unsigned char addr[8])
{
  char charbuffer[50];
  sprintf(charbuffer, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
  return charbuffer;
}

// Sensor wird geändert
void handleSetSensor()
{
  int id = server.arg(0).toInt();

  if (id == -1)
  {
    id = numberOfSensors;
    numberOfSensors += 1;
    if (numberOfSensors >= numberOfSensorsMax)
      return;
  }

  String new_mqtttopic = sensors[id].getTopic();
  String new_name = sensors[id].getName();
  String new_address = sensors[id].getSens_adress_string();
  float new_offset = sensors[id].getOffset();
  bool new_sw = sensors[id].getSw();
  bool new_remote = sensors[id].getRemote();

  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "name")
    {
      new_name = server.arg(i);
    }
    if (server.argName(i) == "topic")
    {
      new_mqtttopic = server.arg(i);
    }
    if (server.argName(i) == "address")
    {
      new_address = server.arg(i);
    }
    if (server.argName(i) == "offset")
    {
      new_offset = formatDOT(server.arg(i));
    }
    if (server.argName(i) == "sw")
    {
      new_sw = checkBool(server.arg(i));
    }
    if (server.argName(i) == "remote")
    {
      new_remote = checkBool(server.arg(i));
    }
    yield();
  }

  sensors[id].change(new_address, new_mqtttopic, new_name, new_offset, new_sw, new_remote);
  saveConfig();
  server.send(201, "text/plain", "created");
}

void handleDelSensor()
{
  int id = server.arg(0).toInt();

  //  Alle einen nach vorne schieben
  for (int i = id; i < numberOfSensors; i++)
  {
    if (i == (numberOfSensorsMax - 1)) // 5 - Array von 0 bis (numberOfSensorsMax-1)
    {
      sensors[i].change("", "", "", 0.0, false, false);
    }
    else
      sensors[i].change(sensors[i + 1].getSens_adress_string(), sensors[i + 1].getTopic(), sensors[i + 1].getName(), sensors[i + 1].getOffset(), sensors[i + 1].getSw(), sensors[i + 1].getRemote());

    yield();
  }

  // den letzten löschen
  numberOfSensors--;
  saveConfig();
  server.send(200, "text/plain", "deleted");
}

void handleRequestSensorAddresses()
{
  numberOfSensorsFound = searchSensors();
  int id = server.arg(0).toInt();
  String message;
  if (id != -1)
  {
    message += F("<option>");
    // message += SensorAddressToString(sensors[id].address);
    message += sensors[id].getSens_adress_string();
    message += F("</option><option disabled>──────────</option>");
  }
  for (int i = 0; i < numberOfSensorsFound; i++)
  {
    message += F("<option>");
    message += SensorAddressToString(addressesFound[i]);
    message += F("</option>");
    yield();
  }
  server.send(200, "text/html", message);
}

void handleRequestSensors()
{
  int id = server.arg(0).toInt();
  StaticJsonDocument<1024> doc;

  if (id == -1) // fetch all sensors
  {
    JsonArray sensorsArray = doc.to<JsonArray>();
    for (int i = 0; i < numberOfSensors; i++)
    {
      JsonObject sensorsObj = doc.createNestedObject();
      sensorsObj["name"] = sensors[i].getName();
      String str = sensors[i].getName();
      str.replace(" ", "%20"); // Erstze Leerzeichen für URL Charts
      sensorsObj["namehtml"] = str;
      sensorsObj["offset"] = sensors[i].getOffset();
      sensorsObj["sw"] = sensors[i].getSw();
      sensorsObj["remote"] = sensors[i].getRemote();
      sensorsObj["state"] = sensors[i].getState();
      if (sensors[i].getValue() != -127.0)
        sensorsObj["value"] = sensors[i].getTotalValueString();
      else
      {
        if (sensors[i].getErr() == 1)
          sensorsObj["value"] = "CRC";
        if (sensors[i].getErr() == 2)
          sensorsObj["value"] = "DER";
        if (sensors[i].getErr() == 3)
          sensorsObj["value"] = "UNP";
        else
          sensorsObj["value"] = "ERR";
      }
      sensorsObj["mqtt"] = sensors[i].getTopic();
      yield();
    }
  }
  else // get single sensor by id
  {
    doc["name"] = sensors[id].getName();
    doc["offset"] = sensors[id].getOffset();
    doc["sw"] = sensors[id].getSw();
    doc["remote"] = sensors[id].getRemote();
    doc["script"] = sensors[id].getTopic();
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
