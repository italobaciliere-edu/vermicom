#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <WiFiManager.h>

#define BLYNK_TEMPLATE_ID "TMPL2iRNz_w2r"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "NT6Pmyfb4tGaFUSMNzrI4trXrlQymC_r"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

/* Definicao de pinos */
#define TEMP_PIN 25
#define HUMIDITY_PIN 34
#define WATER_LEVEL_PIN 26
#define PUMP_PIN 32
#define DISPLAY_BUTTON 33

/* Inicializacao de pacotes */
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiManager wifiManager;

int temperature = 0;
int humidity = 0;
int waterLevel = 0;
bool displayMode = true;

bool offset(int prev, int next)
{
  if (prev != next && prev != next + 1 && prev != next - 1)
    return true;

  return false;
}

void printState()
{
  Serial.print("Umidade: ");
  Serial.println(humidity);

  Serial.print("Temperatura: ");
  Serial.println(temperature);

  Serial.print("Nivel da agua: ");
  Serial.println(waterLevel);

  displayMode ? Serial.println("Display desligado!") : Serial.println("Display ligado!");

  digitalRead(PUMP_PIN) ? Serial.println("Bomba ligada!") : Serial.println("Bomba desligada!");
}

void togglePump()
{
  if (!digitalRead(PUMP_PIN))
  {
    digitalWrite(PUMP_PIN, HIGH);
    Blynk.virtualWrite(V3, 1);
    Serial.println("Bomba ligada!");
  }
  else
  {
    digitalWrite(PUMP_PIN, LOW);
    Blynk.virtualWrite(V3, 0);
    Serial.println("Bomba desligada!");
  }
}

void setup()
{
  WiFi.mode(WIFI_STA);
  bool connected = wifiManager.autoConnect("VermiconAP", "vermicon@1234");

  if (!connected)
  {
    Serial.println("Failed to connect Wifi");
    ESP.restart();
  }
  else
  {
    Serial.println("Wifi Connected..");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());

  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);
  pinMode(DISPLAY_BUTTON, INPUT_PULLUP);

  Serial.begin(9600);
  sensors.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.display();
  display.setTextSize(1);
  display.clearDisplay();
}

BLYNK_WRITE(V3)
{
  togglePump();
}

void loop()
{
  Blynk.run();

  /* SeÃ§Ã£o da humidade */
  int tmpHumidity = map(analogRead(HUMIDITY_PIN), 0, 4095, 0, 100);
  if (offset(humidity, tmpHumidity))
  {
    humidity = tmpHumidity;

    Serial.print("Umidade: ");
    Serial.println(humidity);

    Blynk.virtualWrite(V0, humidity);

    if (humidity <= 60 && !digitalRead(PUMP_PIN))
    {
      digitalWrite(PUMP_PIN, HIGH);
      Blynk.virtualWrite(V3, 1);
      Serial.println("Bomba ligada!");
    }
    else if (humidity > 60 && digitalRead(PUMP_PIN))
    {
      digitalWrite(PUMP_PIN, LOW);
      Blynk.virtualWrite(V3, 0);
      Serial.println("Bomba desligada!");
    }

    display.print("Umidade: ");
    display.println(humidity);
  }

  /* SeÃ§Ã£o da temperatura */
  sensors.requestTemperatures();
  int tmpTemperature = sensors.getTempCByIndex(0);
  if (offset(temperature, tmpTemperature))
  {
    temperature = tmpTemperature;

    Serial.print("Temperatura: ");
    Serial.println(temperature);

    Blynk.virtualWrite(V2, temperature);

    display.print("Temperatura: ");
    display.println(temperature);
  }

  /* SeÃ§Ã£o do sensor de nÃ­vel */
  int tmpWaterLevel = digitalRead(WATER_LEVEL_PIN);
  if (offset(waterLevel, tmpWaterLevel))
  {
    waterLevel = tmpWaterLevel;

    Blynk.virtualWrite(V1, waterLevel);

    Serial.print("Nivel da agua: ");
    Serial.println(waterLevel);

    display.print("Estado do Tanque: ");
    if (waterLevel == 1)
    {
      display.println("Cheio");
    }
    else
    {
      display.println("Vazio");
    }
  }

  if (!digitalRead(DISPLAY_BUTTON))
  {
    if (displayMode)
    {
      // display.noBacklight();
      displayMode = false;

      Serial.println("Display desligado!");
    }
    else
    {
      // display.backlight();
      displayMode = true;

      Serial.println("Display ligado!");
    }
  }

  if (Serial.available() > 0)
  {
    String data = Serial.readString();
    if (data == "ReadState")
    {
      printState();
    }
    else if (data == "PumpToggle")
    {
      togglePump();
    }
  }

  delay(200);
}