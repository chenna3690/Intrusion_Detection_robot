

// Developed by OMNYTRIX KABS - Uday Chaitanya
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WiFiClient.h>
#include <ESP32Servo.h>

// Replace with your network credentials
const char* ssid = "Intrusion Detection";
const char* password = "123456789";

int PIR_detector_pin = 13;     // PIR sensor pin
int flame_sensor_pin = 32;     // Flame sensor pin
int gas_sensor_pin = 33;       // MQ135 digital output pin
int trigPin = 25;              // Ultrasonic trigger
int echoPin = 26;              // Ultrasonic echo
int servoPin = 14;             // Servo control pin
int buzzerPin = 27;            // Buzzer pin

Servo myServo;
int servoAngle = 90;

AsyncWebServer server(80);

// Sensor Functions
String PIR() {
  int state = digitalRead(PIR_detector_pin);
  Serial.print("PIR: "); Serial.println(state);
  return (state == HIGH) ? "Detected" : "Not Detected";
}

String Flame() {
  int state = digitalRead(flame_sensor_pin);
  Serial.print("Flame: "); Serial.println(state);
  return (state == LOW) ? "Flame Detected" : "No Flame";
}

String Gas() {
  int state = digitalRead(gas_sensor_pin);
  Serial.print("Gas Sensor: "); Serial.println(state);
  return (state == LOW) ? "Gas Detected" : "No Gas Detected";
}

String Ultrasonic() {
  long duration;
  float distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");

  // Buzzer control based on distance
  if (distance > 0 && distance < 10) {
    digitalWrite(buzzerPin, HIGH);  // Turn buzzer ON
  } else {
    digitalWrite(buzzerPin, LOW);   // Turn buzzer OFF
  }

  return String(distance) + " cm";
}

// HTML Page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; display: inline-block; text-align: center; }
    h2 { font-size: 2.5rem; }
    p { font-size: 1.8rem; }
  </style>
</head>
<body>
  <h2>Intrusion Detection Robot</h2>
  <p><strong>Human:</strong> <span id="pir">%PIR%</span></p>
  <p><strong>Flame:</strong> <span id="flame">%FLAME%</span></p>
  <p><strong>Gas:</strong> <span id="gas">%GAS%</span></p>
  <p><strong>Distance:</strong> <span id="ultrasonic">%ULTRASONIC%</span></p>

  <h3>Servo Control</h3>
  <input type="range" min="0" max="180" value="90" id="servoSlider">
  <p>Angle: <span id="servoValue">90</span>°</p>

<script>
document.getElementById("servoSlider").addEventListener("input", function() {
  var angle = this.value;
  document.getElementById("servoValue").innerText = angle;
  fetch("/servo?angle=" + angle);
});

setInterval(function() {
  fetch('/PIR').then(r => r.text()).then(t => { document.getElementById("pir").innerHTML = t; });
  fetch('/FLAME').then(r => r.text()).then(t => { document.getElementById("flame").innerHTML = t; });
  fetch('/GAS').then(r => r.text()).then(t => { document.getElementById("gas").innerHTML = t; });
  fetch('/ULTRASONIC').then(r => r.text()).then(t => { document.getElementById("ultrasonic").innerHTML = t; });
}, 5000);
</script>
</body>
</html>
)rawliteral";

String processor(const String& var) {
  if (var == "PIR") return PIR();
  if (var == "FLAME") return Flame();
  if (var == "GAS") return Gas();
  if (var == "ULTRASONIC") return Ultrasonic();
  return String();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_detector_pin, INPUT);
  pinMode(flame_sensor_pin, INPUT);
  pinMode(gas_sensor_pin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off initially

  myServo.attach(servoPin);
  myServo.write(servoAngle); // Set initial servo position

  WiFi.softAP(ssid, password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/PIR", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", PIR().c_str());
  });
  server.on("/FLAME", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", Flame().c_str());
  });
  server.on("/GAS", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", Gas().c_str());
  });
  server.on("/ULTRASONIC", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", Ultrasonic().c_str());
  });

  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle")) {
      int angle = request->getParam("angle")->value().toInt();
      if (angle >= 0 && angle <= 180) {
        myServo.write(angle);
        servoAngle = angle; 
        request->send(200, "text/plain", "Servo moved to " + String(angle));
      } else {
        request->send(400, "text/plain", "Invalid angle. Must be 0-180.");
      }
    } else {
      request->send(400, "text/plain", "Missing angle parameter.");
    }
  });

  server.begin();
}

void loop() {
  delay(10);
}
