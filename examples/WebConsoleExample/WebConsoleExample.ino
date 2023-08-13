#include <WebConsole.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// <Web Socket Port>, <Buffer size>, <Also print to Serial>
WebConsole console(81, 50, true);

WiFiUDP ntpUDP;
WebServer server(80);
NTPClient timeClient(ntpUDP);
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone tz(myDST, mySTD);

unsigned long last_print;

// Callback function to handle specific events
String myCallbackFunction(const String& command) {
  // Process the command and generate a response
  String response = "Callback Received: " + command + "\n";
  return response;
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      console.print("WiFi Connected with IP address: ");
      console.println(WiFi.localIP().toString());

      // timeClient.setUpdateInterval(600000);
      timeClient.begin();
      timeClient.setUpdateInterval(600000);
      timeClient.update();

      // Set the current time to display the correct time in the console
      console.setEpoch(tz.toLocal(timeClient.getEpochTime()));
      console.begin();

      // Set up HTTP server routes
      server.on("/", []{ server.send(200, "text/html", console.getConsolePage()); });
      server.on("/script.js", []{ server.send(200, "application/javascript", console.getConsoleScript()); });
      server.begin();

      // Print a welcome message
      console.println("Setup done...");
      break;
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);

  delay(1000);

  console.println("==== STARTING ====");

  // Register the callback function
  console.registerCommandCallback(myCallbackFunction);

  // Initialize WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.onEvent(WiFiEvent);

  WiFi.begin(ssid, password);

  // The rest of the setup is handled in the function WiFiEvent above
}

void loop() {
  // Handle server requests
  server.handleClient();

  if (millis() - last_print > 5000) {
    last_print = millis();
    console.printf("Time to print something: %d\n", millis());
  }

  console.loop();
  if (timeClient.update())
    console.setEpoch(tz.toLocal(timeClient.getEpochTime()));

}
