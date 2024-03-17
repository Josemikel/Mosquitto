#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>
#include "DHT.h"

#define BUTTON_LEFT 0
#define DHTPIN 27 // pin 27 del ttgo
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Update these with values suitable for your network.

const char *ssid = "VELEZ";
const char *password = "71621728";
const char *mqtt_server = "192.168.1.7";

TFT_eSPI tft = TFT_eSPI();

float T = 0;
float H = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

bool toggle = true;

void isr()
{
    /*   if (toggle)
           Serial.println("on");
       else
           Serial.println("off");
   */
    toggle = !toggle;
}

void setup_wifi()
{

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("Conectado");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Llega mensaje del topic: ");
    Serial.print(topic);
    Serial.print(".. Mensaje .. ");

    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    if (messageTemp == "1")
    {
        tft.fillCircle(40, 170, 19, TFT_DARKGREEN); // rellenos CAMBIAN CON LOS SWITCH
    }
    else if (messageTemp == "0")
    {
        tft.fillCircle(40, 170, 19, TFT_BLACK);
    }
    else if (messageTemp == "3")
    {
        tft.fillCircle(95, 170, 19, TFT_BLACK);
    }
    else if (messageTemp == "4")
    {
        tft.fillCircle(95, 170, 19, TFT_PURPLE);
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {

        Serial.print("intentando conexión MQTT...");
        // Create a random client ID
        String clientId = "esp32Cliente1";

        // Attempt to connect
        if (client.connect("esp32Cliente1"))
        {
            Serial.println("conectado");
            // subscribe
            client.subscribe("esp32/led1");
            client.subscribe("esp32/led2");
        }
        else
        {
            Serial.print("fallo de conexion, rc=");
            Serial.print(client.state());
            Serial.println("intentando en 5 segundos");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    tft.init(); // display
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.drawString("Sin conexion a", 15, 100, 2);
    tft.drawString(ssid, 20, 120, 2);

    setup_wifi();

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Temperatura", 10, 18, 2);
    tft.drawString("Humedad", 10, 88, 2);
    tft.drawString("`C", 63, 40, 4);
    tft.drawString("%", 63, 110, 4);

    tft.drawCircle(40, 170, 20, TFT_GREEN); // CONTORNOS
    tft.drawCircle(95, 170, 20, TFT_MAGENTA);

    Serial.println(F("DHTxx test!")); // sensor
    dht.begin();

    attachInterrupt(BUTTON_LEFT, isr, RISING);
}

void loop()
{
    // medición de T y H

    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    tft.setTextColor(TFT_BLUE);
    tft.drawString("Conectado a", 20, 200, 2);
    tft.drawString(ssid, 20, 220, 2);

    unsigned long now = millis();
    if (now - lastMsg > 2000)
    {
        lastMsg = now;
        float H = dht.readHumidity();
        float T = dht.readTemperature();

        if (isnan(H) || isnan(T))
        { // revisar errores
            Serial.println(F("ERROR"));
            tft.setTextColor(TFT_RED);
            tft.fillRect(9, 39, 53, 30, TFT_BLACK); // borrar valor anterior
            tft.fillRect(9, 109, 53, 30, TFT_BLACK);
            tft.drawString("ERROR", 10, 45, 2); // escribir si hay error
            tft.drawString("ERROR", 10, 115, 2);
            return;
        }
        Serial.print(F("Humedad: "));
        Serial.print(H);
        Serial.print(F("%  Temperatura: "));
        Serial.print(T);
        Serial.print(F("°C "));
        Serial.println();

        tft.fillRect(9, 39, 53, 30, TFT_BLACK); // borrar valor anterior
        tft.fillRect(9, 109, 53, 30, TFT_BLACK);

        tft.setTextColor(TFT_CYAN);
        tft.drawString(String(T, 1), 10, 40, 4);
        tft.drawString(String(H, 1), 10, 110, 4);

        char Tstring[8];
        dtostrf(T, 2, 1, Tstring);
        Serial.print("Temp: ");
        Serial.println(Tstring);
        client.publish("esp32/Temperatura", Tstring);

        char Hstring[8];
        dtostrf(H, 2, 1, Hstring);
        Serial.print("Hum: ");
        Serial.println(Hstring);
        client.publish("esp32/Humedad", Hstring);

        char Togglestring[8];
        sprintf(Togglestring, "%d", toggle);
        Serial.print("Toggle: ");
        Serial.println(Togglestring);
        client.publish("esp32/sw", Togglestring);
    }
}