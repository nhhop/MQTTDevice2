class mqtt
{
  private:
      
  public:
    bool isEnabled;
    bool externalBroker;

    mqtt()
    {
      
    }

    void change()
    {

    }
    
    bool publish(String topic, String data, uint8_t qos=0, uint8_t retain=0)
    {
      if (!isEnabled)
        return false;

      if (externalBroker)
      {
        if (!pubsubClient.connected())
          return false;
          
        char pubsub_topic[50];
        topic.toCharArray(pubsub_topic, 50);
        char pubsub_msg[100];
        data.toCharArray(pubsub_msg, 100);
        pubsubClient.publish(pubsub_topic, pubsub_msg);
        return true;
      }
      else
        return mqttBroker.publish(topic, data);
    }

    bool subscribe(String topic, uint8_t qos=0)
    {
      if (!isEnabled)
        return false;

      if (externalBroker)
      {
        if (!pubsubClient.connected())
          return false;

        char pubsub_topic[50];
        topic.toCharArray(pubsub_topic, 50);
        pubsubClient.subscribe(pubsub_topic);

        return true;
      }
      else
        return mqttBroker.subscribe(topic);
    }

    bool unsubscribe(String topic)
    {
      if (!isEnabled)
        return false;

      if (externalBroker)
      {
        if (!pubsubClient.connected())
          return false;

        char pubsub_topic[50];
        topic.toCharArray(pubsub_topic, 50);

        pubsubClient.unsubscribe(pubsub_topic);

        return true;
      }
      else
        return mqttBroker.subscribe(topic);
    }
};

mqtt mqttConnection = mqtt();


void mqttcallback(char *topic, unsigned char *payload, unsigned int length)
{
  DEBUG_MSG("Web: Received MQTT Topic: %s ", topic);
  // Serial.print("Web: Received MQTT Topic: ");
  // Serial.println(topic);
  // Serial.print("Web: Payload: ");
  // for (int i = 0; i < length; i++)
  // {
  //   Serial.print((char)payload[i]);
  // }
  // Serial.println(" ");
  char payload_msg[length];
  for (int i = 0; i < length; i++)
  {
    payload_msg[i] = payload[i];
  }

  if (inductionCooker.mqtttopic == topic)
  {
    // if (inductionCooker.induction_state)
      // {
      inductionCooker.handlemqtt(payload_msg);
      DEBUG_MSG("%s\n", "*** Handle MQTT Induktion");
      // }
    // else
      // DEBUG_MSG("%s\n", "*** Verwerfe MQTT wegen Status Induktion (Event handling)");
  }

  for (int i = 0; i < numberOfActors; i++)
  {
    if (actors[i].mqtttopic == topic)
    {
      // if (actors[i].actor_state)
        // {
        actors[i].handlemqtt(payload_msg);
        DEBUG_MSG("%s %s\n", "*** Handle MQTT Aktor", actors[i].name_actor);
        // }
      // else
        // DEBUG_MSG("%s %s\n", "*** Verwerfe MQTT zum Aktor", actors[i].name_actor);
      yield();
    }
  }

  for (int i = 0; i < numberOfSensors; i++)
  {
    if (sensors[i].mqtttopic == topic)
    {
      // if (actors[i].actor_state)
        // {
        sensors[i].handlemqtt(payload_msg);
        DEBUG_MSG("%s %s\n", "*** Handle MQTT Aktor", sensors[i].sens_name);
        // }
      // else
        // DEBUG_MSG("%s %s\n", "*** Verwerfe MQTT zum Aktor", actors[i].name_actor);
      yield();
    }
  }
}


void handleRequestMqtt()
{
  StaticJsonDocument<512> doc;
  doc["mqtthost"] = mqtthost;
  doc["enable_mqtt"] = StopOnMQTTError;
  doc["delay_mqtt"] = wait_on_error_mqtt / 1000;
  doc["mqtt_state"] = oledDisplay.mqttOK; // Anzeige MQTT Status -> mqtt_state verzögerter Status!
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetMqtt()
{
  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "MQTTHOST")
      server.arg(i).toCharArray(mqtthost, sizeof(mqtthost));

    if (server.argName(i) == "enable_mqtt")
    {
      StopOnMQTTError = checkBool(server.arg(i));
    }
    if (server.argName(i) == "delay_mqtt")
      if (isValidInt(server.arg(i)))
      {
        wait_on_error_mqtt = server.arg(i).toInt() * 1000;
      }
    
    yield();
  }
  saveConfig();
  server.send(201, "text/plain", "created");
}