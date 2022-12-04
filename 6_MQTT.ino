bool publish(String topic, String data, uint8_t qos=0, uint8_t retain=0)
{
  char pubsub_topic[50];
  topic.toCharArray(pubsub_topic, 50);
  char pubsub_msg[100];
  data.toCharArray(pubsub_msg, 100);
  pubsubClient.publish(pubsub_topic, pubsub_msg);
  
  return true;

  return mqttBroker.publish(topic, data);

  return false;
}

bool subscribe(String topic, uint8_t qos=0)
{
  char pubsub_topic[50];
  topic.toCharArray(pubsub_topic, 50);
  pubsubClient.subscribe(pubsub_topic);

  return true;
  
  return mqttBroker.subscribe(topic);
  
  return false;
}

bool unsubscribe(String topic)
{
  char pubsub_topic[50];
  topic.toCharArray(pubsub_topic, 50);

  pubsubClient.unsubscribe(pubsub_topic);

  return true;

  return mqttBroker.subscribe(topic);
  
  return false;
}

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