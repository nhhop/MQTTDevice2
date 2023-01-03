void loop()
{
  server.handleClient();    // Webserver handle
  queueEventSystem(EM_WLAN); // Überprüfe WLAN
  queueEventSystem(EM_MQTT); // Überprüfe MQTT
  if (startMDNS)            // MDNS handle
    queueEventSystem(EM_MDNS);
  
  gEM.processAllEvents();

  if (numberOfSensors > 0)  // Sensoren Ticker
    TickerSen.update();
  if (numberOfActors > 0)   // Aktoren Ticker
    TickerAct.update();
  if (inductionStatus > 0)  // Induktion Ticker
    TickerInd.update();
  if (useDisplay)           // Display Ticker
    TickerDisp.update();

  TickerNTP.update();       // NTP Ticker
}
