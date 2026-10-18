// V3.1.1: corrige orden de declaraciones y prototipos para compilacion ESP8266.
// Mantiene el comportamiento RF y la logica Jebao de V3.1.
#include <Arduino.h>
#include "techman_config.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <RF24.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <time.h>
#include <sys/time.h>

// NodeMCU ESP8266 + nRF24L01 / Si24R1 external module (3.3 V ONLY).
#define CE_PIN D2
#define CSN_PIN D1
// HC-SR04: TRIG D0(GPIO16), ECHO D8(GPIO15) via 5V->3.3V divider.
#define SONAR_TRIG D0
#define SONAR_ECHO D8
RF24 radio(CE_PIN, CSN_PIN);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
bool arduinoOtaStarted = false;
DNSServer dnsServer;
// AP en subred separada de la LAN para permitir recuperacion si falla el Wi-Fi.
const IPAddress PORTAL_IP(192, 168, 4, 1);
const char* STA_SSID = TECHMAN_WIFI_SSID;
const char* STA_PASS = TECHMAN_WIFI_PASS;
// Change before exposing the router port! HTTP Basic is NOT encryption.
const char* WEB_USER = "jebao";
const char* WEB_PASS = TECHMAN_WEB_PASS;
static bool authorized(){
  if (server.authenticate(WEB_USER, WEB_PASS)) return true;
  server.requestAuthentication(BASIC_AUTH, "JEBAO", "Credenciales requeridas");
  return false;
}
const char* AP_SSID = "JEBAO-CONTROL";
const char* AP_PASS = TECHMAN_AP_PASS;
const uint8_t address[5] = {0x34, 0x43, 0x10, 0x10, 0x01};
uint8_t payload[32] = {0xA5, 0x5F, 0x94, 0xC1};

// Evergrow B: protocolos obtenidos de capturas SPI del mando original.
// Se reutiliza el unico nRF24; cada orden retorna al control Jebao.
bool evergrowMode=false;
uint32_t evergrowPausedAt=0, egAttempts=0, egAcks=0;
bool egPowered=true, egStorm=false;
uint8_t egProfile=3; // 0 Dia, 1 Nubes, 2 Noche, 3 Custom
uint8_t egChannels[6]={6,2,6,6,5,0};
uint8_t egLastPayload[32]={0};
uint8_t egLastTxAcks=0;
uint32_t egLastCommandMs=0;
const uint8_t EG_BASE[32]={
  0x81,0x14,0x0A,0x80,0x83,0x02,0x00,0x03,
  6,2,6,6,5,0,0,0,0,2,54,18,0,0,0,0,0,0,0,0,0,0,0,0
};
static void applyEvergrowRadio();
static void applyJebaoRadio();
static void egMakePayload(uint8_t *p);
static String egHex(const uint8_t *p);
// Custom: hasta 24 puntos horarios; interpolacion lineal en ESP.
// Upload RF de 24 puntos es EXPERIMENTAL y NO demuestra persistencia en lampara.
// Los perfiles Dia/Nubes/Noche son escenas fijas, no editables.
static const uint8_t EG_MAX_POINTS=24;
struct EgPoint { uint16_t minute; uint8_t ch[6]; };
struct EgConfig { uint32_t magic; uint8_t count; uint8_t enabled; EgPoint point[EG_MAX_POINTS]; uint32_t crc; };
EgConfig egCfg;
// Estructura de EEPROM V3.1.1 para migrar horarios existentes de 12 puntos.
struct EgConfigLegacy { uint32_t magic; uint8_t count; uint8_t enabled; EgPoint point[12]; uint32_t crc; };
static uint32_t egLegacyHash(const void *c){
  const uint8_t* p=(const uint8_t*)c;uint32_t h=2166136261UL;
  for(size_t i=0;i<sizeof(EgConfigLegacy)-sizeof(uint32_t);i++){h^=p[i];h*=16777619UL;}
  return h;
}

uint32_t lastTx = 0;
bool radioReady = false;
uint32_t egLastScheduleMinute=0xFFFFFFFFUL;
uint32_t egLastClockSync=0;
bool egClockSynced=false;
static uint32_t egConfigHash(const void *config){
  const uint8_t *ptr=(const uint8_t*)config;
  uint32_t h=2166136261UL;
  for(size_t i=0;i<sizeof(EgConfig)-sizeof(uint32_t);i++){h^=ptr[i];h*=16777619UL;}
  return h;
}
static void egDefaultConfig(){
  memset(&egCfg,0,sizeof(egCfg));egCfg.magic=0x45473234UL;
  egCfg.count=2;egCfg.enabled=0;
  egCfg.point[0].minute=0;egCfg.point[1].minute=1439;
  // Por seguridad el programa no se activa ni enciende luces al arrancar.
  egCfg.crc=egConfigHash(&egCfg);
}
static void egSaveConfigMigration(){egCfg.crc=egConfigHash(&egCfg);EEPROM.put(0,egCfg);EEPROM.commit();}
static void egLoadConfig(){
  EEPROM.begin(sizeof(EgConfig)); EEPROM.get(0,egCfg);
  if(egCfg.magic!=0x45473234UL||egCfg.count<2||egCfg.count>EG_MAX_POINTS||
     egCfg.crc!=egConfigHash(&egCfg)){
    EgConfigLegacy old;EEPROM.get(0,old);
    if(old.magic==0x45473234UL && old.count>=2 && old.count<=12 && old.crc==egLegacyHash(&old)){
      egDefaultConfig();egCfg.count=old.count;egCfg.enabled=old.enabled;
      for(uint8_t i=0;i<old.count;i++)egCfg.point[i]=old.point[i];
      egSaveConfigMigration();
    }else{egDefaultConfig();return;}
  }
  for(uint8_t i=0;i<egCfg.count;i++){
    if(egCfg.point[i].minute>1439 || (i && egCfg.point[i-1].minute>=egCfg.point[i].minute)){
      egDefaultConfig();return;
    }
    for(uint8_t c=0;c<6;c++)if(egCfg.point[i].ch[c]>100){egDefaultConfig();return;}
  }
}
static void egSaveConfig(){egCfg.crc=egConfigHash(&egCfg);EEPROM.put(0,egCfg);EEPROM.commit();}
static bool egClock(time_t &t, uint16_t &minute){
  time(&t);if(t<1700000000L)return false;
  struct tm local;localtime_r(&t,&local);
  minute=(uint16_t)(local.tm_hour*60+local.tm_min);return true;
}
static void egEvaluate(uint16_t minute,uint8_t out[6]){
  // Recorrido ciclico, incluyendo la interpolacion entre el ultimo punto y el primero del dia siguiente.
  uint8_t after=0;
  while(after<egCfg.count && egCfg.point[after].minute<=minute)after++;
  uint8_t a=(after==0)?egCfg.count-1:after-1;
  uint8_t b=(after==egCfg.count)?0:after;
  int32_t ta=egCfg.point[a].minute,tb=egCfg.point[b].minute,now=minute;
  if(after==0){ta-=1440;}else if(after==egCfg.count){tb+=1440;}
  int32_t span=tb-ta;
  for(uint8_t i=0;i<6;i++){
    int32_t x=egCfg.point[a].ch[i], y=egCfg.point[b].ch[i];
    out[i]=(uint8_t)constrain(x+((y-x)*(now-ta)+(span/2))/span,0L,100L);
  }
}
static void egTransmit(const char *cmd){
  if(!radioReady)return;
  uint8_t p[32];
  // Los datos horarios se completan en egMakePayload(), basado en reloj del ESP.
  egMakePayload(p);memcpy(egLastPayload,p,32);
  bool transient=!evergrowMode;if(transient)applyEvergrowRadio();
  egLastTxAcks=0;
  for(uint8_t i=0;i<3;i++){
    if(radio.write(p,32)){++egLastTxAcks;++egAcks;}
    ++egAttempts;if(i!=2){delay(12);yield();}
  }
  if(transient){applyJebaoRadio();lastTx=millis();}
  egLastCommandMs=millis();
  Serial.printf("EVERGROW cmd=%s TX=3 ACK_RF=%u/3 (sin ACK no implica fallo) payload=%s\n",cmd,egLastTxAcks,egHex(p).c_str());
}
static void egScheduleTick(){
  if(egProfile!=3||!egCfg.enabled||!egPowered||!radioReady)return;
  time_t t;uint16_t minute;if(!egClock(t,minute))return;
  uint32_t absMinute=(uint32_t)(t/60);
  if(absMinute==egLastScheduleMinute)return;
  egLastScheduleMinute=absMinute;
  uint8_t next[6];egEvaluate(minute,next);
  bool changed=memcmp(next,egChannels,6)!=0;
  if(changed){memcpy(egChannels,next,6);}
  // Refresco periodico de la escena, incluso cuando los valores permanecen constantes.
  if(changed || (absMinute%5==0))egTransmit("custom_auto");
}
const uint16_t MIN_MS = 200, MAX_MS = 1500;
const uint32_t TX_INTERVAL_MS = 10;
uint16_t manualOnMs = 800, manualOffMs = 800;
uint16_t autoStartMs = 800, autoEndMs = 600, autoStepMs = 1;
uint16_t manualStepMs = 1;
uint32_t dwellMs = 4000;
uint16_t onMs = 800, offMs = 800, currentAutoMs = 800;
uint32_t phaseStarted = 0, stepStarted = 0, lastReport = 0;
uint32_t txOk = 0, txNoAck = 0;
uint16_t autoIndex = 0, autoTotal = 0, autoCompleted = 0;
bool phaseOn = false;
enum Mode : uint8_t { STOPPED, MANUAL, AUTO, LOCKED, FIXED };
uint8_t radioMode = 5; // tramas verificadas: 1, 2, 3, 5; modo 4 pendiente
Mode mode = STOPPED;
static void setPhase(bool on, uint32_t now);
// /v2: buscador de oscilacion periodica dominante, sin PID.
// Una sola sonda no demuestra la forma espacial de la ola en todo el acuario.
bool waveSearch=false, sensorReady=false, waveHasBest=false;
float distanceCm=0, waveCm=0, quality=0, bestScore=-1, bestWave=0, bestQuality=0;
uint16_t bestMs=0, waveStepsDone=0, waveStepsTotal=0;
uint16_t waveStartMs=800, waveEndMs=600, waveStepMs=10;
uint32_t waveDwellMs=8000, sonarLast=0, sonarLastValid=0, waveWindowStart=0;
uint32_t waveSamples=0;
// Ring buffer with actual HC-SR04 readings for remote diagnostics.
struct SonarSample { uint32_t t; uint16_t mm; };
SonarSample sonarHistory[128];
uint8_t sonarPos=0, sonarCount=0;
uint32_t lastWifiRetry=0, lastWifiDebug=0;
bool wifiWasConnected=false;
const uint32_t WIFI_RETRY_MS=10000;
float wSum=0,wSum2=0,wSin=0,wCos=0,wMin=10000,wMax=-10000;
const uint32_t SENSOR_INTERVAL_MS=35, SENSOR_TIMEOUT_US=8000, SENSOR_LOSS_MS=1800;
static void resetWaveSamples(uint32_t t){
  waveWindowStart=t; waveSamples=0; wSum=wSum2=wSin=wCos=0;wMin=10000;wMax=-10000;
}
static void waveSearchStop(){waveSearch=false;}
static void sensorTick(uint32_t now){
  if((uint32_t)(now-sonarLast)<SENSOR_INTERVAL_MS)return;
  sonarLast=now;
  digitalWrite(SONAR_TRIG,LOW);delayMicroseconds(2);
  digitalWrite(SONAR_TRIG,HIGH);delayMicroseconds(10);digitalWrite(SONAR_TRIG,LOW);
  unsigned long us=pulseIn(SONAR_ECHO,HIGH,SENSOR_TIMEOUT_US);
  if(!us){sensorReady=false;return;}
  float cm=us*0.0343f/2.0f;
  if(cm<4.0f||cm>120.0f){sensorReady=false;return;}
  distanceCm=cm;sensorReady=true;sonarLastValid=millis();
  sonarHistory[sonarPos] = {millis(), (uint16_t)(cm*10.0f+0.5f)};
  sonarPos=(sonarPos+1)%128; if(sonarCount<128)++sonarCount;
  if(!waveSearch)return;
  // Omitir primer ciclo para reducir el transitorio del cambio de frecuencia.
  uint32_t elapsed=(uint32_t)(millis()-waveWindowStart);
  uint32_t period=2UL*onMs;
  if(elapsed<period)return;
  float angle=6.28318530718f*((elapsed%period)/(float)period);
  wSum+=cm;wSum2+=cm*cm;wSin+=cm*sinf(angle);wCos+=cm*cosf(angle);
  if(cm<wMin)wMin=cm;if(cm>wMax)wMax=cm;
  ++waveSamples;
}
static void rateWaveStep(){
  if(waveSamples<20 || (millis()-sonarLastValid)>SENSOR_LOSS_MS)return;
  float mean=wSum/waveSamples;
  float variance=max(0.0f,wSum2/waveSamples-mean*mean);
  float fundamental=2.0f*sqrtf(wSin*wSin+wCos*wCos)/waveSamples;
  // Fraccion aproximada de energia de la fundamental (0..1).
  float coherence=variance>0.0001f? constrain((fundamental*fundamental)/(2.0f*variance),0.0f,1.0f):0;
  float pp=wMax-wMin;
  if(pp>12.0f || pp<0.15f)return; // evitar salpicaduras y ruido plano
  // Premia amplitud periodica y penaliza irregularidad.
  float score=fundamental*coherence;
  waveCm=pp;quality=coherence;
  if(score>bestScore){bestScore=score;bestMs=onMs;bestWave=pp;bestQuality=coherence;waveHasBest=true;}
}
static void waveAbort(bool restoreBest){
  waveSearch=false;
  if(restoreBest && waveHasBest){
    onMs=offMs=bestMs;manualOnMs=manualOffMs=bestMs;
    mode=LOCKED;setPhase(true,millis());
  } else if(mode==AUTO){ mode=STOPPED;setPhase(false,millis()); }
}
static uint32_t passed(uint32_t now, uint32_t then) { return (uint32_t)(now - then); }
static bool validTime(long ms) { return ms >= MIN_MS && ms <= MAX_MS; }
static bool validStep(long ms) { return ms == 100 || ms == 10 || ms == 1; }
static uint16_t calcTotal() {
  uint16_t dist = abs((int)autoEndMs - (int)autoStartMs);
  return 1 + (dist + autoStepMs - 1) / autoStepMs; // includes final endpoint
}
static uint16_t autoAt(uint16_t index) {
  uint16_t dist = abs((int)autoEndMs - (int)autoStartMs);
  uint32_t advance = (uint32_t)index * autoStepMs;
  if (advance > dist) advance = dist;
  return autoStartMs <= autoEndMs ? autoStartMs + advance : autoStartMs - advance;
}
static uint32_t stepDuration(uint16_t ms) {
  uint32_t cycle = 2UL * ms;
  return ((dwellMs + cycle - 1) / cycle) * cycle;
}
static void setPhase(bool on, uint32_t now) {
  phaseOn = on;
  phaseStarted = now;
  payload[2] = (radioMode == 1) ? (phaseOn ? 0x98 : 0x90) :
               (radioMode == 2) ? 0x99 : (radioMode == 3) ? 0x9B :
               (phaseOn ? 0x9C : 0x94);
}
static void manualStart() {
  waveSearchStop();
  if (radioMode == 2 || radioMode == 3) { mode = FIXED; setPhase(true,millis()); return; }
  mode = MANUAL;
  onMs = manualOnMs; offMs = manualOffMs;
  setPhase(true, millis());
}
static void stopPulses() {
  waveSearchStop();
  mode = STOPPED;
  radioMode = 5;
  setPhase(false, millis()); // 0x94 is NOT a verified motor-off command
}
static void beginAutoStep(uint16_t index, uint32_t now) {
  autoIndex = index;
  currentAutoMs = autoAt(index);
  onMs = offMs = currentAutoMs;
  stepStarted = now;
  setPhase(true, now);
}
static void autoStart() {
  waveSearchStop();
  if(radioMode != 5) return; // el barrido está validado solo para modo 5
  autoTotal = calcTotal();
  autoCompleted = 0;
  mode = AUTO;
  beginAutoStep(0, millis());
}
static bool selectRadioMode(uint8_t selected) {
  waveSearchStop();
  if(selected != 1 && selected != 2 && selected != 3 && selected != 5) return false;
  radioMode = selected;
  if(selected == 2 || selected == 3) {
    mode = FIXED;
    setPhase(true, millis());
  } else {
    manualStart();
  }
  return true;
}
static void lockFrequency() {
  waveSearchStop();
  if (mode != AUTO) return;
  // Stop sweeping, keep transmitting alternating phases with current ON/OFF.
  mode = LOCKED;
  manualOnMs = onMs;
  manualOffMs = offMs;
}
static void sendStatus() {
 if(!authorized())return;
  uint32_t now = millis();
  uint32_t left = 0;
  if (mode == AUTO) {
    uint32_t currentTarget = stepDuration(currentAutoMs);
    uint32_t e = passed(now, stepStarted);
    left = e >= currentTarget ? 0 : currentTarget - e;
    for (uint16_t i = autoIndex + 1; i < autoTotal; ++i) left += stepDuration(autoAt(i));
  }
  char hex[3]; snprintf(hex, sizeof(hex), "%02X", payload[2]);
  String j; j.reserve(480);
  j += "{\"mode\":\"";
  j += mode == AUTO ? "auto" : mode == MANUAL ? "manual" : mode == LOCKED ? "locked" : mode == FIXED ? "fixed" : "stopped";
  j += "\",\"radioMode\":" + String(radioMode);
  j += ",\"onMs\":" + String(onMs) + ",\"offMs\":" + String(offMs);
  j += ",\"manualOnMs\":" + String(manualOnMs) + ",\"manualOffMs\":" + String(manualOffMs);
  j += ",\"manualStepMs\":" + String(manualStepMs) + ",\"autoStepMs\":" + String(autoStepMs);
  j += ",\"autoStartMs\":" + String(autoStartMs) + ",\"autoEndMs\":" + String(autoEndMs);
  j += ",\"dwellMs\":" + String(dwellMs) + ",\"autoIndex\":" + String(autoIndex);
  j += ",\"autoCompleted\":" + String(autoCompleted) + ",\"autoTotal\":" + String(autoTotal);
  j += ",\"remainingMs\":" + String(left) + ",\"radio\":" + String(radioReady ? "true" : "false");
  j += ",\"packet\":\"" + String(hex) + "\",\"txOk\":" + String(txOk) + ",\"txNoAck\":" + String(txNoAck) + "}";
  server.send(200, "application/json", j);
}
static void error400(const char* msg) { server.send(400, "text/plain; charset=utf-8", msg); }
static void redirectPortal() {
  if(!authorized())return;
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.sendHeader("Cache-Control", "no-store");
  server.send(302, "text/plain", "Portal JEBAO");
}

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>JEBAO · Resonancia WiFi</title>
<style>
:root{color-scheme:dark}*{box-sizing:border-box}body{background:#0b1521;color:#eef5fc;margin:0;font:15px system-ui,Arial}
main{max-width:540px;margin:auto;padding:20px 14px 35px}.tag{font-size:12px;color:#6fe5ce;letter-spacing:2px}h1{font-size:26px;margin:8px 0 5px}
.card{background:#172535;border:1px solid #304358;border-radius:17px;padding:18px;margin:15px 0}.muted,.sub{color:#9eb1bf}.sub{font-size:12px}
.row{display:flex;gap:9px;align-items:center}.row>*{flex:1}button{border:0;padding:13px 7px;border-radius:11px;color:#09201d;background:#39d7b0;font-size:15px;font-weight:700;cursor:pointer}
button.secondary{background:#34495c;color:white}button.stop{background:#d64b62;color:white}.mini{flex:0 0 46px;font-size:22px;padding:8px 0}
.val{font-size:28px;font-weight:800;text-align:center;white-space:nowrap;font-variant-numeric:tabular-nums}button:disabled{opacity:.35;cursor:not-allowed} .modebtn{background:#304558;color:white} .modebtn.active{background:#39d7b0;color:#09201d} input[type=range]{width:100%;accent-color:#39d7b0;margin:13px 0}
label{display:block;margin:14px 0 7px}.selector{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}.selector button{padding:11px 2px;background:#304558;color:white}
.selector button.active{background:#39d7b0;color:#09201d}.hidden{display:none}.pill{background:#284b42;border-radius:20px;padding:5px 12px;color:#8ef6cb}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.kv{background:#0c1c2c;border-radius:10px;padding:12px}.kv b{display:block;font-size:21px;margin-top:5px}
.progress{background:#304358;border-radius:10px;height:11px;overflow:hidden;margin:15px 0 7px}.progress>div{height:100%;background:#39d7b0;width:0}
.note{color:#aabac9;font-size:12px;line-height:1.5}.tabs button{background:#34495c;color:white}.tabs button.active{background:#39d7b0;color:#09201d}
</style></head><body><main>
<div class="tag">CONTROL INALÁMBRICO · ESP8266</div><p><a style="color:#7cf0d0" href="/evergrow">☀ Abrir Evergrow B</a></p><h1>Techman Reef · JEBAO WiFi Controller</h1><div class="muted">Buscador de resonancia · ON/OFF</div>
<div class="card"><h3>Modo RF capturado</h3>
<div class="selector" style="grid-template-columns:repeat(5,1fr)">
<button class="modebtn" id="rf1" onclick="chooseMode(1)">1</button>
<button class="modebtn" id="rf2" onclick="chooseMode(2)">2</button>
<button class="modebtn" id="rf3" onclick="chooseMode(3)">3</button>
<button class="modebtn" id="rf4" disabled title="No tenemos una captura RF verificada para el modo 4">4 ?</button>
<button class="modebtn" id="rf5" onclick="chooseMode(5)">5</button></div>
<p class="note" id="modeNote">Modo 5: paquetes 94/9C. Modo 4 pendiente de captura.</p></div>
<div class="card"><div class="row tabs"><button id="tabManual" onclick="tab('manual')">Manual</button><button id="tabAuto" class="active" onclick="tab('auto')">Auto síncrono</button></div></div>
<section id="manual" class="hidden"><div class="card"><h3>Modo manual</h3><div class="sub">RESOLUCIÓN · BOTONES +/−</div><div class="selector" id="manualSteps"></div>
<label>Encendido (ON)</label><div class="row"><button class="mini secondary" onclick="bump('manualOn',-1)">−</button><div class="val" id="manualOnText">0,800 s</div><button class="mini secondary" onclick="bump('manualOn',1)">+</button></div>
<input id="manualOn" type="range" min="200" max="1500" step="1" value="800" oninput="previewManual()" onchange="sendManual()">
<label>Apagado (OFF)</label><div class="row"><button class="mini secondary" onclick="bump('manualOff',-1)">−</button><div class="val" id="manualOffText">0,800 s</div><button class="mini secondary" onclick="bump('manualOff',1)">+</button></div>
<input id="manualOff" type="range" min="200" max="1500" step="1" value="800" oninput="previewManual()" onchange="sendManual()">
<p class="sub">Ciclo: <b id="cycle">1,600 s</b></p><button style="width:100%" onclick="cmd('/api/manual/start')">▶ Aplicar e iniciar modo Manual</button></div></section>
<section id="auto"><div class="card"><h3>Auto · barrido síncrono</h3><p class="note">ON y OFF siempre iguales. Barrido desde Inicio hasta Final; la resolución define el paso entre pruebas.</p>
<div class="sub">RESOLUCIÓN AUTO</div><div class="selector" id="autoSteps"></div>
<label>Inicio</label><div class="row"><button class="mini secondary" onclick="bump('autoStart',-1)">−</button><div class="val" id="autoStartText">0,800 s</div><button class="mini secondary" onclick="bump('autoStart',1)">+</button></div>
<input id="autoStart" type="range" min="200" max="1500" step="1" value="800" oninput="previewAuto()" onchange="sendAuto()">
<label>Final</label><div class="row"><button class="mini secondary" onclick="bump('autoEnd',-1)">−</button><div class="val" id="autoEndText">0,600 s</div><button class="mini secondary" onclick="bump('autoEnd',1)">+</button></div>
<input id="autoEnd" type="range" min="200" max="1500" step="1" value="600" oninput="previewAuto()" onchange="sendAuto()">
<label>Velocidad Auto · duración mínima por ajuste</label><div class="row"><button class="mini secondary" onclick="bumpDwell(-500)">−</button><div class="val" id="dwellText">4,0 s</div><button class="mini secondary" onclick="bumpDwell(500)">+</button></div>
<input id="dwell" type="range" min="1000" max="30000" step="500" value="4000" oninput="previewAuto()" onchange="sendAuto()">
<div class="grid"><div class="kv">Pruebas<b id="tests">201</b></div><div class="kv">Duración estimada<b id="estimate">—</b></div></div>
<div class="row" style="margin-top:15px"><button id="startAuto" onclick="cmd('/api/auto/start')">▶ Iniciar Auto</button><button class="secondary" id="lockAuto" onclick="cmd('/api/auto/lock')">◎ Fijar frecuencia</button></div>
<p class="note">«Fijar frecuencia» detiene el barrido, pero mantiene la alternancia ON/OFF del ajuste actual. Cada ajuste completa ciclos enteros; puede durar más que el valor seleccionado.</p>
<div class="grid"><div class="kv">ON actual<b id="currentOn">—</b></div><div class="kv">OFF actual<b id="currentOff">—</b></div></div>
<div class="progress"><div id="bar"></div></div><div class="sub" id="progress">—</div><p class="sub" id="remaining">—</p></div></section>
<div class="card"><div class="row"><button class="stop" onclick="cmd('/api/stop')">■ Fijar paquete 94</button></div><p class="note">IMPORTANTE: 0x94/0x9C son estados RF observados, no comandos de parada/arranque físico validados. «Fijar 94» no garantiza detener la bomba. Usa el interruptor de alimentación para una parada de emergencia.</p></div>
<div class="card"><div>Estado: <b id="state">Conectando…</b></div><div>Radio: <b id="radio">—</b></div><div>Paquete: <b id="packet">—</b></div><div>TX ACK: <b id="ok">0</b> · TX sin ACK: <b id="fail">0</b></div></div>
<p class="note">ACK significa recepción RF, no confirmación de movimiento. La resonancia se identifica observando la ola; no hay sensor de amplitud.</p>
</main><script>
const $=id=>document.getElementById(id),fmt=v=>(v/1000).toFixed(3).replace('.',',')+' s';
let mStep=1,aStep=1,dirtyManual=false,dirtyAuto=false,tabName='auto',queue=Promise.resolve(),rfMode=5;
const clamp=v=>Math.max(200,Math.min(1500,Math.round(v)));
function tab(t){tabName=t;$('manual').classList.toggle('hidden',t!=='manual');$('auto').classList.toggle('hidden',t!=='auto');$('tabManual').classList.toggle('active',t==='manual');$('tabAuto').classList.toggle('active',t==='auto')}
async function chooseMode(n){
 try{await queue.catch(()=>{});await api('/api/rf/mode?n='+n);rfMode=n;await refresh()}
 catch(e){alert('Error al seleccionar modo: '+e.message)}
}
function selectors(){for(const [id,current,fn] of [['manualSteps',mStep,'manual'],['autoSteps',aStep,'auto']]){
 $(id).innerHTML=[100,10,1].map(v=>'<button class="'+(current===v?'active':'')+'" onclick="setStep(\''+fn+'\','+v+')">'+(v/1000).toString().replace('.',',')+' s</button>').join('');}}
function setStep(which,v){if(which==='manual'){mStep=v;dirtyManual=true;selectors();sendManual()}else{aStep=v;dirtyAuto=true;selectors();sendAuto()}}
function bump(id,dir){let step=id.startsWith('manual')?mStep:aStep;$(id).value=clamp(+$(id).value+dir*step);if(id.startsWith('manual')){previewManual();sendManual()}else{previewAuto();sendAuto()}}
function bumpDwell(delta){$('dwell').value=Math.max(1000,Math.min(30000,+$('dwell').value+delta));previewAuto();sendAuto()}
function previewManual(){dirtyManual=true;$('manualOnText').textContent=fmt(+$('manualOn').value);$('manualOffText').textContent=fmt(+$('manualOff').value);$('cycle').textContent=fmt(+$('manualOn').value+ +$('manualOff').value)}
function totalSteps(){return 1+Math.ceil(Math.abs(+$('autoStart').value- +$('autoEnd').value)/aStep)}
function stepAt(i){let begin=+$('autoStart').value,end=+$('autoEnd').value;return begin+(begin<=end?1:-1)*Math.min(Math.abs(end-begin),i*aStep)}
function estimate(){let total=0;for(let i=0,n=totalSteps();i<n;i++){let cycle=2*stepAt(i);total+=Math.ceil(+$('dwell').value/cycle)*cycle;}return total}
function duration(ms){let s=Math.ceil(ms/1000);return (Math.floor(s/3600)?Math.floor(s/3600)+' h ':'')+Math.floor(s%3600/60)+' min '+s%60+' s'}
function previewAuto(){dirtyAuto=true;$('autoStartText').textContent=fmt(+$('autoStart').value);$('autoEndText').textContent=fmt(+$('autoEnd').value);$('dwellText').textContent=(+$('dwell').value/1000).toFixed(1).replace('.',',')+' s';$('tests').textContent=totalSteps();$('estimate').textContent=duration(estimate())}
async function api(url){const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw Error(await r.text());return r.json()}
// Serialize writes, avoiding stale requests arriving out of order.
function write(url,kind){queue=queue.catch(()=>{}).then(()=>api(url));queue.then(()=>{if(kind==='manual')dirtyManual=false;if(kind==='auto')dirtyAuto=false;refresh()}).catch(e=>{alert('Error al aplicar ajuste: '+e.message)});}
function sendManual(){write('/api/manual?on='+$('manualOn').value+'&off='+$('manualOff').value+'&step='+mStep,'manual')}
function sendAuto(){write('/api/auto/config?start='+$('autoStart').value+'&end='+$('autoEnd').value+'&step='+aStep+'&dwell='+$('dwell').value,'auto')}
async function cmd(url){try{await queue.catch(()=>{});await api(url);await refresh()}catch(e){alert('Error: '+e.message)}}
async function refresh(){try{const s=await api('/api/status');
 if(!dirtyManual){$('manualOn').value=s.manualOnMs;$('manualOff').value=s.manualOffMs;mStep=s.manualStepMs;previewManual();dirtyManual=false}
 if(!dirtyAuto){$('autoStart').value=s.autoStartMs;$('autoEnd').value=s.autoEndMs;$('dwell').value=s.dwellMs;aStep=s.autoStepMs;previewAuto();dirtyAuto=false}
 rfMode=s.radioMode;
 for(const n of [1,2,3,5])$('rf'+n).classList.toggle('active',n===rfMode);
 $('modeNote').textContent=({1:'Modo 1: alternancia 98/90, tiempos configurables (comportamiento físico por verificar).',2:'Modo 2: paquete 99 constante, sin temporización ON/OFF.',3:'Modo 3: paquete 9B constante, sin temporización ON/OFF.',5:'Modo 5: alternancia 94/9C y Auto síncrono; parada física no confirmada.'})[rfMode];
 $('startAuto').disabled=rfMode!==5;
 $('lockAuto').disabled=rfMode!==5 || s.mode!=='auto';
 selectors();$('state').textContent=({auto:'Auto en curso',manual:'Manual',locked:'Frecuencia fijada',fixed:'Paquete fijo',stopped:'Paquete 94 fijo'})[s.mode]||s.mode;
 $('radio').textContent=s.radio?'Detectada':'NO detectada';$('packet').textContent=s.packet;$('ok').textContent=s.txOk;$('fail').textContent=s.txNoAck;
 $('currentOn').textContent=fmt(s.onMs);$('currentOff').textContent=fmt(s.offMs);
 $('bar').style.width=(s.autoTotal?100*s.autoCompleted/s.autoTotal:0)+'%';$('progress').textContent=s.autoCompleted+' / '+s.autoTotal+' ajustes completados';
 $('remaining').textContent=s.mode==='auto'?'Restante aprox.: '+duration(s.remainingMs):'Barrido inactivo';
 }catch(e){$('state').textContent='Sin conexión Wi-Fi'}}
previewManual();dirtyManual=false;previewAuto();dirtyAuto=false;selectors();refresh();setInterval(refresh,1500);
</script></body></html>
)HTML";

const char PAGE_V2[] PROGMEM = R"V2HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>JEBAO · Búsqueda de ola</title>
<style>:root{color-scheme:dark}*{box-sizing:border-box}body{background:#0b1521;color:#f4f8fb;margin:0;font:16px system-ui,Arial}main{max-width:550px;margin:auto;padding:20px 15px}a{color:#58dbbd}.card{background:#172535;border:1px solid #304358;border-radius:16px;padding:18px;margin:16px 0}label{display:block;margin:12px 0 5px;color:#b7c8d7}input{width:100%;padding:12px;border-radius:8px;background:#091523;border:1px solid #4e6884;color:white;font:inherit}button{padding:14px;border:0;border-radius:9px;background:#45d8b7;font-weight:700;cursor:pointer}button.alt{background:#eb6976;color:white}.row{display:flex;gap:10px}.row>*{flex:1;min-width:0}.stats{display:grid;grid-template-columns:1fr 1fr;gap:10px}.stats div{background:#0b1a29;padding:13px;border-radius:10px}.stats b{display:block;font-size:22px}.small{color:#a6b9c9;font-size:13px;line-height:1.6}</style></head><body><main><a href="/">← Controlador original</a> · <a href="/evergrow">Evergrow</a><h1>Buscar una ola dominante</h1><p class="small">El sensor en el extremo del rebosadero mide el vaivén. El programa busca la frecuencia con mayor oscilación periódica y menos irregularidades; no garantiza por sí solo una sola ola en todo el tanque.</p>
<div class="card"><div class="stats"><div>Distancia<b id="dist">—</b></div><div>Altura pico a pico<b id="height">—</b></div><div>Regularidad<b id="quality">—</b></div><div>Mejor frecuencia<b id="best">—</b></div></div><p id="status">—</p><p id="progress" class="small">—</p></div>
<div class="card"><h3>Barrido síncrono ON = OFF</h3><label>Inicio (segundos)</label><input id="start" type="number" min="0.2" max="1.5" step="0.001" value="0.8"><label>Final (segundos)</label><input id="end" type="number" min="0.2" max="1.5" step="0.001" value="0.6"><label>Resolución</label><select id="step" style="width:100%;padding:11px;background:#091523;color:white;border:1px solid #4e6884"><option value="100">0,1 s</option><option value="10" selected>0,01 s</option><option value="1">0,001 s</option></select><label>Segundos mínimos por frecuencia (mínimo efectivo: 5 ciclos)</label><input id="dwell" type="number" min="4" max="60" step="1" value="8"><div class="row" style="margin-top:18px"><button onclick="startSearch()">▶ Buscar</button><button class="alt" onclick="stopSearch()">■ Fijar mejor</button></div><p class="small" id="message"></p></div>
<div class="card small">HC-SR04: TRIG D0, ECHO D8 <b>mediante divisor 5 V → 3,3 V</b>; VCC 5 V y masa común. No pongas ECHO de 5 V directo al ESP8266. El HC-SR04 no es estanco: protégelo de salpicaduras y condensación salina. El resultado depende de colocar el sensor en un punto con movimiento de nivel, no sobre un nodo estacionario. La bomba debe estar supervisada durante el barrido.</div>
<script>const $=id=>document.getElementById(id);async function call(u){const r=await fetch(u,{cache:'no-store'});if(!r.ok)throw Error(await r.text());return r.json()}async function startSearch(){try{const qs=new URLSearchParams({start:Math.round(+$('start').value*1000),end:Math.round(+$('end').value*1000),step:$('step').value,dwell:Math.round(+$('dwell').value*1000)});await call('/api/v2/search?'+qs);$('message').textContent='Barrido iniciado'}catch(e){$('message').textContent=e.message}}async function stopSearch(){try{await call('/api/v2/lock');$('message').textContent='Mejor ajuste fijado'}catch(e){$('message').textContent=e.message}}async function refresh(){try{const s=await call('/api/v2/status');$('dist').textContent=s.distanceCm.toFixed(1)+' cm';$('height').textContent=s.waveCm.toFixed(2)+' cm';$('quality').textContent=(s.quality*100).toFixed(0)+' %';$('best').textContent=s.bestMs?s.bestMs/1000+' s':'—';$('status').textContent=s.search?'Buscando…':s.bestMs?'Mejor ajuste seleccionado':'Inactivo';$('progress').textContent=s.done+' / '+s.total+' frecuencias · Sensor '+(s.sensor?'OK':'sin lectura')+' · RF '+(s.radio?'OK':'no detectado')}catch(e){$('status').textContent='Sin conexión'}}refresh();setInterval(refresh,900)</script></main></body></html>
)V2HTML";
static void v2Status(){
 if(!authorized())return;
 String j; j.reserve(320);
 j="{\"search\":"+String(waveSearch?"true":"false")+",\"sensor\":"+String(sensorReady && millis()-sonarLastValid<SENSOR_LOSS_MS?"true":"false")+",\"radio\":"+String(radioReady?"true":"false");
 j+=",\"distanceCm\":"+String(distanceCm,2)+",\"waveCm\":"+String(waveCm,2)+",\"quality\":"+String(quality,3);
 j+=",\"bestMs\":"+String(waveHasBest?bestMs:0)+",\"bestWave\":"+String(bestWave,2)+",\"bestQuality\":"+String(bestQuality,3);
 j+=",\"done\":"+String(waveStepsDone)+",\"total\":"+String(waveStepsTotal)+",\"onMs\":"+String(onMs)+",\"offMs\":"+String(offMs)+"}";
 server.send(200,"application/json",j);
}
static void v2Search(){
 if(!authorized())return;
 if(!server.hasArg("start")||!server.hasArg("end")||!server.hasArg("step")||!server.hasArg("dwell")){error400("Faltan parametros");return;}
 long a=server.arg("start").toInt(),b=server.arg("end").toInt(),st=server.arg("step").toInt(),dw=server.arg("dwell").toInt();
 if(!validTime(a)||!validTime(b)||!validStep(st)||dw<4000||dw>60000){error400("Rango no valido");return;}
 if(!radioReady||radioMode!=5){error400("Necesita RF detectada y modo 5");return;}
 if(!sensorReady||millis()-sonarLastValid>500){error400("Sin eco valido; verifica sensor");return;}
 waveStartMs=a;waveEndMs=b;waveStepMs=st;waveDwellMs=dw;
 autoStartMs=a;autoEndMs=b;autoStepMs=st;dwellMs=dw;
 // Reinicio para no mezclar datos con barridos anteriores.
 waveHasBest=false;bestScore=-1;bestWave=bestQuality=waveCm=quality=0;bestMs=0;
 waveStepsDone=0;waveStepsTotal=calcTotal();waveSearch=true;
 autoTotal=calcTotal();autoCompleted=0;mode=AUTO;beginAutoStep(0,millis());
 resetWaveSamples(millis());v2Status();
}

// Nunca enviar por RF ambos protocolos a la vez.
// Ambas capturas usan 2440 MHz y direccion coincidente, pero protocolo distinto.
static void applyJebaoRadio(){
  radio.stopListening();
  radio.setChannel(40); radio.setDataRate(RF24_250KBPS);
  radio.setAddressWidth(5); radio.setPayloadSize(32);
  radio.setAutoAck(true); radio.setRetries(1,10);
  radio.setCRCLength(RF24_CRC_16); radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address); radio.stopListening();
}
static void applyEvergrowRadio(){
  radio.stopListening();
  radio.setChannel(40); radio.setDataRate(RF24_250KBPS);
  radio.setAddressWidth(5); radio.setPayloadSize(32);
  radio.setAutoAck(true); radio.setRetries(0,0); // A: SETUP_RETR=00 capturado en B
  radio.setCRCLength(RF24_CRC_16); radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address); radio.stopListening();
}
static void chooseController(bool useEvergrow){
  if(evergrowMode==useEvergrow)return;
  if(useEvergrow){
    evergrowPausedAt=millis();
    evergrowMode=true; // detiene inmediatamente la emision Jebao
    if(radioReady)applyEvergrowRadio();
  }else{
    const uint32_t delta=millis()-evergrowPausedAt;
    // Mantener los parametros Jebao y desplazar su cronometro; no reiniciar barrido.
    phaseStarted+=delta;
    stepStarted+=delta;
    waveWindowStart+=delta;
    sonarLastValid=millis(); // sensorTick actualizara muestras validas
    evergrowMode=false;
    if(radioReady)applyJebaoRadio();
    lastTx=millis();
  }
}
static uint8_t egChecksum(const uint8_t *p){
  uint8_t v=0x95;
  for(uint8_t i=0;i<19;i++)v^=p[i];
  return v;
}
static void egMakePayload(uint8_t *p){
  memcpy(p,EG_BASE,32);
  p[3]=egStorm?0x82:0x80;
  p[4]=(egPowered?0x80:0x00)|egProfile;
  p[5]=(egProfile==3)?0x02:0x0C;
  p[6]=(egProfile==3)?0xAA:0x00;
  for(uint8_t i=0;i<6;i++)p[8+i]=egChannels[i];
  // El navegador envia el reloj local en la misma peticion.
  if(server.hasArg("h")&&server.hasArg("m")&&server.hasArg("s")){
    p[16]=(uint8_t)constrain(server.arg("h").toInt(),0,23);
    p[17]=(uint8_t)constrain(server.arg("m").toInt(),0,59);
    p[18]=(uint8_t)constrain(server.arg("s").toInt(),0,59);
  }else{
    time_t now;uint16_t minutes;
    if(egClock(now,minutes)){
      struct tm lt; localtime_r(&now,&lt);
      p[16]=lt.tm_hour;p[17]=lt.tm_min;p[18]=lt.tm_sec;
    }else{
      uint32_t secs=millis()/1000UL;
      p[16]=0;p[17]=(secs/60UL)%60;p[18]=secs%60;
    }
  }
  p[19]=egChecksum(p);
}
// Paquete observado en capturas del mando Evergrow B al seleccionar
// una HORA CUSTOM: B2=14, B5=83, B6=hora, B7=AA, B9..B14=CH1..CH6.
// La lectura real del mando reiniciado es OTRO tipo de paquete: B2=11, B16=indice 01..1B.
// NO asumir que el paquete de lectura se pueda usar para guardar ajustes en la lampara.
// NO se ha demostrado todavia almacenamiento persistente en la lampara.
static void egMakeHourlyCustom(uint8_t *p,uint8_t hour,const uint8_t channels[6]){
  memcpy(p,EG_BASE,32);
  p[3]=0x80;p[4]=0x83;p[5]=hour;p[6]=0xAA;p[7]=0x03;
  memcpy(p+8,channels,6);
  time_t now;uint16_t minute;
  if(egClock(now,minute)){
    struct tm tmv;localtime_r(&now,&tmv);
    p[16]=tmv.tm_hour;p[17]=tmv.tm_min;p[18]=tmv.tm_sec;
  }else{p[16]=p[17]=p[18]=0;}
  p[19]=egChecksum(p);
}
static uint32_t egUploadAttempts=0,egUploadAcks=0;
static bool egUploadAll24(){
  if(!radioReady)return false;
  // Garantizar que hay exactamente un punto para cada hora del dia.
  if(egCfg.count!=24)return false;
  for(uint8_t i=0;i<24;i++)if(egCfg.point[i].minute!=uint16_t(i)*60)return false;
  bool transient=!evergrowMode;
  if(transient)applyEvergrowRadio();
  egUploadAttempts=egUploadAcks=0;
  for(uint8_t h=0;h<24;h++){
    uint8_t p[32];egMakeHourlyCustom(p,h,egCfg.point[h].ch);
    memcpy(egLastPayload,p,32);
    for(uint8_t tries=0;tries<3;tries++){
      if(radio.write(p,32)){++egUploadAcks;++egAcks;}
      ++egUploadAttempts;++egAttempts;
      delay(12);yield();
    }
  }
  if(transient){applyJebaoRadio();lastTx=millis();}
  egLastCommandMs=millis();
  Serial.printf("CUSTOM EXPERIMENTAL: 24 horas, TX=%lu ACK_RF=%lu (NO confirma guardado)\n",(unsigned long)egUploadAttempts,(unsigned long)egUploadAcks);
  return true;
}
static String egHex(const uint8_t *p){
  String out;out.reserve(100);const char* digits="0123456789ABCDEF";
  for(uint8_t i=0;i<32;i++){
    if(i)out+=' ';
    out+=digits[p[i]>>4];out+=digits[p[i]&15];
  }
  return out;
}
static void evergrowStatus(){
  if(!authorized())return;
  String j="{\"active\":"+String(evergrowMode?"true":"false")+
    ",\"radio\":"+String(radioReady?"true":"false")+
    ",\"attempts\":"+String(egAttempts)+",\"ack\":"+String(egAcks)+
    ",\"powered\":"+String(egPowered?"true":"false")+
    ",\"storm\":"+String(egStorm?"true":"false")+
    ",\"profile\":"+String(egProfile)+",\"channels\":[";
  for(uint8_t i=0;i<6;i++){if(i)j+=',';j+=String(egChannels[i]);}
  time_t et;uint16_t em;bool clockReady=egClock(et,em);
  j+="],\"scheduleEnabled\":"+String(egCfg.enabled?"true":"false")+
     ",\"clockReady\":"+String(clockReady?"true":"false")+
     ",\"minute\":"+String(clockReady?em:-1)+
     ",\"lastAcks\":"+String(egLastTxAcks)+",\"lastCommandMs\":"+String(egLastCommandMs)+
     ",\"lastPayload\":\""+egHex(egLastPayload)+"\"}";
  server.send(200,"application/json",j);
}
static void evergrowSend(){
  if(!authorized())return;
  if(!radioReady){server.send(503,"text/plain","Radio NRF24 no detectada");return;}
  String cmd=server.arg("cmd");
  if(cmd=="on") egPowered=true;
  else if(cmd=="off")egPowered=false;
  else if(cmd=="storm_on")egStorm=true;
  else if(cmd=="storm_off")egStorm=false;
  else if(cmd=="profile"){
    if(!server.hasArg("n")){server.send(400,"text/plain","Falta n");return;}
    long n=server.arg("n").toInt();
    if(n<0||n>3){server.send(400,"text/plain","Perfil invalido");return;}
    egProfile=n;egPowered=true;
    // Dia/Nubes/Noche son escenas fijas; Custom calcula seis canales segun la hora.
    const uint8_t scenes[3][6]={{50,50,50,50,50,50},{10,10,10,10,10,10},{0,0,11,0,0,0}};
    if(n<3)memcpy(egChannels,scenes[n],6);
    else if(egCfg.enabled){time_t t;uint16_t minute;
      if(egClock(t,minute)){egEvaluate(minute,egChannels);egLastScheduleMinute=(uint32_t)(t/60);}
    }
  }else if(cmd=="channel"){
    if(egProfile!=3 || egCfg.enabled){server.send(409,"text/plain","Canales bloqueados: edita el horario Custom o desactiva la programacion");return;}
    if(!server.hasArg("ch")||!server.hasArg("v")){server.send(400,"text/plain","Faltan ch/v");return;}
    long ch=server.arg("ch").toInt(),v=server.arg("v").toInt();
    if(ch<1||ch>6||v<0||v>100){server.send(400,"text/plain","Rango CH1-6 y 0-100");return;}
    egChannels[ch-1]=(uint8_t)v;
  }else {server.send(400,"text/plain","Comando desconocido");return;}
  uint8_t p[32];egMakePayload(p);memcpy(egLastPayload,p,32);
  bool transient=!evergrowMode;
  if(transient)applyEvergrowRadio();
  // Cada presion transmite tres paquetes como el mando original.
  egLastTxAcks=0;
  for(uint8_t i=0;i<3;i++){
    if(radio.write(p,32)){++egLastTxAcks;++egAcks;}
    ++egAttempts;
    if(i!=2){delay(12);yield();}
  }
  if(transient){applyJebaoRadio();lastTx=millis();}
  egLastCommandMs=millis();
  Serial.printf("EVERGROW cmd=%s TX=3 ACK_RF=%u/3 (sin ACK no implica fallo) payload=%s\n",cmd.c_str(),egLastTxAcks,egHex(p).c_str());
  String j="{\"sent\":3,\"acks\":"+String(egLastTxAcks)+
           ",\"validatedOnLamp\":false,\"payload\":\""+egHex(p)+"\"}";
  server.send(200,"application/json",j);
}
const char EG_PAGE[] PROGMEM = R"EGHTML(
<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Evergrow B · Wi-Fi</title><style>
*{box-sizing:border-box}body{background:#0b1521;color:#eef5fc;font:16px system-ui;margin:0}
main{max-width:560px;margin:auto;padding:20px 15px}.card{background:#172535;border:1px solid #304358;border-radius:14px;padding:16px;margin:14px 0}
button{padding:13px;border-radius:9px;border:0;background:#39d7b0;color:#09201d;font-weight:700;cursor:pointer}
button.alt{background:#35495d;color:white}.row{display:flex;gap:7px;align-items:center}.row>*{flex:1;min-width:0}
button.selected{outline:2px solid white}button:disabled{opacity:.45;cursor:not-allowed}a{color:#8debd1}.small{color:#b9c8d4;font-size:13px;line-height:1.5}
.channel{margin-top:15px}.channel header{display:flex;justify-content:space-between;margin-bottom:8px}
.channel input{width:100%;accent-color:#39d7b0}.step{flex:0 0 42px;font-size:20px;padding:6px}
#result{white-space:pre-wrap;overflow-wrap:anywhere}.point{border:1px solid #35495d;border-radius:10px;padding:10px;margin-top:10px}.point input{background:#0b1a29;color:white;border:1px solid #476077;border-radius:6px;padding:8px}.point input[type=number]{width:60px}.point-head{display:flex;gap:8px;align-items:center;justify-content:space-between}.point-ch{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;margin-top:10px} .point-ch label{font-size:12px;color:#b9c8d4}.chdot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px;vertical-align:middle}.point-ch input{display:block;margin-top:5px;max-width:100%}
.graph-wrap{overflow-x:auto;background:#0a1828;border:1px solid #304358;border-radius:12px;padding:10px}.graph-wrap svg{display:block;width:100%;min-width:320px;height:auto}.legend{display:flex;flex-wrap:wrap;gap:8px 13px;margin:12px 0}.legend label{display:inline-flex;align-items:center;gap:4px;font-size:12px;color:#c9d5e2;margin:0}.legend input{width:auto;accent-color:#39d7b0}.template-note{color:#f7cc80}
</style></head><body><main><a href="/">← Jebao</a><h1>Evergrow IT5080 B</h1>
<p class="small">Radio RF compartida. Cada botón intenta 3 emisiones Evergrow y después restablece Jebao. La lámpara puede ejecutar la orden sin devolver ACK; verifica visualmente el resultado.</p>
<div class="card"><div class="row"><button onclick="send('on')">Luz ON</button><button class="alt" onclick="send('off')">Luz OFF</button></div><div class="row" style="margin-top:10px"><button onclick="send('storm_on')">Tormenta ON</button><button class="alt" onclick="send('storm_off')">Tormenta OFF</button></div></div>
<div class="card"><h3>Modos</h3><div class="row"><button id="p0" onclick="profile(0)">Día</button><button id="p1" onclick="profile(1)">Nubes</button><button id="p2" onclick="profile(2)">Noche</button><button id="p3" onclick="profile(3)">Custom</button></div></div>
<div class="card" id="customCard"><h3>Custom · programación horaria</h3>
<p class="small">Añade hasta 24 horas y programa CH1–CH6 para cada una. AVISO: esta versión guarda el horario en el ESP8266, no en la memoria de la lámpara. El ESP debe permanecer encendido para ejecutar la programación. Día, Nubes y Noche permanecen fijos.</p>
<div class="row" style="margin-top:10px"><button onclick="make24()">Crear 24 horas exactas</button><button class="alt" onclick="loadCapturedPreset()">Cargar perfil del mando reiniciado</button></div>
<p class="small template-note">Perfil recuperado de la lectura completa tras reiniciar el mando (24 horas). Registros 19, 1A, 1B son Día/Nubes/Noche, no horas. Cargar NO guarda ni transmite automáticamente.</p>
<h3>Gráfica de los 6 canales</h3><div id="graphLegend" class="legend"></div><div id="scheduleGraph" class="graph-wrap"></div>
<p class="small">Puedes modificar los campos de cada hora y la gráfica se actualizará. Las líneas muestran los puntos horarios introducidos.</p>
<div id="scheduleRows"></div><div class="row" style="margin-top:10px"><button onclick="addPoint()">+ Añadir hora</button><button onclick="saveSchedule()">Guardar horario</button></div>
<div class="row" style="margin-top:10px"><button onclick="activateSchedule(true)">Activar Custom automático</button><button class="alt" onclick="activateSchedule(false)">Pausar horario</button></div>
<div class="row" style="margin-top:12px"><button onclick="upload24()">Transmitir las 24 horas a la lámpara (prueba)</button></div>
<p class="small">PRUEBA EXPERIMENTAL: la captura identifica hora y seis intensidades, pero no demuestra que la lámpara guarde el horario. Guarda el horario antes de transmitir. Para comprobar persistencia, desactiva la ejecución automática del ESP y verifica el funcionamiento autónomo de la luz.</p>
<p id="scheduleInfo" class="small">Sincronizando reloj…</p><p id="scheduleError" class="small"></p>
</div>
<div class="card"><h3>Intensidades actuales</h3><div id="channels"></div><p class="small">Día/Nubes/Noche son escenas fijas. En Custom automático modifica el horario de arriba, no estas barras.</p></div>
<div class="card"><p id="status" class="small">Conectando…</p><p id="result" class="small">Pulsa un botón y observa si responde la lámpara.</p></div>
</main><script>
const $=id=>document.getElementById(id);let state=null,busy=false;
const channelColors=['#f05b66','#54d67b','#456cff','#e1e7f0','#38c8fa','#ae73f6'];
let graphVisible=[true,true,true,true,true,true];
function channelDot(c){return '<i class="chdot" style="background:'+channelColors[c]+'"></i>'}
function renderLegend(){const el=$('graphLegend');if(!el)return;el.innerHTML=channelColors.map((color,i)=>'<label><input type="checkbox" data-series="'+i+'" '+(graphVisible[i]?'checked':'')+'>'+channelDot(i)+'CH'+(i+1)+'</label>').join('')}
function updateGraph(){const host=$('scheduleGraph');if(!host)return;const w=640,h=270,L=40,R=12,T=15,B=32,pw=w-L-R,ph=h-T-B;let sorted=points.slice().sort((a,b)=>a.t-b.t);const x=t=>L+pw*t/1440,y=v=>T+ph*(100-v)/100;let out='<svg viewBox="0 0 '+w+' '+h+'" role="img" aria-label="Intensidad de los seis canales por hora">';
for(let v=0;v<=100;v+=25){out+='<line x1="'+L+'" y1="'+y(v)+'" x2="'+(w-R)+'" y2="'+y(v)+'" stroke="#33445a" stroke-width="1"/><text x="'+(L-6)+'" y="'+(y(v)+4)+'" text-anchor="end" font-size="11" fill="#9db3c7">'+v+'%</text>'}
for(let hr=0;hr<=24;hr+=3){out+='<line x1="'+x(hr*60)+'" y1="'+T+'" x2="'+x(hr*60)+'" y2="'+(h-B)+'" stroke="#253850" stroke-width="1"/><text x="'+x(hr*60)+'" y="'+(h-9)+'" text-anchor="middle" font-size="12" fill="#b8cddd">'+String(hr).padStart(2,'0')+'</text>'}
if(sorted.length){for(let c=0;c<6;c++){if(!graphVisible[c])continue;const path=sorted.map((p,i)=>(i?'L':'M')+x(p.t).toFixed(1)+','+y(p.ch[c]).toFixed(1)).join(' ');out+='<path d="'+path+'" fill="none" stroke="'+channelColors[c]+'" stroke-width="2.5" stroke-linejoin="round"/>';for(const p of sorted)out+='<circle cx="'+x(p.t)+'" cy="'+y(p.ch[c])+'" r="2.5" fill="'+channelColors[c]+'"/>'}}
out+='</svg>';host.innerHTML=out}
function loadCapturedPreset(){collectPoints();if(!confirm('¿Cargar el perfil real leído después del reinicio del mando? Solo se modifica la vista actual, NO se guarda ni se transmite automáticamente.'))return;
// Evergrow B, captura de lectura 81 11 0A; B16=01..18 (horas 00..23), B9..B14=CH1..CH6.
// B16=19,1A,1B corresponden a Día/Nubes/Noche y NO son horas.
const values=[
  [5, 0, 0, 0, 0, 0],
  [0, 5, 0, 0, 0, 0],
  [0, 0, 10, 0, 0, 0],
  [0, 0, 0, 2, 0, 0],
  [0, 0, 0, 0, 5, 0],
  [0, 0, 0, 0, 0, 2],
  [1, 0, 0, 0, 0, 0],
  [0, 0, 0, 10, 0, 0],
  [3, 0, 0, 1, 0, 3],
  [3, 1, 3, 3, 3, 3],
  [6, 3, 6, 6, 6, 6],
  [10, 5, 10, 10, 10, 10],
  [15, 7, 15, 15, 15, 15],
  [20, 12, 20, 20, 20, 20],
  [27, 14, 27, 27, 27, 27],
  [30, 15, 30, 30, 30, 30],
  [27, 14, 27, 27, 27, 27],
  [20, 12, 20, 20, 20, 20],
  [15, 7, 15, 15, 15, 15],
  [10, 5, 10, 10, 10, 10],
  [6, 3, 6, 6, 6, 6],
  [3, 1, 3, 3, 3, 3],
  [3, 0, 0, 1, 0, 3],
  [0, 0, 0, 0, 2, 0]
];
points=values.map((ch,h)=>({t:h*60,ch:ch.slice()}));renderPoints();$('scheduleError').textContent='Perfil real del mando cargado en pantalla (24 horas). Pulsa Guardar horario para conservarlo.';}

$('channels').innerHTML=Array.from({length:6},(_,i)=>'<div class="channel"><header><strong>'+channelDot(i)+' CH'+(i+1)+'</strong><span id="v'+i+'">0 %</span></header><div class="row"><button class="step alt" data-step="-1" data-ch="'+i+'">−</button><input id="ch'+i+'" type="range" min="0" max="100" value="0" data-ch="'+i+'"><button class="step alt" data-step="1" data-ch="'+i+'">+</button></div></div>').join('');
async function req(url){const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw Error(await r.text());return r.json()}
function clock(){const d=new Date();return '&h='+d.getHours()+'&m='+d.getMinutes()+'&s='+d.getSeconds()}
function render(s){state=s;$('status').textContent='Radio: '+(s.radio?'detectada':'NO detectada')+' · ACK RF '+s.ack+'/'+s.attempts+' (puede ser 0 y funcionar) · Protocolo activo: '+(s.active?'Evergrow':'Jebao');for(let i=0;i<6;i++){$('ch'+i).value=s.channels[i];$('v'+i).textContent=s.channels[i]+' %';$('ch'+i).disabled=s.profile!==3||s.scheduleEnabled;}document.querySelectorAll('[data-step]').forEach(b=>b.disabled=s.profile!==3||s.scheduleEnabled);for(let i=0;i<4;i++)$('p'+i).classList.toggle('selected',i===s.profile);$('scheduleInfo').textContent='Horario '+(s.scheduleEnabled?'ACTIVO':'PAUSADO')+' · '+(s.clockReady?'Reloj ESP listo ('+String(Math.floor(s.minute/60)).padStart(2,'0')+':'+String(s.minute%60).padStart(2,'0')+')':'Sin hora válida: sincroniza desde esta página');}
async function refresh(){if(busy)return;try{render(await req('/api/evergrow/status'))}catch(e){$('status').textContent='Error de conexión: '+e.message}}
async function send(c,extra=''){if(busy)return;busy=true;$('result').textContent='Transmitiendo '+c+'…';try{const r=await req('/api/evergrow/send?cmd='+c+extra+clock());$('result').textContent=c+' · '+r.sent+' emisiones RF intentadas · ACK RF '+r.acks+'/3 (no confirma el estado de la luz)\nComprueba el resultado visualmente.\n'+r.payload;render(await req('/api/evergrow/status'))}catch(e){$('result').textContent='ERROR: '+e.message}finally{busy=false}}
function profile(n){send('profile','&n='+n)}
let points=[];
function timeStr(m){return String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0')}
function renderPoints(){$('scheduleRows').innerHTML=points.map((p,i)=>'<div class="point"><div class="point-head"><strong>Punto '+(i+1)+'</strong><input type="time" value="'+timeStr(p.t)+'" data-row="'+i+'" data-time><button class="alt" onclick="removePoint('+i+')">Eliminar</button></div><div class="point-ch">'+p.ch.map((v,c)=>'<label>'+channelDot(c)+'CH'+(c+1)+' (%)<input type="number" min="0" max="100" value="'+v+'" data-row="'+i+'" data-col="'+c+'"></label>').join('')+'</div></div>').join('');updateGraph()}
function collectPoints(){points.forEach((p,i)=>{const t=document.querySelector('[data-row="'+i+'"][data-time]');if(t&&t.value){const x=t.value.split(':').map(Number);p.t=x[0]*60+x[1]}for(let c=0;c<6;c++){const el=document.querySelector('[data-row="'+i+'"][data-col="'+c+'"]');if(el)p.ch[c]=Number(el.value)}})}
function addPoint(){collectPoints();if(points.length>=24){alert('Máximo 24 puntos');return}points.sort((a,b)=>a.t-b.t);let t=720;for(let i=0;i<1440;i++){if(!points.some(p=>p.t===t))break;t=(t+1)%1440}points.push({t,ch:[0,0,0,0,0,0]});renderPoints()}
function make24(){collectPoints();if(!confirm('Crear 24 puntos exactos de 00:00 a 23:00. Se interpolarán los valores de los puntos existentes; todavía no se guardará.'))return;
const old=points.slice().sort((a,b)=>a.t-b.t);
function value(m,c){if(!old.length)return 0;if(old.length===1)return old[0].ch[c];let k=old.findIndex(p=>p.t>m);let a=(k<=0?old[old.length-1]:old[k-1]),b=(k<0?old[0]:old[k]);let ta=a.t,tb=b.t,now=m;if(k===0)ta-=1440;if(k<0)tb+=1440;return Math.max(0,Math.min(100,Math.round(a.ch[c]+(b.ch[c]-a.ch[c])*(now-ta)/(tb-ta))));}
points=Array.from({length:24},(_,h)=>({t:h*60,ch:Array.from({length:6},(_,c)=>value(h*60,c))}));renderPoints();}
async function upload24(){collectPoints();if(points.length!==24||points.some((p,i)=>p.t!==i*60)){alert('Primero crea y guarda las 24 horas exactas');return;}if(!confirm('Prueba RF sin persistencia demostrada. ¿Transmitir los 24 paquetes Custom?'))return;
try{const r=await fetch('/api/evergrow/upload24',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'confirm=YES'});if(!r.ok)throw Error(await r.text());const j=await r.json();$('scheduleError').textContent='Transmitidos '+j.attempts+' paquetes, ACK RF '+j.acks+'. Guardado en lámpara SIN CONFIRMAR. Desactiva Custom automático para probar.';}catch(e){$('scheduleError').textContent='ERROR: '+e.message;}}
function removePoint(i){collectPoints();if(points.length<=2){alert('Mínimo dos puntos');return}points.splice(i,1);renderPoints()}
document.addEventListener('input',e=>{if(e.target.matches('#scheduleRows input')){const r=Number(e.target.dataset.row);if(!Number.isInteger(r)||!points[r])return;if(e.target.dataset.col!==undefined){const c=Number(e.target.dataset.col);const value=Number(e.target.value);if(Number.isFinite(value))points[r].ch[c]=Math.max(0,Math.min(100,value));}else if(e.target.dataset.time&&e.target.value){const parts=e.target.value.split(':').map(Number);points[r].t=parts[0]*60+parts[1];}updateGraph();}});
$('graphLegend').addEventListener('change',e=>{if(e.target.dataset.series!==undefined){graphVisible[Number(e.target.dataset.series)]=e.target.checked;updateGraph()}});
renderLegend();updateGraph();
async function loadSchedule(){try{const j=await req('/api/evergrow/schedule');points=j.points;renderPoints()}catch(e){$('scheduleError').textContent=e.message}}
async function saveSchedule(){collectPoints();points.sort((a,b)=>a.t-b.t);if(points.some((p,i)=>i&&p.t===points[i-1].t||p.ch.some(v=>!Number.isInteger(v)||v<0||v>100))){$('scheduleError').textContent='Horas repetidas o canales fuera del rango 0-100';return}const q=new URLSearchParams();q.set('data',points.map(p=>[p.t,...p.ch].join(',')).join(';'));try{const r=await fetch('/api/evergrow/schedule',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:q.toString()});if(!r.ok)throw Error(await r.text());$('scheduleError').textContent='Horario guardado en memoria. Actívalo cuando quieras.';await loadSchedule()}catch(e){$('scheduleError').textContent='ERROR: '+e.message}}
async function activateSchedule(on){try{const r=await req('/api/evergrow/activate?on='+(on?1:0)+clock());$('scheduleError').textContent=on?'Programación activada':'Programación pausada';render(await req('/api/evergrow/status'))}catch(e){$('scheduleError').textContent='ERROR: '+e.message}}
async function syncClock(){try{const r=await req('/api/evergrow/clock?epoch='+Math.floor(Date.now()/1000));return r}catch(e){$('scheduleError').textContent='Sin reloj: '+e.message}}

function channel(i,v){v=Math.max(0,Math.min(100,v));send('channel','&ch='+(i+1)+'&v='+v)}
$('channels').addEventListener('input',e=>{if(e.target.matches('input[type=range]'))$('v'+e.target.dataset.ch).textContent=e.target.value+' %'});
$('channels').addEventListener('change',e=>{if(e.target.matches('input[type=range]'))channel(+e.target.dataset.ch,+e.target.value)});
$('channels').addEventListener('click',e=>{const b=e.target.closest('button[data-step]');if(!b)return;const i=+b.dataset.ch;channel(i,+$('ch'+i).value + (+b.dataset.step))});
syncClock().then(()=>refresh());loadSchedule();setInterval(refresh,4000);setInterval(syncClock,300000);
</script></body></html>
)EGHTML";

void setup() {
  Serial.begin(115200); delay(300);
  egLoadConfig();
  setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1);tzset();
  pinMode(SONAR_TRIG, OUTPUT); digitalWrite(SONAR_TRIG, LOW);
  pinMode(SONAR_ECHO, INPUT);
  SPI.begin(); radioReady = radio.begin();
  if (radioReady) {
    applyJebaoRadio();
  }
  WiFi.persistent(false); WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(PORTAL_IP, PORTAL_IP, IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(53, "*", PORTAL_IP);
  WiFi.setAutoReconnect(true);
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.printf("Conectando a SSID %s (hasta 30 segundos)...\n", STA_SSID);
  // Se espera 30 segundos antes de activar el control de la bomba.
  // Si no hay Wi-Fi, el punto de acceso permanece disponible para recuperacion.
  uint32_t wifiWaitStart=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-wifiWaitStart<30000UL){
    delay(250); yield();
  }
  Serial.printf("STA: %s, codigo=%d, IP=%s\n",
    WiFi.status()==WL_CONNECTED?"CONECTADO":"SIN CONEXION",
    (int)WiFi.status(), WiFi.localIP().toString().c_str());
  server.on("/evergrow", HTTP_GET, [](){if(!authorized())return;server.send_P(200,"text/html; charset=utf-8",EG_PAGE);});
  server.on("/api/evergrow/status", HTTP_GET, evergrowStatus);
  server.on("/api/evergrow/mode", HTTP_GET, [](){
    if(!authorized())return;
    if(!server.hasArg("on") || (server.arg("on")!="0" && server.arg("on")!="1")){
      server.send(400,"text/plain","Usa on=0 o on=1");return;
    }
    chooseController(server.arg("on")=="1"); evergrowStatus();
  });
  server.on("/api/evergrow/clock", HTTP_GET, [](){
    if(!authorized())return;
    if(!server.hasArg("epoch")){server.send(400,"text/plain","Falta epoch");return;}
    time_t epoch=(time_t)strtoll(server.arg("epoch").c_str(),nullptr,10);
    if(epoch<1700000000L){server.send(400,"text/plain","Epoch invalido");return;}
    struct timeval tv={epoch,0};settimeofday(&tv,nullptr);
    egClockSynced=true;egLastClockSync=millis();
    server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/api/evergrow/schedule", HTTP_GET, [](){
    if(!authorized())return;
    String j="{\"enabled\":"+String(egCfg.enabled?"true":"false")+",\"points\":[";
    for(uint8_t i=0;i<egCfg.count;i++){
      if(i)j+=',';j+="{\"t\":"+String(egCfg.point[i].minute)+",\"ch\":[";
      for(uint8_t c=0;c<6;c++){if(c)j+=',';j+=String(egCfg.point[i].ch[c]);}
      j+="]}";
    }j+="]}";server.send(200,"application/json",j);
  });
  server.on("/api/evergrow/schedule", HTTP_POST, [](){
    if(!authorized())return;
    String data=server.arg("data");
    if(!data.length()||data.length()>1600){server.send(400,"text/plain","Datos invalidos");return;}
    EgConfig tmp=egCfg;tmp.count=0;
    uint16_t offset=0;
    while(offset<data.length()){
      if(tmp.count>=EG_MAX_POINTS){server.send(400,"text/plain","Maximo 24 puntos");return;}
      int sep=data.indexOf(';',offset);if(sep<0)sep=data.length();
      String item=data.substring(offset,sep);uint8_t idx=0;int start=0;int values[7];
      while(start<=item.length() && idx<7){
        int end=item.indexOf(',',start);if(end<0)end=item.length();
        String part=item.substring(start,end);
        if(!part.length()){server.send(400,"text/plain","Campo vacio");return;}
        for(unsigned int k=0;k<part.length();k++)if(!isDigit(part[k])){server.send(400,"text/plain","Se esperan numeros");return;}
        values[idx++]=part.toInt();start=end+1;
      }
      if(idx!=7 || start<=item.length()){server.send(400,"text/plain","Cada punto necesita hora y 6 canales");return;}
      if(values[0]<0||values[0]>1439||(tmp.count && values[0]<=tmp.point[tmp.count-1].minute)){
        server.send(400,"text/plain","Horas deben estar ordenadas y ser distintas");return;
      }
      EgPoint &p=tmp.point[tmp.count++];p.minute=values[0];
      for(uint8_t c=0;c<6;c++){
        if(values[c+1]<0||values[c+1]>100){server.send(400,"text/plain","Canal fuera de 0-100");return;}
        p.ch[c]=values[c+1];
      }
      offset=sep+1;
    }
    if(tmp.count<2){server.send(400,"text/plain","Minimo dos horas");return;}
    egCfg=tmp;egSaveConfig();egLastScheduleMinute=0xFFFFFFFFUL;
    server.send(200,"application/json","{\"saved\":true}");
  });
  server.on("/api/evergrow/activate", HTTP_GET, [](){
    if(!authorized())return;
    if(!server.hasArg("on")||(server.arg("on")!="0"&&server.arg("on")!="1")){
      server.send(400,"text/plain","Se espera on=0 o on=1");return;
    }
    bool enable=server.arg("on")=="1";
    time_t et;uint16_t mn;
    if(enable&&!egClock(et,mn)){server.send(409,"text/plain","Reloj ESP no sincronizado");return;}
    egCfg.enabled=enable;egSaveConfig();egLastScheduleMinute=0xFFFFFFFFUL;
    if(enable){egProfile=3;egPowered=true;egEvaluate(mn,egChannels);egTransmit("custom_start");
      egLastScheduleMinute=(uint32_t)(et/60);}
    server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/api/evergrow/upload24", HTTP_POST, [](){
    if(!authorized())return;
    if(server.arg("confirm")!="YES"){
      server.send(400,"text/plain","Debes confirmar prueba experimental");return;
    }
    if(!radioReady){server.send(503,"text/plain","NRF24 no detectado");return;}
    if(!egUploadAll24()){
      server.send(409,"text/plain","Se necesitan exactamente 24 puntos: 00:00, 01:00, ... 23:00. Guarda primero el horario.");return;
    }
    String j="{\"transmitted\":true,\"lampSaved\":null,\"attempts\":"+String(egUploadAttempts)+",\"acks\":"+String(egUploadAcks)+"}";
    server.send(200,"application/json",j);
  });
  server.on("/api/evergrow/clock_packet", HTTP_GET, [](){
    if(!authorized())return;
    // Investigacion: B2=0B, B6=hora, B7=minutos; B11/B18/B20 sin decodificar.
    // Por seguridad NO se emite un paquete incompleto para el reloj.
    server.send(200,"application/json",R"json({"known":{"B2":11,"B6":"hour","B7":"minute"},"unknown":[11,18,20],"transmitted":false})json");
  });
  server.on("/api/evergrow/send", HTTP_GET, evergrowSend);
  server.on("/", HTTP_GET, [](){ if(!authorized())return; server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/v2", HTTP_GET, [](){if(!authorized())return; server.send_P(200,"text/html; charset=utf-8",PAGE_V2);});
  server.on("/api/v2/status", HTTP_GET, v2Status);
  // Snapshot of raw distances: last 128 samples, time in millis, height in mm.
  server.on("/api/v2/samples", HTTP_GET, [](){
    if(!authorized())return;
    String j="{\"uptimeMs\":"+String(millis())+",\"count\":"+String(sonarCount)+",\"samples\":[";
    for(int i=0;i<sonarCount;i++){
      uint8_t ix=(sonarPos+128-sonarCount+i)%128;
      if(i)j+=",";
      j+="["+String(sonarHistory[ix].t)+","+String(sonarHistory[ix].mm)+"]";
    }
    j+="]}"; server.send(200,"application/json",j);
  });
  server.on("/api/remote/status", HTTP_GET, [](){
    if(!authorized())return;
    String j="{\"wifi\":"+String(WiFi.status()==WL_CONNECTED?"true":"false")+
      ",\"stationIp\":\""+WiFi.localIP().toString()+"\",\"rssi\":"+String(WiFi.RSSI())+
      ",\"freeHeap\":"+String(ESP.getFreeHeap())+",\"uptimeMs\":"+String(millis())+"}";
    server.send(200,"application/json",j);
  });
  // OTA desde Arduino IDE: el puerto de red se anuncia mediante mDNS.
  ArduinoOTA.setHostname("jebao-wavebox-v2");
  ArduinoOTA.setPassword(WEB_PASS);
  if(WiFi.status()==WL_CONNECTED){
    ArduinoOTA.begin();
    arduinoOtaStarted=true;
    Serial.println("Arduino OTA listo: jebao-wavebox-v2");
  }

  // Browser-based OTA: authenticate independently; requires compiled ESP8266 .bin.
  httpUpdater.setup(&server, "/update", WEB_USER, WEB_PASS);

  server.on("/api/v2/search", HTTP_GET, v2Search);
  server.on("/api/v2/lock", HTTP_GET, [](){if(!authorized())return;if(!waveHasBest){error400("Todavia no hay frecuencia valida");return;}waveAbort(true);v2Status();});
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/rf/mode", HTTP_GET, [](){if(!authorized())return;
    if(!server.hasArg("n")){error400("Falta n");return;}
    long n=server.arg("n").toInt();
    if(!selectRadioMode((uint8_t)n) || n==4 || n<1 || n>5){error400("Modo sin captura verificada");return;}
    sendStatus();
  });
  server.on("/api/manual", HTTP_GET, [](){if(!authorized())return;
    if (!server.hasArg("on") || !server.hasArg("off") || !server.hasArg("step")) { error400("Faltan parametros"); return; }
    long on=server.arg("on").toInt(), off=server.arg("off").toInt(), step=server.arg("step").toInt();
    if (!validTime(on)||!validTime(off)||!validStep(step)) { error400("Manual fuera de rango"); return; }
    if(waveSearch){error400("Deten la busqueda /v2 antes de ajustar Manual");return;}
    manualOnMs=on;manualOffMs=off;manualStepMs=step;
    if (mode==MANUAL) { onMs=manualOnMs; offMs=manualOffMs; setPhase(true,millis()); }
    sendStatus();
  });
  server.on("/api/manual/start", HTTP_GET, [](){if(!authorized())return; manualStart();sendStatus(); });
  server.on("/api/auto/config", HTTP_GET, [](){if(!authorized())return;
    if(!server.hasArg("start")||!server.hasArg("end")||!server.hasArg("step")||!server.hasArg("dwell")){error400("Faltan parametros");return;}
    long start=server.arg("start").toInt(),end=server.arg("end").toInt(),step=server.arg("step").toInt(),dwell=server.arg("dwell").toInt();
    if(!validTime(start)||!validTime(end)||!validStep(step)||dwell<1000||dwell>30000||dwell%500){error400("Auto fuera de rango");return;}
    if(mode==AUTO){error400("Fija frecuencia o detén el barrido antes de cambiar la configuracion");return;}
    autoStartMs=start;autoEndMs=end;autoStepMs=step;dwellMs=dwell;autoTotal=calcTotal();autoCompleted=0;autoIndex=0;
    sendStatus();
  });
  server.on("/api/auto/start", HTTP_GET, [](){if(!authorized())return;if(radioMode != 5){error400("Auto solo disponible para modo 5");return;}autoStart();sendStatus();});
  server.on("/api/auto/lock", HTTP_GET, [](){if(!authorized())return;if(mode!=AUTO){error400("Auto no esta en marcha");return;}lockFrequency();sendStatus();});
  server.on("/api/stop", HTTP_GET, [](){if(!authorized())return;stopPulses();sendStatus();});
  server.on("/generate_204", HTTP_ANY, redirectPortal);
  server.on("/gen_204", HTTP_ANY, redirectPortal);
  server.on("/hotspot-detect.html", HTTP_ANY, redirectPortal);
  server.on("/library/test/success.html", HTTP_ANY, redirectPortal);
  server.on("/connecttest.txt", HTTP_ANY, redirectPortal);
  server.on("/ncsi.txt", HTTP_ANY, redirectPortal);
  server.on("/fwlink", HTTP_ANY, redirectPortal);
  server.onNotFound(redirectPortal);
  server.begin();
  autoTotal = calcTotal();
  stopPulses();
  lastTx = lastReport = millis();
  Serial.printf("JEBAO AP http://192.168.4.1/ ; LAN http://%s/ ; OTA /update\n", WiFi.localIP().toString().c_str());
}

void loop() {
  dnsServer.processNextRequest(); server.handleClient();
  // El portal AP y la actualizacion HTTP permanecen accesibles incluso sin red domestica.
  const bool connected=(WiFi.status()==WL_CONNECTED);
  if(connected && !arduinoOtaStarted){
    ArduinoOTA.begin();
    arduinoOtaStarted=true;
    Serial.printf("Arduino OTA listo: jebao-wavebox-v2, IP=%s\n",WiFi.localIP().toString().c_str());
  }
  if(connected && arduinoOtaStarted) ArduinoOTA.handle();
  if(connected != wifiWasConnected){
    wifiWasConnected=connected;
    Serial.printf("WiFi %s, estado=%d, IP=%s\n", connected?"CONECTADO":"DESCONECTADO", (int)WiFi.status(), WiFi.localIP().toString().c_str());
    if(!connected){
      // El control de la bomba no depende de la red Wi-Fi.
      // Conservar modo, fase y temporizadores en la desconexion.
      Serial.println(F("JEBAO: Wi-Fi perdido; manteniendo modo RF y pulsos"));
    }
  }
  if(!connected && millis()-lastWifiRetry>=WIFI_RETRY_MS){
    lastWifiRetry=millis();
    Serial.printf("Reintentando conexion a %s, estado=%d\n", STA_SSID, (int)WiFi.status());
    WiFi.disconnect(false);
    WiFi.begin(STA_SSID,STA_PASS);
  }
  if(millis()-lastWifiDebug>=5000UL){
    lastWifiDebug=millis();
    Serial.printf("WIFI estado=%d SSID=%s IP=%s RSSI=%d OTA=%s\n",
      (int)WiFi.status(),WiFi.SSID().c_str(),WiFi.localIP().toString().c_str(),WiFi.RSSI(),
      connected && arduinoOtaStarted?"LISTO":"NO DISPONIBLE");
  }
  // Jebao y sensor siempre activos, independientemente de la conectividad Wi-Fi.
  // El usuario puede detener manualmente la bomba con el control de parada.
  uint32_t now = millis();
  sensorTick(now);
  egScheduleTick();
  if(evergrowMode){ yield(); return; } // solo Evergrow transmite mientras se usa
  if(waveSearch && millis()-sonarLastValid>SENSOR_LOSS_MS) waveAbort(true);
  if(mode == AUTO || mode == MANUAL || mode == LOCKED) {
    uint32_t len = phaseOn ? onMs : offMs;
    if(passed(now,phaseStarted) >= len) {
      // Advance using scheduled boundary to reduce jitter and clock drift.
      uint32_t boundary = phaseStarted + len;
      if(mode==AUTO && !phaseOn && passed(boundary,stepStarted)>=(waveSearch? max<uint32_t>(waveDwellMs, uint32_t(10) * currentAutoMs) : dwellMs)) {
        if(waveSearch){rateWaveStep();waveStepsDone=autoIndex+1;}
        autoCompleted=autoIndex+1;
        if(autoCompleted>=autoTotal) {
          // Keep pulsing the final setting so that resonance can still be observed.
          if(waveSearch){waveAbort(true);}else{mode=LOCKED;setPhase(true,boundary);}
        } else {
          beginAutoStep(autoCompleted,boundary);
          if(waveSearch)resetWaveSamples(boundary);
        }
      } else {
        setPhase(!phaseOn,boundary);
      }
    }
  }
  now=millis();
  if(radioReady && passed(now,lastTx)>=TX_INTERVAL_MS) {
    lastTx=now;
    if(radio.write(payload,sizeof(payload))) ++txOk; else ++txNoAck;
  }
  if(passed(now,lastReport)>=10000) {
    lastReport=now;
    Serial.printf("MODO=%u ON=%u OFF=%u AUTO=%u/%u RF ACK=%lu FAIL=%lu\n",
      (unsigned)mode,onMs,offMs,autoCompleted,autoTotal,(unsigned long)txOk,(unsigned long)txNoAck);
  }
  yield();
}