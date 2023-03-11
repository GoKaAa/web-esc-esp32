/////////////////////////////////////////////////////////////
// WebESCCont v2.0 Made by Ankush Sheoran                  //
// Copyright (c) 2023, Ankush Sheoran. All Rights Reserved //
// Functions Added:                                        //
// -> ESCIB                                                //
// -> ESC_CM                                               //
// -> Manual Mode                                          //
// -> Normal Mode                                          //
// -> Web GUI Slider Interface / AP WiFi Connection        //
/////////////////////////////////////////////////////////////
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32_Servo.h> //Servo Library to control the ESC
#define _DEBUG_PORT Serial //Define Serial Object as _DEBUG_PORT

Servo ESC;     // create servo object to control the ESC

//int potADCP = 1; //Potentiometer Analog Pin
int potValue;  // value from the analog pin

int signalOut = 15; //Signal Out Pin
int minPWM    = 1000; //Minimum Pulse Width (in ms 10^-3)
int maxPWM    = 2000; //Maximum Pulse Width (in ms 10^-3)
int pot_max   = 180;  //Maximum Potentiometer value (Should be even number)
int pot_min   = 0;    //Minimum Potentiometer value (Can be any type of number)
int esc_impulse_brake = 0; //ESC Impulse Brake Setting

//ESC Impulse Braking Configuration
int MaxImpulseForce = 180;
int MinImpulseForce = 0;
int ESCIB_EN = 1;

const char* ssid     = "esc_mgmt";
const char* password = "esc_mgmt!@#$";

String slider_value = "0";

const char* input_parameter = "value";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Web ESC Control / Calibrate</title>
  <style>
    html {font-family: Times New Roman; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 2.0rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FF0000;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background:#01070a; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #01070a; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>WebESCCont v2 ESC PWM Frequency Generator</h2>
  <p><span id="textslider_value">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min=pot_min max=pot_max value="%SLIDERVALUE%" step="1" class="slider"></p>
  <br><hr><br>
<script>
function updateSliderPWM(element) {
  var slider_value = document.getElementById("pwmSlider").value;
  document.getElementById("textslider_value").innerHTML = slider_value;
  console.log(slider_value);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+slider_value, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String processor(const String& var){
  if (var == "SLIDERVALUE"){
    return slider_value;
  }
  return String();
}

void setup(){
  _DEBUG_PORT.begin(115200);
  delay(1000);
  _DEBUG_PORT.println("WebESCCont v1.0");
  _DEBUG_PORT.println("Made by Ankush Sheoran");
  _DEBUG_PORT.println("");
  _DEBUG_PORT.print("[Servo/ESC/Attach] Attached ESC Signal Out Pin to "); _DEBUG_PORT.print(signalOut); _DEBUG_PORT.print(" with minimum pulse width set to "); _DEBUG_PORT.print(minPWM); _DEBUG_PORT.print(" and maximum pulse width set to "); _DEBUG_PORT.println(maxPWM);
  ESC.attach(signalOut,minPWM,maxPWM); // (pin, min pulse width, max pulse width in microseconds)

  Serial.println("\n[*] Creating AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  

  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam(input_parameter)) {
      message = request->getParam(input_parameter)->value();
      slider_value = message;
      potValue = slider_value.toInt();
  _DEBUG_PORT.println(""); //Give a Space
  _DEBUG_PORT.print("[Servo/ESC/Write] Writing Pot Value "); _DEBUG_PORT.println(potValue); //Print the value
  ESC.write(potValue);    // Send the signal to the ESC
    }
    else {
      message = "No message sent";
    }
    _DEBUG_PORT.println(message);
    request->send(200, "text/plain", "OK");
  });
  
  server.begin();
}
  
void loop() {
  if (esc_impulse_brake) {
  if (potValue == 0) {
    if (ESCIB_EN) {
      _DEBUG_PORT.println("[*] Impulsive Brakes Used!");
      ESC.write(MinImpulseForce);
      delay(10);
      ESC.write(MaxImpulseForce);
      delay(65);
      ESC.write(MinImpulseForce);
      ESCIB_EN = 0;
    }
  }
  if (potValue > 0) {
    ESCIB_EN = 1;
  }
  }
  if (_DEBUG_PORT.available()) {
    String datainput = _DEBUG_PORT.readString();
    datainput.trim();
    if (datainput.equals("help")){
      _DEBUG_PORT.println("mode -set normal           -> Set the Motor's Running Mode to normal Mode");
      _DEBUG_PORT.println("mode -set manual           -> Set the Motor's Running Mode to Manual Mode");
      _DEBUG_PORT.println("esc -do calibrate          -> Calibrate the ESC (Calib Type: Zero Calibration)");
      _DEBUG_PORT.println("esc impulse_brake en       -> Enable esc Impulse Braking when throttle is set to Zero");
      _DEBUG_PORT.println("esc impulse_brake dis      -> Disable esc Impulse Braking when throttle is set to Zero");
      
    }
    if (datainput.equals("mode -set")){
      _DEBUG_PORT.println("Please use the complete command (mode -set <mode>)");
    }
    if (datainput.equals("esc impulse_brake en")){
      esc_impulse_brake = 1;
      _DEBUG_PORT.println("Enabled the impulse brake option");
    }
    if (datainput.equals("esc impulse_brake dis")){
      esc_impulse_brake = 0;
      _DEBUG_PORT.println("Disabled the impulse brake option");
    }
    if (datainput.equals("mode -set normal")){
      _DEBUG_PORT.println("[*] Froce Normal Mode....");
      ESC.write(180);
      _DEBUG_PORT.println("[+] ESC / Motor in Normal Mode");
    }
    if (datainput.equals("mode -set manual")){
      _DEBUG_PORT.println("[*] Froce Manual Mode....");
      ESC.write(0);
      _DEBUG_PORT.println("[+] ESC / Motor in Manual Mode / Speed 0");
    }
    if (datainput.equals("esc -do calibrate")){
      _DEBUG_PORT.println("[*] Force ESC into Calibration Mode...");
      _DEBUG_PORT.println("[*] Set ESC Output Speed Max");
      ESC.write(180);
      _DEBUG_PORT.println("[+] Please Remove the Power Supply / Battery and Re-Connect the Battery in under 5 seconds!");
      delay(5000);
      _DEBUG_PORT.println("[*] Set ESC Output Speed 0");
      ESC.write(0);
      delay(100);
      _DEBUG_PORT.println("[+] If you are hearing a long If the long tone indicating successful calibration was heard, the ESCs are live now and if you raise the throttle a bit they should spin. Test that the motors spin by raising the throttle a bit and then lowering it again. (Do this under 10 seconds)");
      delay(10000);
      _DEBUG_PORT.println("[*] Set ESC Output Speed 0");
      ESC.write(0);
      _DEBUG_PORT.println("[+] New Disconnect the battery and connect it again after 5 seconds of disconnection to exit the calibration Mode!");
      _DEBUG_PORT.println("[+] Zero Calibration Complete!");
    }
  }
}
