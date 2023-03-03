#include <WiFi.h>
#include <Preferences.h>

const char* ssid = "Freezone_kappa";
const char* password = "15935755";

WiFiServer server(80);

String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

Preferences preferences;
unsigned int counter;

const int output26 = 26;
const int output27 = 27;
const int output25 = 25;
const int input35 = 35;

String output26State = "off";
String output27State = "off";
String output25State = "off";
float voltValue;

TaskHandle_t pinTask;

void setupWifi(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void setupStorage(){
  preferences.begin("storage", false); // False means read+write
  counter = preferences.getUInt("counter", 0);
}

void handlePins(void* parameter){//Core 0, "second" void loop function, control of the pins
  Serial.println("handlePins");
  Serial.println(xPortGetCoreID());
  for(;;){
    float adcValue = analogRead(input35);	
    voltValue = ((adcValue * 3.3) / 4095);
    if(voltValue > 1.5){
      output25State = "on";
      digitalWrite(output25, HIGH);
    } else {
      output25State = "off";
      digitalWrite(output25, LOW);
    }
  }
}

void setupPins(){
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  pinMode(output25, OUTPUT);
  pinMode(input35, INPUT); // adcValue = analogRead(ADCPIN);  voltValue = ((adcValue * 3.3) / 4095);

  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);
  digitalWrite(output25, LOW);
  
  xTaskCreatePinnedToCore( //create "second" void loop 
      handlePins, // function that implements the task
      "Task1", // name of the task
      10000, // size of the task's stack, in words
      NULL, // parameters to pass to the task function
      0, // priority of the task
      &pinTask, // handle to the task, if needed
      0); // ID of the CPU core to run the task on (0 or 1) s
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup ");
  Serial.println(xPortGetCoreID());
  setupWifi();
  setupStorage();
  setupPins();
}

void serveWebClient(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    counter++;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /26/on") >= 0) {
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            client.println("<p>");
            client.println(counter);
            client.println("</p>");
            client.println("<p>");
            client.println("Volt value: ");
            client.println(counter);
            client.println("</p>");

            if (output26State=="off") {
              client.println("<p>PIN 26 state: OFF</p>");
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p>PIN 26 state: ON</p>");
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            if (output27State=="off") {
              client.println("<p>PIN 27 state: OFF</p>");
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p>PIN 27 state: ON</p>");
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("<p>Input pin: 35</p>");
            if (output25State=="off") {
              client.println("<p>PIN 28 state: OFF</p>");
            } else {
              client.println("<p>PIN 28 state: ON</p>");
            }
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    preferences.putUInt("counter", counter);
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void loop() {
  serveWebClient();
}
