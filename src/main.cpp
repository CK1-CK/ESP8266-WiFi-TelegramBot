// Bibliotheken
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFi-Telegram-credentials.h>

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

String url_wetter = "https://aktuell.mannheim-wetter.info/rss3.xml"; // URL Wetter-Abfrage

// Variable für das Licht
const int lightPin = LED_BUILTIN; // Am ESP8266 Pin D5
bool lightState = HIGH;           // High = LED Light on  | Low= LED Light off

// Variable für die Anzahl der Anfragen
int numNewRequests;

// Variable für den Text der Anfrage, die du sendest
String text = "";
// UserID des Absenders
String chat_id = "";
// Name des Absenders
String from_name = "";
// Variable für die Willkommensnachricht
String welcome = "";

String extractValuesfromXML(String searchStringStart, String searchStringEnd, String sourceString)
{
  int startIndex = sourceString.indexOf(searchStringStart);
  int endIndex = sourceString.indexOf(searchStringEnd, startIndex);

  return sourceString.substring(startIndex, endIndex);
}

String GetWeatherData(String weburl)
{
  HTTPClient httpCommunicator;
  String payload = "";
  if (httpCommunicator.begin(client, weburl))
  {
    int httpCode = httpCommunicator.GET(); // HTTP-Code der Response speichern

    if (httpCode == HTTP_CODE_OK)
    {
      payload = httpCommunicator.getString(); // Save Webpage Content

      // Serial.println("###Wetterdaten Start###");
      // Serial.println(payload);
      // Serial.println("###Wetterdaten Ende###");


      return payload;
    }
    else
    {
      // HTTP-Error
      String error_return = httpCommunicator.errorToString(httpCode);
      Serial.println("Http-Error: " + String(httpCode) + " " + error_return);

      // bot.sendMessage(chat_id, "HTTP Error: " + String(httpCode) + " " + error_return + " Wetterdaten konnten nicht abgefragt werden \n", "");
    }
    httpCommunicator.end(); // Close Connection
    Serial.println("HTTP Connection Closed.");
  }
  else
  {
    Serial.printf("HTTP-Verbindung konnte nicht hergestellt werden!");
    // bot.sendMessage(chat_id, "HTTP-Verbindung konnte nicht hergestellt werden! \n", "");
  }

  return String(EXIT_FAILURE);
}

// Funktion fürs Verarbeiten neuer Anfragen
void handleNewRequests(int numNewRequests)
{

  for (int i = 0; i < numNewRequests; i++)
  { // loopt durch die neuen Anfragen

    // Checkt, ob du die Anfrage gesendet hast oder jemand anderes
    chat_id = String(bot.messages[i].chat_id);
    if (chat_id != userID)
    {
      bot.sendMessage(chat_id, "Du bist nicht autorisiert!", "");
      continue;
    }

    // Anfragetext speichern
    text = bot.messages[i].text;
    Serial.println(text);

    from_name = bot.messages[i].from_name;

    if (text == "/start")
    {
      welcome = "Willkommen, " + from_name + ".\n";
      welcome += "Folgende Befehle kannst du verwenden: \n\n";
      welcome += "/lichtEin \n";
      welcome += "/lichtAus \n";
      welcome += "/status \n";
      welcome += "/wetter \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/lichtEin")
    {
      lightState = LOW;
      digitalWrite(lightPin, lightState);
      bot.sendMessage(chat_id, "Das Licht ist an.", "");
    }

    if (text == "/lichtAus")
    {
      lightState = HIGH;
      digitalWrite(lightPin, lightState);
      bot.sendMessage(chat_id, "Das Licht ist aus.", "");
    }

    if (text == "/status")
    {
      if (!digitalRead(lightPin))
      {
        bot.sendMessage(chat_id, "Das Licht ist an.", "");
      }
      else
      {
        bot.sendMessage(chat_id, "Das Licht ist aus.", "");
      }
    }

    if (text == "/wetter")
    {

      String currentWeatherData = "Aktuelle Werte: \n";
      String weather_data;

      weather_data = GetWeatherData(url_wetter);

      if (weather_data != String(EXIT_FAILURE))
      {
        // Extract WeatherValues from WebpageContent
        //currentWeatherData += extractValuesfromXML("Temperatur:", "<", weather_data) + "\n";
        //currentWeatherData += extractValuesfromXML("Taupunkt", ";", weather_data) + "\n";
        currentWeatherData += "Luftdruck: " +  extractValuesfromXML("Luftdruck", "<", weather_data) + "\n";
        //currentWeatherData += "Regen: " + extractValuesfromXML("Regen seit", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Wind:", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Vorhersage:", "<", weather_data) + "\n";

        Serial.println(currentWeatherData); // Serial Output of current weather conditions

        bot.sendMessage(chat_id, currentWeatherData, "");
      }
      else
      {
        bot.sendMessage(chat_id, " Wetterdaten konnten nicht abgefragt werden \n", "");
      }
      Serial.println("Ende Wetter!");
      bot.sendMessage(chat_id, "Ende Wetter", "");
    }
  }
}

void connect2Wifi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    for (unsigned int i = 0; i < ssids->length(); i++)
    {
      // Verbindung zum WLAN
      Serial.print("Verbinde mich mit: ");
      Serial.println(ssids[i]);

      WiFi.begin(ssids[i], passwords[i]);

      delay(10000);

      if (WiFi.status() == WL_CONNECTED)
        break;

      Serial.println("Verbindung nicht erfolgreich!");
      Serial.println("-----------------------------");
    }
  }
  Serial.println("Verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

void setup()
{

  Serial.begin(115200);
  client.setInsecure();

  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, lightState);

  connect2Wifi();
}

void loop()
{
  // checkt, ob eine neue Anfrage reinkam
  int numNewRequests = bot.getUpdates(bot.last_message_received + 1);

  while (numNewRequests)
  { // wird ausgeführt, wenn numNewRequests == 1
    Serial.println("Anfrage erhalten");
    handleNewRequests(numNewRequests);
    numNewRequests = bot.getUpdates(bot.last_message_received + 1);
  }
  delay(1000);
}