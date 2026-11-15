#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// Wifi ssid and password
const char* ssid = "MEC_Nahor_123";
const char* password = "Nahor1234";

// Create web server on port 80
WebServer server(80);

//Setting up inputs and outputs for logic

bool Foward = false;
bool Reverse = false;
bool Left = false;
bool Right = false;
bool SCOOP_UP = false;
bool SCOOP_DOWN = false;

int Current_Motor_Speed; 
int Motor_Speed_LIMIT = 200;

const int ServoPin = 33;

const int For_Left_LEDC  = 8;
const int For_Right_LEDC = 9;
const int Rev_Left_LEDC  = 10;
const int Rev_Right_LEDC = 11;

const int FOWARD_LEFT_MOTOR  = 25;
const int FOWARD_RIGHT_MOTOR = 26; //26
const int REVERSE_LEFT_MOTOR = 27;
const int REVERSE_RIGHT_MOTOR = 32;

Servo scoopServo;

int ServoPosition = 0; 

void pwmSetup(int pin, int channel) {
    ledcSetup(channel, 1000, 8); 
    ledcAttachPin(pin, channel);
}

// HTML Controller
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<style>
    body { 
      font-family: sans-serif; 
      text-align: center; 
      margin-top: 50px; 
      user-select: none;
    }
    .btn-container {
      margin-bottom: 20px;
    }
    button {
      width: 200px;
      height: 100px;
      font-size: 24px;
      background-color: #007BFF;
      color: white;
      border: none;
      border-radius: 12px;
      user-select: none;   
      -webkit-user-select: none; 
      -webkit-touch-callout: none;
      touch-action: manipulation; 
      cursor: pointer;
    }
    button:active {
      background-color: #0056b3;
    }
</style>
</head>
<body>
<h2>ROVER Controller</h2>

<div class="btn-container">
    <button id="FORWARD_Button">W</button>
</div>
  
<div class="btn-container">
  <button id="LEFT_Button">A</button>
   <button id="REVERSE_Button">S</button>
   <button id="RIGHT_Button">D</button>
   <button id="SCOOP_UP_Button">UP</button>
   <button id="SCOOP_DOWN_Button">DOWN</button>
</div>

<div class="btn-container">
</div>

<script>

    function setupHoldButton(buttonId, paramName) {

        const btn = document.getElementById(buttonId);
        let isPressed = false;

        function sendState(state) {
        if (isPressed === state) return;
        isPressed = state;
        fetch('/button?' + paramName + '=' + (state ? '1' : '0'));
        }

        btn.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        sendState(true);
        });

        btn.addEventListener('pointerup', () => sendState(false));
        btn.addEventListener('pointerleave', () => sendState(false));
        btn.addEventListener('pointercancel', () => sendState(false));
    }

    setupHoldButton('FORWARD_Button', 'FowardTrue');
    setupHoldButton('REVERSE_Button', 'ReverseTrue');
    setupHoldButton('LEFT_Button', 'LeftTrue');
    setupHoldButton('RIGHT_Button', 'RightTrue');
    setupHoldButton('SCOOP_UP_Button', 'ScoopUpTrue');
    setupHoldButton('SCOOP_DOWN_Button', 'ScoopDownTrue');

</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  String html = htmlPage;
  server.send(200, "text/html", html);
}
 
//Handling of inputs from our HTML controller
void handleButtonInputs() {
    if (server.hasArg("FowardTrue")) {
        Foward = (server.arg("FowardTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(Foward ? "TRUE" : "FALSE");
    }
    if (server.hasArg("ReverseTrue")) {
        Reverse = (server.arg("ReverseTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(Reverse ? "TRUE" : "FALSE");
    }
    if (server.hasArg("LeftTrue")) {
        Left = (server.arg("LeftTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(Left ? "TRUE" : "FALSE");
    }
    if (server.hasArg("RightTrue")) {
        Right = (server.arg("RightTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(Right ? "TRUE" : "FALSE");
    }

    if (server.hasArg("ScoopUpTrue")) {
        SCOOP_UP = (server.arg("ScoopUpTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(SCOOP_UP ? "TRUE" : "FALSE");
    }

    if (server.hasArg("ScoopDownTrue")) {
        SCOOP_DOWN = (server.arg("ScoopDownTrue") == "1");
        Serial.print("Button State: ");
        Serial.println(SCOOP_DOWN ? "TRUE" : "FALSE"); 
    }
    server.send(200, "text/plain", "OK");
}

void setup() {
    //Setup serial monitor for monitoring and debuggin
    Serial.begin(9600);
  //
    pinMode(FOWARD_LEFT_MOTOR, OUTPUT);
    pinMode(FOWARD_RIGHT_MOTOR, OUTPUT);
    pinMode(REVERSE_LEFT_MOTOR, OUTPUT);
    pinMode(REVERSE_RIGHT_MOTOR, OUTPUT);

    pwmSetup(FOWARD_LEFT_MOTOR, For_Left_LEDC);
    pwmSetup(FOWARD_RIGHT_MOTOR, For_Right_LEDC);
    pwmSetup(REVERSE_LEFT_MOTOR, Rev_Left_LEDC);
    pwmSetup(REVERSE_RIGHT_MOTOR, Rev_Right_LEDC);
    // Starting server access point
    bool ok = WiFi.softAP(ssid, password, 1);
    Serial.println(ok ? "AP started" : "AP FAILED");  
    Serial.println("Access Point Started");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

    //servo setup
    ESP32PWM::allocateTimer(3);        
    scoopServo.setPeriodHertz(50);
    scoopServo.attach(ServoPin, 500, 2400);
    scoopServo.write(ServoPosition);

    // Server functions
    server.on("/", handleRoot);
    server.on("/button", handleButtonInputs);
    server.begin();
}

void loop() {
    server.handleClient();
    //When 'W' is pressed
    if (Foward) {
        ledcWrite(For_Left_LEDC, 1023);
        ledcWrite(For_Right_LEDC, 1023);
        ledcWrite(Rev_Left_LEDC, 0);
        ledcWrite(Rev_Right_LEDC, 0);
        Serial.print("foward movmement on ");
    } 
    //When 'W' is pressed
    else if (Reverse) {
        ledcWrite(For_Left_LEDC, 0);
        ledcWrite(For_Right_LEDC, 0);
        ledcWrite(Rev_Left_LEDC, 1023);
        ledcWrite(Rev_Right_LEDC, 1023);
        Serial.print("reverse movmement on ");
    } 
    //When 'W' is pressed
    else if (Right) {
        ledcWrite(For_Left_LEDC, 0);
        ledcWrite(For_Right_LEDC, 1023);
        ledcWrite(Rev_Left_LEDC, 1023);
        ledcWrite(Rev_Right_LEDC, 0);
        Serial.print("left movmement on ");
    } 
    //When 'W' is pressed
    else if (Left) {
        ledcWrite(For_Left_LEDC, 1023);
        ledcWrite(For_Right_LEDC, 0);
        ledcWrite(Rev_Left_LEDC, 0);
        ledcWrite(Rev_Right_LEDC, 1023);
        Serial.print("right movmement on ");
    } 
    else {
        ledcWrite(For_Left_LEDC, 0);
        ledcWrite(For_Right_LEDC, 0);
        ledcWrite(Rev_Left_LEDC, 0);
        ledcWrite(Rev_Right_LEDC, 0);

    }

    //When 'UP' is pressed
    if (SCOOP_UP)
    {
    if (ServoPosition < 180) { // Ensure the position does not exceed 180 degrees
        ServoPosition = ServoPosition + 5; // Increment the servo position
        scoopServo.write(ServoPosition); // Move the servo to the new position
        Serial.print(ServoPosition);
        delay(50); // Small delay to allow the servo to reach the position
        }
        Serial.println("SERVO MOVING UP");
    }

    //When 'DOWN' is pressed
    if (SCOOP_DOWN)
    {
        if (ServoPosition > 0) 
        { // Ensure the position does not go below 0 degrees
            ServoPosition = ServoPosition - 5; // Decrement the servo position
            scoopServo.write(ServoPosition); // Move the servo to the new position
            Serial.print(ServoPosition);
            delay(50); // Small delay to allow the servo to reach the position
        }
        Serial.println("SERVO MOVING DOWN");
    }

}