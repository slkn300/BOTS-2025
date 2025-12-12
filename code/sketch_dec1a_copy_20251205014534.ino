#include <TB6612_ESP32.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>

#define PIN_NEO_PIXEL_1  16  // Первая светодиодная лента
#define PIN_NEO_PIXEL_2  17  // Вторая светодиодная лента  
#define PIN_NEO_PIXEL_3  18  // Третья светодиодная лента
#define NUM_PIXELS_1     16  // Количество светодиодов на первой ленте
#define NUM_PIXELS_2     16  // Количество светодиодов на второй ленте
#define NUM_PIXELS_3     16  // Количество светодиодов на третьей ленте

// Инициализация трех светодиодных лент
Adafruit_NeoPixel NeoPixel1(NUM_PIXELS_1, PIN_NEO_PIXEL_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel NeoPixel2(NUM_PIXELS_2, PIN_NEO_PIXEL_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel NeoPixel3(NUM_PIXELS_3, PIN_NEO_PIXEL_3, NEO_GRB + NEO_KHZ800);

#define AIN1 13 // ESP32 Pin D13 to TB6612FNG Pin AIN1
#define BIN1 12 // ESP32 Pin D12 to TB6612FNG Pin BIN1
#define AIN2 14 // ESP32 Pin D14 to TB6612FNG Pin AIN2
#define BIN2 27 // ESP32 Pin D27 to TB6612FNG Pin BIN2
#define PWMA 26 // ESP32 Pin D26 to TB6612FNG Pin PWMA
#define PWMB 25 // ESP32 Pin D25 to TB6612FNG Pin PWMB
#define STBY 33 // ESP32 Pin D33 to TB6612FNG Pin STBY

const char* ssid = "iPhone Gamer";
const char* password = "12345678";

#define RELAY_PIN 15 // pin G15

int slider = 255;
int direction = 0;
int fire = 0;
int led = 0;
int relayMode = 0; // 0 - ручное, 1 - автоматическое с направлением

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar readings;

// these constants are used to allow you to make your motor configuration
// line up with function names like forward.  Value can be 1 or -1
const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000, 8, 1);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000, 8, 2);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
    body {height: 100vh;overflow: hidden;margin: 0;}
    .parent {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        grid-template-rows: repeat(2, 1fr);
        grid-column-gap: 0px;
        grid-row-gap: 0px;
        height: 100%;
        }
        
        .div1 { grid-area: 1 / 2 / 3 / 3; height: 100vh;}
        .div2 { grid-area: 1 / 1 / 2 / 2; padding: 0.5rem;}
        .div3 { grid-area: 2 / 1 / 3 / 2; }
        .parent-j {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        grid-template-rows: repeat(3, 1fr);
        grid-column-gap: 0px;
        grid-row-gap: 0px;
        height: 100%;
        /* justify-items: center;
        align-items: center; */
        }

        .div-u { grid-area: 1 / 2 / 2 / 3; border: 1px solid gray;padding:2rem 3rem; border-radius:100% 100% 0 0;/**display:flex;flex-wrap:nowrap;justify-content:center;align-items:center;**/}
        .div-l { grid-area: 2 / 1 / 3 / 2; border: 1px solid gray;padding:2.95rem 2.4rem; border-radius:100% 0 0 100%;}
        .div-d { grid-area: 3 / 2 / 4 / 3; border: 1px solid gray;padding:2rem 3rem; border-radius:0 0 100% 100%;}
        .div-r { grid-area: 2 / 3 / 3 / 4; border: 1px solid gray;padding:2.95rem 2.4rem; border-radius:0 100% 100% 0;}
        .div-s { grid-area: 2 / 2 / 3 / 3; border: 1px solid rgb(233, 80, 80);padding:3rem 2.3rem; }

        .analog {border:1px solid rgb(216, 211, 211);display: flex;border-radius:15px;justify-content: space-between;align-items: stretch;}
        .analog-value{font-size:5rem;}
        .btn-set{width: 35%;border-radius:0 10px 10px 0;border:none;background:#524FF0;color:white;font-size:1.3rem}
        .analog-data{display: flex;flex-wrap: wrap;align-content: stretch;justify-content: flex-start;padding: 0.5rem;}
        .div-slider{width:100%;}
        .slider{width: 90%;}
        .circ-btn{border-radius:100%;border:1px solid red;width:180px;height:180px;background:none;}
        @media screen and (orientation: portrait) {
            .parent {
                display: grid;
                grid-template-columns: 1fr;
                grid-template-rows: repeat(3, 1fr);
                grid-column-gap: 0px;
                grid-row-gap: 0px;
                }

                .div1 { grid-area: 3 / 1 / 4 / 2; height: auto;}
                .div2 { grid-area: 2 / 1 / 3 / 2; }
                .div3 { grid-area: 1 / 1 / 2 / 2; }
        }
</style>
<div class="parent">
    <div class="div1">
        <div class="parent-j">
            <button class="div-u" id="btn-up">F</button>
            <button class="div-l" id="btn-lt">L</button>
            <button class="div-d" id="btn-dn">B</button>
            <button class="div-r" id="btn-rt">R</button>
            <button class="div-s" id="btn-sp">STOP</button>
        </div>
    </div>
    <div class="div2">
        <div class="analog">
            <div class="analog-data"><div class="analog-value" id="slider-txt">0</div>
                <div class="div-slider"><input id="slider-val" class="slider" type="range" value="0" min="0" max="255"/></div>
            </div> 
            <button class="btn-set" id="btn-set">SET</button>
        </div>
    </div>
    <div class="div3">
        <button class="circ-btn" id="btn-fire">RELAY</button>
        <button class="circ-btn" id="btn-led">LED</button>
        <button class="circ-btn" id="btn-auto-relay">AUTO RELAY</button>
    </div>
</div>

<script>
let gateway = `ws://${window.location.hostname}/ws`;

let sliderTxt = document.querySelector("#slider-txt");
let sliderVal = document.querySelector("#slider-val");
let fireBtn = document.querySelector("#btn-fire");
let ledBtn = document.querySelector("#btn-led");
let autoRelayBtn = document.querySelector("#btn-auto-relay");
let ledState = 0;
let fireState = 0;
let autoRelayState = 0;

let websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    initButtons();
}

function initButtons() {
  // Кнопки управления с автоматическим реле
  document.querySelector('#btn-up').addEventListener('touchstart', ()=>{ 
    websocket.send(JSON.stringify({dir:11}));
    if(autoRelayState) {
      websocket.send(JSON.stringify({fire:1})); // Включаем реле при движении вперед
    }
  });
  document.querySelector('#btn-up').addEventListener('touchend', ()=>{ 
    websocket.send(JSON.stringify({dir:12}));
    if(autoRelayState) {
      websocket.send(JSON.stringify({fire:0})); // Выключаем реле при остановке
    }
  });

  document.querySelector('#btn-dn').addEventListener('touchstart', ()=>{ 
    websocket.send(JSON.stringify({dir:21}));
    if(autoRelayState) {
      websocket.send(JSON.stringify({fire:1})); // Включаем реле при движении назад
    }
  });
  document.querySelector('#btn-dn').addEventListener('touchend', ()=>{ 
    websocket.send(JSON.stringify({dir:22}));
    if(autoRelayState) {
      websocket.send(JSON.stringify({fire:0})); // Выключаем реле при остановке
    }
  });

  document.querySelector('#btn-lt').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:31})) });
  document.querySelector('#btn-lt').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:32})) });

  document.querySelector('#btn-rt').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:41})) });
  document.querySelector('#btn-rt').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:42})) });

  document.querySelector('#btn-set').addEventListener('click', ()=>{ websocket.send(JSON.stringify({slider: parseInt(sliderVal.value)})) });
  
  document.querySelector('#btn-led').addEventListener('click', ()=>{ 
    ledState = !ledState;
    toggleBg(ledBtn, ledState);
    websocket.send(JSON.stringify({led:ledState ? 1 : 0})); 
  });
  
  document.querySelector('#btn-fire').addEventListener('click', ()=>{ 
    fireState = !fireState;
    toggleBg(fireBtn, fireState);
    websocket.send(JSON.stringify({fire:fireState ? 1 : 0})); 
  });

  document.querySelector('#btn-auto-relay').addEventListener('click', ()=>{ 
    autoRelayState = !autoRelayState;
    toggleBg(autoRelayBtn, autoRelayState);
    websocket.send(JSON.stringify({autoRelay:autoRelayState ? 1 : 0})); 
  });

  document.querySelector('#slider-val').addEventListener('change', ()=>{
    document.querySelector("#slider-txt").innerHTML = sliderVal.value;
  });
}

function toggleBg(btn, state) {
    if (state) {
        btn.style.background = '#ff0000';
    }
    else {
        btn.style.background = '#ffffff';
    }
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function getReadings(){
    websocket.send("getReadings");
}

function onMessage(event) {
    websocket.send("getReadings");
}
</script>
)rawliteral";

String getSensorReadings(){
  readings["s"] = String(slider);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    JSONVar myObject = JSON.parse((const char*)data);
    
    if (myObject.hasOwnProperty("slider")) {
      slider = (int)myObject["slider"];
    }
    else if (myObject.hasOwnProperty("fire")) {
      fire = (int)myObject["fire"];      
    }
    else if (myObject.hasOwnProperty("led")) {
      led = (int)myObject["led"];      
    }
    else if (myObject.hasOwnProperty("autoRelay")) {
      relayMode = (int)myObject["autoRelay"];
      if (relayMode == 0) {
        fire = 0; // При отключении авторежима выключаем реле
        digitalWrite(RELAY_PIN, LOW);
      }
    }
    else if (myObject.hasOwnProperty("dir")) {
      direction = (int)myObject["dir"];
      move(direction, slider);      
    }

    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
  }
}

void move(int direction, int slider) {
  if (direction == 11) { // Forward
    forward(motor1, motor2, slider);
    if (relayMode == 1) {
      digitalWrite(RELAY_PIN, HIGH);
    }
  }
  else if (direction == 12) { // Stop forward
    // Альтернатива если brake() не работает
    motor1.drive(0);
    motor2.drive(0);
    if (relayMode == 1) {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
  else if (direction == 21) { // Backward
    back(motor1, motor2, slider);
  }
  else if (direction == 22) { // Stop backward
    motor1.drive(0);
    motor2.drive(0);
  }
  else if (direction == 31) { // Left
    motor1.drive(-slider);
    motor2.drive(slider);
  }
  else if (direction == 32) { // Stop left
    motor1.drive(0);
    motor2.drive(0);
  }
  else if (direction == 41) { // Right
    motor1.drive(slider);
    motor2.drive(-slider);
  }
  else if (direction == 42) { // Stop right
    motor1.drive(0);
    motor2.drive(0);
  }
}

// Функция для управления всеми светодиодными лентами
void controlAllLEDs(bool state) {
  if (state) {
    // Включаем все светодиоды белым цветом
    for (int pixel = 0; pixel < NUM_PIXELS_1; pixel++) {          
      NeoPixel1.setPixelColor(pixel, NeoPixel1.Color(255, 255, 255));
    }
    NeoPixel1.show();
    
    for (int pixel = 0; pixel < NUM_PIXELS_2; pixel++) {          
      NeoPixel2.setPixelColor(pixel, NeoPixel2.Color(255, 255, 255));
    }
    NeoPixel2.show();
    
    for (int pixel = 0; pixel < NUM_PIXELS_3; pixel++) {          
      NeoPixel3.setPixelColor(pixel, NeoPixel3.Color(255, 255, 255));
    }
    NeoPixel3.show();
  }
  else {
    // Выключаем все светодиоды
    NeoPixel1.clear();
    NeoPixel1.show();
    
    NeoPixel2.clear();
    NeoPixel2.show();
    
    NeoPixel3.clear();
    NeoPixel3.show();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup()
{
  // Инициализация всех светодиодных лент
  NeoPixel1.begin();
  NeoPixel2.begin();
  NeoPixel3.begin();
  
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);  
  digitalWrite(RELAY_PIN, LOW); // Начальное состояние - выключено

  initWiFi();
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}

void loop()
{
  // Управляем всеми светодиодными лентами одновременно
  controlAllLEDs(led == 1);

  // Управление реле в ручном режиме
  if (relayMode == 0) {
    if (fire == 1) {
      digitalWrite(RELAY_PIN, HIGH);
    }
    else {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
  
  ws.cleanupClients();
  
  // Для отладки
  // Serial.println("Direction: " + String(direction) + " Slider: " + String(slider) + 
  //                " Fire: " + String(fire) + " LED: " + String(led) + 
  //                " RelayMode: " + String(relayMode));
}