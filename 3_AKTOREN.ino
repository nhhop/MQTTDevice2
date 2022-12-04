class Actor
{
  unsigned long powerLast; // Zeitmessung für High oder Low
  int dutycycle_actor = 5000;
  unsigned char OFF;
  unsigned char ON;

public:
  bool remote = false;
  unsigned char pin_actor = 9; // the number of the LED pin
  String mqtttopic;
  String name;
  unsigned char power_actor;
  bool isOn;
  bool isInverted = false;
  bool switchable;              // actors switchable on error events?
  bool isOnBeforeError = false; // isOn status before error event
  bool state = true;      // Error state actor

  // MQTT Publish
  char actor_mqtttopic[50]; // Für MQTT Kommunikation

  Actor(String pin, String argument, String aname, bool ainverted, bool aswitchable, bool remote)
  {
    change(pin, argument, aname, ainverted, aswitchable, remote);
  }

  void Update()
  {
    if (isPin(pin_actor))
    {
      if (isOn && power_actor > 0)
      {
        if (millis() > powerLast + dutycycle_actor)
        {
          powerLast = millis();
        }
        if (millis() > powerLast + (dutycycle_actor * power_actor / 100L))
        {
          digitalWrite(pin_actor, OFF);
        }
        else
        {
          digitalWrite(pin_actor, ON);
        }
      }
      else
      {
        digitalWrite(pin_actor, OFF);
      }
    }
  }

  void change(const String &pin, const String &argument, const String &aname, const bool &ainverted, const bool &aswitchable, const bool &new_remote)
  {
    // Set PIN
    if (isPin(pin_actor))
    {
      digitalWrite(pin_actor, HIGH);
      pins_used[pin_actor] = false;
      millis2wait(10);
    }

    pin_actor = StringToPin(pin);
    if (isPin(pin_actor))
    {
      pinMode(pin_actor, OUTPUT);
      digitalWrite(pin_actor, HIGH);
      pins_used[pin_actor] = true;
    }

    isOn = false;
    name = aname;
    if (mqtttopic != argument)
    {
      mqtt_unsubscribe();
      mqtttopic = argument;
      mqtt_subscribe();

      // MQTT Publish
      // argument.toCharArray(actor_mqtttopic, argument.length() + 1);
    }
    if (ainverted)
    {
      isInverted = true;
      ON = HIGH;
      OFF = LOW;
    }
    else
    {
      isInverted = false;
      ON = LOW;
      OFF = HIGH;
    }
    switchable = aswitchable;
    state = true;
    isOnBeforeError = false;
    remote = new_remote;
  }

  //MQTT Publish
  void publishmqtt() {
    if (pubsubClient.connected()) {
      StaticJsonDocument<256> doc;
      JsonObject actorObj = doc.createNestedObject("Actor");
      if (isOn) {
        doc["State"] = "on";
        doc["power"] = String(power_actor);
      }
      else
        doc["State"] = "off";

      char jsonMessage[100];
      serializeJson(doc, jsonMessage);
      char topic[50];
      mqtttopic.toCharArray(topic, 50);
      pubsubClient.publish(topic, jsonMessage);
    }
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

  void handlemqtt(char *payload)
  {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, (const char *)payload);
    if (error)
    {
      DEBUG_MSG("Act: handlemqtt deserialize Json error %s\n", error.c_str());
      return;
    }
    if (doc["state"] == "off")
    {
      isOn = false;
      power_actor = 0;
      return;
    }
    if (doc["state"] == "on")
    {
      int newpower = doc["power"];
      isOn = true;
      power_actor = min(100, newpower);
      power_actor = max(0, newpower);
      return;
    }
  }
};

// Initialisierung des Arrays max 8
Actor actors[numberOfActorsMax] = {
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false),
    Actor("", "", "", false, false, false)};

// Funktionen für Loop im Timer Objekt
void handleActors()
{
  for (int i = 0; i < numberOfActors; i++)
  {
    actors[i].Update();
    yield();
  }
}

/* Funktionen für Web */
void handleRequestActors()
{
  int id = server.arg(0).toInt();
  StaticJsonDocument<1024> doc;
  if (id == -1) // fetch all sensors
  {
    JsonArray actorsArray = doc.to<JsonArray>();

    for (int i = 0; i < numberOfActors; i++)
    {
      JsonObject actorsObj = doc.createNestedObject();

      actorsObj["name"] = actors[i].name;
      actorsObj["status"] = actors[i].isOn;
      actorsObj["power"] = actors[i].power_actor;
      actorsObj["mqtt"] = actors[i].mqtttopic;
      actorsObj["pin"] = PinToString(actors[i].pin_actor);
      actorsObj["sw"] = actors[i].switchable;
      actorsObj["state"] = actors[i].state;
      actorsObj["remote"] = actors[i].remote;
      yield();
    }
  }
  else
  {
    doc["name"] = actors[id].name;
    doc["mqtt"] = actors[id].mqtttopic;
    doc["sw"] = actors[id].switchable;
    doc["inv"] = actors[id].isInverted;
    doc["remote"] = actors[id].remote;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetActor()
{
  int id = server.arg(0).toInt();

  if (id == -1)
  {
    id = numberOfActors;
    numberOfActors += 1;
    if (numberOfActors >= numberOfActorsMax)
      return;
  }

  String ac_pin = PinToString(actors[id].pin_actor);
  String ac_argument = actors[id].mqtttopic;
  String ac_name = actors[id].name;
  bool ac_isinverted = actors[id].isInverted;
  bool ac_switchable = actors[id].switchable;
  bool ac_remote = actors[id].remote;

  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "name")
    {
      ac_name = server.arg(i);
    }
    if (server.argName(i) == "pin")
    {
      ac_pin = server.arg(i);
    }
    if (server.argName(i) == "script")
    {
      ac_argument = server.arg(i);
    }
    if (server.argName(i) == "inv")
    {
      ac_isinverted = checkBool(server.arg(i));
    }
    if (server.argName(i) == "sw")
    {
      ac_switchable = checkBool(server.arg(i));
    }
    if (server.argName(i) == "remote")
    {
      ac_remote = checkBool(server.arg(i));
    }
    yield();
  }
  actors[id].change(ac_pin, ac_argument, ac_name, ac_isinverted, ac_switchable, ac_remote);
  saveConfig();
  server.send(201, "text/plain", "created");
}

void handleDelActor()
{
  int id = server.arg(0).toInt();
  for (int i = id; i < numberOfActors; i++)
  {
    if (i == (numberOfActorsMax - 1)) // 5 - Array von 0 bis (numberOfActorsMax-1)
    {
      actors[i].change("", "", "", false, false, false);
    }
    else
    {
      actors[i].change(PinToString(actors[i + 1].pin_actor), actors[i + 1].mqtttopic, actors[i + 1].name, actors[i + 1].isInverted, actors[i + 1].switchable, actors[i + 1].remote);
    }
    yield();
  }

  numberOfActors -= 1;
  saveConfig();
  server.send(200, "text/plain", "deleted");
}

void handlereqPins()
{
  int id = server.arg(0).toInt();
  String message;

  if (id != -1)
  {
    message += F("<option>");
    message += PinToString(actors[id].pin_actor);
    message += F("</option><option disabled>──────────</option>");
  }
  for (int i = 0; i < numberOfPins; i++)
  {
    if (pins_used[pins[i]] == false)
    {
      message += F("<option>");
      message += pin_names[i];
      message += F("</option>");
    }
    yield();
  }
  server.send(200, "text/plain", message);
}

unsigned char StringToPin(String pinstring)
{
  for (int i = 0; i < numberOfPins; i++)
  {
    if (pin_names[i] == pinstring)
    {
      return pins[i];
    }
  }
  return 9;
}

String PinToString(unsigned char pinbyte)
{
  for (int i = 0; i < numberOfPins; i++)
  {
    if (pins[i] == pinbyte)
    {
      return pin_names[i];
    }
  }
  return "NaN";
}

bool isPin(unsigned char pinbyte)
{
  bool returnValue = false;
  for (int i = 0; i < numberOfPins; i++)
  {
    if (pins[i] == pinbyte)
    {
      returnValue = true;
      goto Ende;
    }
  }
Ende:
  return returnValue;
}
