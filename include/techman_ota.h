#pragma once
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#if __has_include("techman_version.h")
#include "techman_version.h"
#endif
#ifndef TECHMAN_FW_VERSION
#define TECHMAN_FW_VERSION "bootstrap-1"
#endif
static const char tmRoots[] PROGMEM = R"ROOT(-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggRvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----)ROOT";
struct TmSavedCredentials { uint32_t magic; char ssid[33];char wifi[65];char web[65];char ap[65];uint32_t crc; };
static TmSavedCredentials tmSaved;
static String tmState="no_comprobado",tmAvailable="";
static int tmHttpCode=0;
static String tmDiagnostic="pendiente";
static bool tmRequested=false,tmInstall=false,tmBusy=false;
static uint32_t tmLast=0;
static uint32_t tmCrc(const TmSavedCredentials &v){const uint8_t*p=(const uint8_t*)&v;uint32_t h=2166136261UL;for(size_t i=0;i<sizeof(v)-4;i++){h^=p[i];h*=16777619UL;}return h;}
static void tmCopy(char*d,size_t n,const char*s){memset(d,0,n);if(s)strncpy(d,s,n-1);}
static void tmCredentials(){
  EEPROM.get(2048,tmSaved);
  bool valid=tmSaved.magic==0x544D5231UL&&tmSaved.crc==tmCrc(tmSaved)
    && memchr(tmSaved.ssid,0,sizeof(tmSaved.ssid))&&memchr(tmSaved.wifi,0,sizeof(tmSaved.wifi))
    && memchr(tmSaved.web,0,sizeof(tmSaved.web))&&memchr(tmSaved.ap,0,sizeof(tmSaved.ap));
  if(!valid&&strlen(TECHMAN_WIFI_SSID)&&strcmp(TECHMAN_WEB_PASS,"CHANGE_ME_BEFORE_USE")!=0){
    memset(&tmSaved,0,sizeof(tmSaved));tmSaved.magic=0x544D5231UL;
    tmCopy(tmSaved.ssid,sizeof(tmSaved.ssid),TECHMAN_WIFI_SSID);
    tmCopy(tmSaved.wifi,sizeof(tmSaved.wifi),TECHMAN_WIFI_PASS);
    tmCopy(tmSaved.web,sizeof(tmSaved.web),TECHMAN_WEB_PASS);
    tmCopy(tmSaved.ap,sizeof(tmSaved.ap),TECHMAN_AP_PASS);
    tmSaved.crc=tmCrc(tmSaved);EEPROM.put(2048,tmSaved);EEPROM.commit();valid=true;
  }
  if(valid){STA_SSID=tmSaved.ssid;STA_PASS=tmSaved.wifi;WEB_PASS=tmSaved.web;AP_PASS=tmSaved.ap;}
}
static bool tmIsVersion(const String&s){if(s.length()!=12)return false;for(unsigned i=0;i<s.length();i++)if(!isxdigit((unsigned char)s[i]))return false;return true;}
static void tmRunCheck(bool install){
  Serial.printf("[TECHMAN OTA] Heap antes TLS=%u maxBlock=%u fragmentation=%u%%\n",ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),ESP.getHeapFragmentation());
  tmRequested=tmInstall=false;
  if(WiFi.status()!=WL_CONNECTED){tmState="sin_wifi";return;}
  if(time(nullptr)<1735689600L){tmState="sin_hora";return;}
  if(ESP.getFreeSketchSpace()<420000){tmState="sin_espacio";return;}
  String base=TECHMAN_OTA_BASE_URL;
  if(!base.startsWith("https://")){tmState="sin_url";return;}
  // Mantener la conexion TLS del manifiesto en su propio ambito.
  // El cliente HTTPS y su memoria DEBEN liberarse ANTES de iniciar otra conexion.
  String ver;
  {
    BearSSL::X509List roots(tmRoots);
    BearSSL::WiFiClientSecure c;
    c.setTrustAnchors(&roots);
    c.setBufferSizes(1024,1024);
    c.setTimeout(12000);
    HTTPClient http;
    http.setTimeout(12000);
    if(!http.begin(c,base+"/manifest.txt")) {
      tmState="conexion_manifest";
      tmDiagnostic="http.begin fallo";
      Serial.println("[TECHMAN OTA] "+tmDiagnostic);
      return;
    }
    const int status=http.GET();
    tmHttpCode=status;
    if(status==200)ver=http.getString();
    const String error=status<0?http.errorToString(status):"";
    char tlsDetail[150]={0};
    const int sslCode=c.getLastSSLError(tlsDetail,sizeof(tlsDetail));
    http.end();
    tmDiagnostic="HTTP="+String(status)+" ssl="+String(sslCode)+" "+String(tlsDetail)+" httpErr="+error+" length="+String(ver.length());
    Serial.println("[TECHMAN OTA] "+tmDiagnostic);
  }
  // Aqui HTTPClient, BearSSL::WiFiClientSecure y certificado ya han sido destruidos.
  ver.trim();
  Serial.printf("[TECHMAN OTA] Heap despues manifest=%u maxBlock=%u frag=%u%%\n",ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),ESP.getHeapFragmentation());
  if(!tmIsVersion(ver)){tmState="manifest_invalido_http_"+String(tmHttpCode);return;}
  if(ver==String(TECHMAN_FW_VERSION)){tmAvailable="";tmState="actualizado";return;}
  tmAvailable=ver;
  if(!install){tmState="disponible";return;}
  // Evitar OOM conocido antes de crear el segundo cliente TLS.
  if(ESP.getMaxFreeBlockSize()<16000){
    tmState="ram_insuficiente_ota";
    Serial.println("[TECHMAN OTA] Instalacion cancelada: RAM contigua insuficiente");
    return;
  }
  tmState="instalando";
  tmBusy=true;
  Serial.printf("[TECHMAN OTA] Iniciando firmware: heap=%u maxBlock=%u\n",ESP.getFreeHeap(),ESP.getMaxFreeBlockSize());
  BearSSL::X509List fwRoots(tmRoots);
  BearSSL::WiFiClientSecure fw;
  fw.setTrustAnchors(&fwRoots);
  fw.setBufferSizes(1024,1024);
  fw.setTimeout(20000);
  ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return rc=ESPhttpUpdate.update(fw,base+"/firmware.bin");
  tmBusy=false;
  if(rc==HTTP_UPDATE_FAILED)tmState="error_flash_"+String(ESPhttpUpdate.getLastError());
  else if(rc==HTTP_UPDATE_NO_UPDATES)tmState="sin_cambios";
}
static void tmTick(){
  if(tmBusy||WiFi.status()!=WL_CONNECTED||mode==AUTO||waveSearch)return;
  if(!tmRequested&&!tmInstall)return;
  bool install=tmInstall;tmLast=millis();tmRunCheck(install);
}
static void tmRoutes(){
  server.on("/api/ota/status",HTTP_GET,[](){
    if(!authorized())return;
    server.send(200,"application/json",String("{\"version\":\"")+TECHMAN_FW_VERSION+"\",\"available\":\""+tmAvailable+"\",\"state\":\""+tmState+"\",\"httpCode\":"+String(tmHttpCode)+",\"diagnostic\":\""+tmDiagnostic+"\"}");
  });
  server.on("/api/ota/check",HTTP_POST,[](){if(!authorized())return;tmRequested=true;server.send(202,"application/json","{\"queued\":true}");});
  server.on("/api/ota/install",HTTP_POST,[](){if(!authorized())return;if(mode==AUTO||waveSearch){server.send(409,"text/plain","Deten el barrido primero");return;}tmInstall=true;server.send(202,"application/json","{\"queued\":true}");});
}
