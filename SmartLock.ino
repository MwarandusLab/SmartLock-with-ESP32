#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiClient.h>

const char* ssid = "Smart Lock";
const char* password = "30010231";
const char* loginEmail = "kherikisia@gmail.com";
const char* loginPassword = "0748613509";

ESP8266WebServer server(80);
DNSServer dnsServer;

bool captivePortalRequested = false;
bool isAuthenticated = false;

void handleCaptivePortal() {
  if (!isAuthenticated) {
    server.send(200, "text/html", "<html><body><script>window.location.replace('http://192.168.1.1');</script></body></html>");
  } else {
    server.send(200, "text/html", "<html><body><script>window.location.replace('http://192.168.1.1/success');</script></body></html>");
  }
}


void handleRoot() {
  if (isAuthenticated) {
    handleSuccess();
  } else {
    String html = "<html><head><style>"
                  "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"
                  "form { display: inline-block; background-color: white; padding: 20px; border-radius: 5px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }"
                  "input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin: 5px 0; }"
                  "input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }"
                  "</style></head><body>"
                  "<form method='post' action='/login'>"
                  "Email: <input type='text' name='email'><br>"
                  "Password: <input type='password' name='password'><br>"
                  "<input type='submit' value='Submit'>"
                  "</form></body></html>";
    server.send(200, "text/html", html);
  }
}

void handleLogin() {
  String inputEmail = server.arg("email");
  String inputPassword = server.arg("password");

  if (inputEmail == loginEmail && inputPassword == loginPassword) {
    isAuthenticated = true;
    server.sendHeader("Location", "/success");
    server.send(302, "text/plain", "");
  } else {
    server.send(401, "text/plain", "Authentication Failed");
  }
}

void handleSuccess() {
  if (!isAuthenticated) {
    handleRoot();
    return;
  }
  String successPage = "<html><head><style>"
                       "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"
                       "h1 { color: #4CAF50; }"
                       "button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 10px; }"
                       "</style></head><body>"
                       "<h1>Login Successful</h1>"
                       "<button onclick='openRelay()'>Open</button>"
                       "<button onclick='closeRelay()'>Close</button>"
                       "<script>"
                       "function openRelay() {"
                       "  fetch('/relay?state=on');"
                       "}"
                       "function closeRelay() {"
                       "  fetch('/relay?state=off');"
                       "}"
                       "</script></body></html>";
  server.send(200, "text/html", successPage);
}

void handleRelay() {
  String relayState = server.arg("state");
  if (relayState == "on") {
    digitalWrite(5, HIGH);  // Assuming your relay is connected to GPIO 5
  } else if (relayState == "off") {
    digitalWrite(5, LOW);
  }
  server.send(200, "text/plain", "Relay state changed");
}

void setup() {
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  delay(100);
  IPAddress apIP(192, 168, 1, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  dnsServer.start(53, "*", apIP);

  server.onNotFound([]() {
    if (!captivePortalRequested) {
      handleCaptivePortal();
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/success", HTTP_GET, handleSuccess);
  server.on("/relay", HTTP_GET, handleRelay);

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  // Check if the captive portal needs to be opened
  // if (!captivePortalRequested && WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
  //   captivePortalRequested = true;
  //   handleCaptivePortal();
  // }
}
