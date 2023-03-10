// Bibliotheken
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFi-Telegram-credentials.h>

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

String url_weather_values = "https://aktuell.mannheim-wetter.info/rss3.xml";                                // URL Weather - Current Values
String url_weather_forecast = "https://wetterstationen.meteomedia.de/messnetz/vorhersagegrafik/107300.png"; // URL Weather Forecast

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

/// @brief Extracts Values from an XML File
/// @param searchStringStart Paramenter you search for
/// @param searchStringEnd End Char or End Sign of the Search
/// @param sourceString XML Source
/// @return Exctracted Value
String extractValuesfromXML(String searchStringStart, String searchStringEnd, String sourceString)
{
  int startIndex = sourceString.indexOf(searchStringStart);
  int endIndex = sourceString.indexOf(searchStringEnd, startIndex);

  return sourceString.substring(startIndex, endIndex);
}

/// @brief Gets weatherdate from a webpage
/// @param weburl
/// @return Weather Data as XML
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

      // Debug
      //  Serial.println("###Wetterdaten Start###");
      //  Serial.println(payload);
      //  Serial.println("###Wetterdaten Ende###");

      return payload;
    }
    else
    {
      // HTTP-Error
      String error_return = httpCommunicator.errorToString(httpCode);
      Serial.println("Http-Error: " + String(httpCode) + " " + error_return); // HTTP Error
    }
    httpCommunicator.end(); // Close Connection
    // Serial.println("HTTP Connection Closed.");
  }
  else
  {
    Serial.printf("HTTP-Verbindung konnte nicht hergestellt werden!"); // Error: No connection
  }

  return String(EXIT_FAILURE);
}

/// @brief Sends a message to Telegram Bot. Uses the global chat_id
/// @param text Message Text
/// @return Success/No Success
bool sendMessage2TelegramBot(String text)
{
  return bot.sendMessage(chat_id, text, ""); // Send Message to TelegramBot
}

/// @brief Sends a message to Telegram Bot
/// @param text Message Text
/// @param chatid ChatBot Token ID
/// @return Success/No Success
bool sendMessage2TelegramBot(String text, String chatid)
{
  return bot.sendMessage(chatid, text, ""); // Send Message to TelegramBot
}

/// @brief Manages new TelegramBot request.
/// @param numNewRequests
void handleNewRequests(int numNewRequests)
{

  for (int i = 0; i < numNewRequests; i++)
  { // loopt durch die neuen Anfragen

    // Checkt, ob du die Anfrage gesendet hast oder jemand anderes
    chat_id = String(bot.messages[i].chat_id);
    if (chat_id != userID)
    {
      sendMessage2TelegramBot("Du bist nicht autorisiert!");
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

      sendMessage2TelegramBot(welcome);
    }

    if (text == "/lichtEin")
    {
      lightState = LOW;
      digitalWrite(lightPin, lightState);
      sendMessage2TelegramBot("Das Licht ist an.");
    }

    if (text == "/lichtAus")
    {
      lightState = HIGH;
      digitalWrite(lightPin, lightState);
      sendMessage2TelegramBot("Das Licht ist aus.");
    }

    if (text == "/status")
    {
      if (!digitalRead(lightPin))
      {
        sendMessage2TelegramBot("Das Licht ist an.");
      }
      else
      {
        sendMessage2TelegramBot("Das Licht ist aus.");
      }
    }

    if (text == "/wetter")
    {

      String currentWeatherData = "Aktuelle Werte: \n";
      String weather_data;

      weather_data = GetWeatherData(url_weather_values);

      if (weather_data != String(EXIT_FAILURE))
      {
        // Extract WeatherValues from WebpageContent
        currentWeatherData += extractValuesfromXML("Temperatur:", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Taupunkt", ";", weather_data) + "\n";
        currentWeatherData += "Luftdruck: " + extractValuesfromXML("Luftdruck", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Sonne:", "<", weather_data) + "\n";
        currentWeatherData += "Regen: " + extractValuesfromXML("Regen seit", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Wind:", "<", weather_data) + "\n";
        currentWeatherData += extractValuesfromXML("Vorhersage:", "<", weather_data) + "\n";

        Serial.println(currentWeatherData); // Serial Output of current weather conditions

        sendMessage2TelegramBot(currentWeatherData);   // Send waether values to TelegramBot
        sendMessage2TelegramBot(url_weather_forecast); // Weather Forecast to TelegramBot
      }
      else
      {
        sendMessage2TelegramBot("Wetterdaten konnten nicht abgefragt werden."); // Send Error Message to TelegramBot
      }
    }
  }
}

/// @brief Connect to a Wifi Network. It is possible to save more than one Wifi Network for different locations.
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

/// @brief NodeMCU Setup Routine
void setup()
{

  Serial.begin(115200); // Serial Console
  client.setInsecure();

  pinMode(lightPin, OUTPUT);          // NodeMCU: Internal LED
  digitalWrite(lightPin, lightState); // Set internale LED

  connect2Wifi(); // Connect to Wifi
}

/// @brief Loop
void loop()
{
  int numNewRequests = bot.getUpdates(bot.last_message_received + 1); // Check is there a new Request at the TelegramBot

  while (numNewRequests) // If there is a new TelegramBot request, else do nothing.
  {
    Serial.println("Anfrage erhalten");
    handleNewRequests(numNewRequests);
    numNewRequests = bot.getUpdates(bot.last_message_received + 1);
  }
  delay(1000);
}