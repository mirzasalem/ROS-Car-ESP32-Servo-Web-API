#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#define PIN_SG90 4   // Output pin used
#define BUTTON_PIN 2 // Input pin for the button
Servo sg90;

bool rotateTo180 = true; // Track whether the servo should rotate to 180° or 0°
bool jobReceived = false; // Track if job is received
bool buttonPressed = false; // Track button press state

String getUrl = ""; // Store the GET URL globally
String jobId = "";  // Store the jobId globally

const char* ssid = "CIS Tech Ltd.";
const char* password = "cis@2022#";

WebServer server(80);

void handleGetJob() {
  if (server.hasArg("job_id")) {
    jobId = server.arg("job_id");
    Serial.println("Received job_id: " + jobId);

    // Create URL for the external API GET request
    getUrl = "http://192.168.68.109:8000/jobs/" + jobId;

    // Send GET request to the external server
    HTTPClient http;
    http.begin(getUrl);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP GET Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);

      // Send the response back to the client
      server.send(200, "application/json", response);

      // Set jobReceived to true so that we wait for the button press in the loop
      jobReceived = true;

    } else {
      Serial.println("Error on HTTP GET request: " + String(httpResponseCode));
      server.send(500, "text/plain", "Error retrieving job data");
    }

    http.end();
  } else {
    server.send(400, "text/plain", "Missing job_id parameter");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connected to WiFi");
  server.on("/getjob", handleGetJob);
  server.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configure button pin as input with an internal pull-up resistor
  sg90.setPeriodHertz(50); // PWM frequency for SG90
  sg90.attach(PIN_SG90, 500, 2400); // Attach the servo with the appropriate min and max pulse widths
  sg90.write(80); // Start with the servo at 0°
}

void loop() {
  server.handleClient();

  if (jobReceived) {
    int buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == HIGH && !buttonPressed) { // Button press detected
      buttonPressed = true; // Set buttonPressed to true

      if (rotateTo180) {
        for (int pos = 80; pos <= 150; pos += 2) {
          sg90.write(pos);
          printf("%d" , pos);
          printf("\n");
          delay(100);
  }
      } else {
        
        for (int pos = 140; pos >= 80; pos -= 2) {
        sg90.write(pos);
        printf("%d", pos);
        printf("\n");
        delay(300);
  }
        
        // Send PUT request after rotating back to 0°
        HTTPClient http;
        http.begin(getUrl);

        // Prepare JSON data for the PUT request
        String jsonData = "{\"status\":1}";

        http.addHeader("Content-Type", "application/json"); // Specify content type header
        int putResponseCode = http.PUT(jsonData);

        if (putResponseCode > 0) {
          String putResponse = http.getString();
          Serial.println("HTTP PUT Response code: " + String(putResponseCode));
          Serial.println("PUT Response: " + putResponse);
        } else {
          Serial.println("Error on sending PUT: " + String(putResponseCode));
        }
        
        http.end();
        jobReceived = false; // Reset jobReceived to false after PUT request
      }

      rotateTo180 = !rotateTo180; // Toggle rotation state
    }

    if (buttonState == HIGH) { // Reset buttonPressed when button is released
      buttonPressed = false;
    }
  }
}
