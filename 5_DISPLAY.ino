class oled
{
  unsigned long lastNTPupdate = 0;

public:
  bool dispEnabled = false;
  int address = 0x3C;

  bool senOK = true;
  bool actOK = true;
  bool indOK = true;
  bool wlanOK = true;
  bool mqttOK = false;
  int counter_sen = 0;
  int counter_act = 0;

  oled()
  {
  }

  void dispUpdate()
  {
    if (dispEnabled == 1)
    {
      showDispClear();
      showDispTime(timeClient.getFormattedTime());
      showDispIP(WiFi.localIP().toString());
      showDispWlan();
      showDispMqtt();
      showDispLines();
      showDispSen();
      showDispAct();
      showDispInd();
      showDispDisplay();
    }
  }

  void change(int dispAddress, bool is_enabled)
  {
    if (is_enabled == 1 && dispAddress != 0)
    {
      address = dispAddress;
      //display.begin(SSD1306_SWITCHCAPVCC, address, true);
      //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
      display.begin(SSD1306_SWITCHCAPVCC, address);
      display.ssd1306_command(SSD1306_DISPLAYON);
      //display.setFont();
      display.clearDisplay();
      display.display();
      dispEnabled = is_enabled;
    }
    else
    {
      dispEnabled = is_enabled;
    }
  }
}

oledDisplay = oled();

void turnDisplay()
{
  if (!oledDisplay.dispEnabled)
  {
    if (oledDisplay.address != 0)
    {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      oledDisplay.dispEnabled = 0;
      TickerDisp.stop();
    }
  }
  else
  {
    if (oledDisplay.address != 0)
    {
      display.ssd1306_command(SSD1306_DISPLAYON);
      oledDisplay.dispEnabled = 1;
      TickerDisp.start();
    }
  }
}

void handleRequestDisplay()
{
  StaticJsonDocument<128> doc;
  doc["enabled"] = 0;
  doc["displayOn"] = 0;
  doc["enabled"] = oledDisplay.dispEnabled;
  doc["updisp"] = DISP_UPDATE;
  if (oledDisplay.dispEnabled == 1)
    doc["displayOn"] = 1;
  else
    doc["displayOn"] = 0;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleRequestDisp()
{
  String request = server.arg(0);
  String message;
  if (request == "isEnabled")
  {
    message = "0";
    if (oledDisplay.dispEnabled)
    {
      message = "1";
    }
    goto SendMessage;
  }
  if (request == "address")
  {
    if (isAddress(oledDisplay.address))
    {
      message += F("<option>");
      message += String(decToHex(oledDisplay.address, 2));
      message += F("</option><option disabled>──────────</option>");
    }

    for (int i = 0; i < numberOfAddress; i++)
    {
      message += F("<option>");
      message += String(decToHex(address[i], 2));
      message += F("</option>");
    }
    goto SendMessage;
  }
  if (request == "updisp")
  {
    message = DISP_UPDATE / 1000;
    goto SendMessage;
  }
SendMessage:
  server.send(200, "text/plain", message);
}

bool isAddress(int value)
{
  bool returnValue = false;
  for (int i = 0; i < numberOfAddress; i++)
  {
    if (address[i] == value)
    {
      returnValue = true;
      goto Ende;
    }
  }
Ende:
  return returnValue;
}

void handleSetDisp()
{
  String dispAddress;
  int address;
  address = oledDisplay.address;
  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "enabled")
    {
      if (server.arg(i) == "1")
      {
        oledDisplay.dispEnabled = 1;
      }
      else
      {
        oledDisplay.dispEnabled = 0;
      }
    }
    if (server.argName(i) == "address")
    {

      dispAddress = server.arg(i);
      dispAddress.remove(0, 2);
      char copy[4];
      dispAddress.toCharArray(copy, 4);
      address = strtol(copy, 0, 16);
    }
    if (server.argName(i) == "updisp")
    {
      int newdup = server.arg(i).toInt();
      if (newdup > 0)
      {
        DISP_UPDATE = newdup * 1000;
        // os_timer_disarm(&TimerDisp);
        // os_timer_arm(&TimerDisp, DISP_UPDATE, useDisplay);
        TickerDisp.stop();
        TickerDisp.config(TCP_UPDATE, 0);
      }
    }
    yield();
  }
  oledDisplay.change(address, oledDisplay.dispEnabled);
  saveConfig();
}

void dispStartScreen() // Show Startscreen
{
  if (useDisplay)
  {
    if (oledDisplay.dispEnabled == 1 && oledDisplay.address != 0)
    {
      TickerDisp.start();
      showDispClear();
      showDispCbpi();
      showDispSTA();
      showDispDisplay();
      //lastToggledDisp = millis();
    }
  }
}

/* ######### Display functions ######### */
void showDispClear() // Clear Display
{
  display.clearDisplay();
  display.display();
}

void showDispDisplay() // Show
{
  display.display();
}

void showDispVal(const String &value) // Display a String value
{
  display.print(value);
  display.display();
}

void showDispVal(const int value) // Display a Int value
{
  display.print(value);
  display.display();
}

void showDispWlan() // Show WLAN icon
{
  if (oledDisplay.wlanOK)
  {
    display.drawBitmap(77, 3, wlan_logo, 20, 20, WHITE);
  }
}
void showDispMqtt() // SHow MQTT icon
{
  if (oledDisplay.mqttOK)
  {
    display.drawBitmap(102, 3, mqtt_logo, 20, 20, WHITE);
  }
}
void showDispCbpi() // SHow CBPI icon
{
  display.clearDisplay();
  display.drawBitmap(41, 0, cbpi_logo, 50, 50, WHITE);
}

void showDispLines() // Draw lines in the bottom
{
  display.drawLine(0, 50, 128, 50, WHITE);
  display.drawLine(42, 50, 42, 64, WHITE);
  display.drawLine(84, 50, 84, 64, WHITE);
}
void showDispSen() // Show Sensor status on the left
{
  display.setTextSize(1);
  display.setCursor(3, 55);
  display.setTextColor(WHITE);
  if (numberOfSensors == 0)
  {
    display.print("S off");
    return;
  }
  if (oledDisplay.counter_sen >= numberOfSensors)
    oledDisplay.counter_sen = 0;

  display.print("S");
  display.print(oledDisplay.counter_sen + 1);
  display.print(" ");
  if (sensors[oledDisplay.counter_sen].sens_err == 0)
    display.print((int)(sensors[oledDisplay.counter_sen].sens_offset + sensors[oledDisplay.counter_sen].sens_value));
  else
    display.print("Err");

  oledDisplay.counter_sen++;
}
void showDispAct() // Show actor status in the mid
{
  display.setCursor(45, 55);
  display.setTextColor(WHITE);
  if (oledDisplay.counter_act >= numberOfActors)
    oledDisplay.counter_act = 0;

  display.print("A");
  display.print(oledDisplay.counter_act + 1);
  display.print(" ");
  if (!actors[oledDisplay.counter_act].isOn)
    display.print("off");
  else
    display.print(actors[oledDisplay.counter_act].power_actor);

  oledDisplay.counter_act++;
}
void showDispInd() // Show InductionCooker status on the right
{
  display.setTextSize(1);
  display.setCursor(87, 55);
  display.setTextColor(WHITE);
  display.print("I ");
  if (inductionStatus == 1)
  {
    if (inductionCooker.isRelayon)
      display.print(inductionCooker.newPower);
    else
      display.print("off");
  }
  else if (inductionStatus == 0)
    display.print("off");
  else
    display.print("Err");
}

void showDispTime(const String &value) // Show time value in the upper left with fontsize 2
{
  display.setCursor(5, 5);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(value);
}

void showDispIP(const String &value) // Show IP address under time value with fontsize 1
{
  display.setCursor(5, 30);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(value);
}

void showDispSet(const String &value) // Show current station mode
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(5, 30);
  display.print(value);
  display.display();
}

void showDispSet() // Show current station mode
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(1, 54);
  display.print("SET ");
  display.print(WiFi.localIP().toString());
  display.display();
}

void showDispSTA() // Show AP mode
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(8, 54);
  display.print("STA ");
  display.print(WiFi.localIP().toString());
  display.display();
}

// void timerDispCallback(void *pArg) // Timer Objekt Temperatur mit Pointer
// {
//   if (oledDisplay.dispEnabled)
//     timDisp = true; // Bei true wird im nächsten loop readTemperature ausgeführt
// }
