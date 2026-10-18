#pragma once
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Updater.h>
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
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
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
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICjzCCAhWgAwIBAgIQXIuZxVqUxdJxVt7NiYDMJjAKBggqhkjOPQQDAzCBiDEL
MAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNl
eSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMT
JVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAwMjAx
MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
Ck5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUg
VVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlm
aWNhdGlvbiBBdXRob3JpdHkwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQarFRaqflo
I+d61SRvU8Za2EurxtW20eZzca7dnNYMYf3boIkDuAUU7FfO7l0/4iGzzvfUinng
o4N+LZfQYcTxmdwlkWOrfzCjtHDix6EznPO/LlxTsV+zfTJ/ijTjeXmjQjBAMB0G
A1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAOBgNVHQ8BAf8EBAMCAQYwDwYD
VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNoADBlAjA2Z6EWCNzklwBBHU6+4WMB
zzuqQhFkoJ2UOQIReVx7Hfpkue4WQrO/isIJxOzksU0CMQDpKmFHjFJKS04YcPbW
RNZu9YO6bVi9JNlWSOrvxKJGgYhqOkbRqZtNyWHa0V1Xahg=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFijCCA3KgAwIBAgIQdY39i658BwD6qSWn4cetFDANBgkqhkiG9w0BAQwFADBf
MQswCQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQD
Ey1TZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYw
HhcNMjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEY
MBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1Ymxp
YyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCTvtU2UnXYASOgHEdCSe5jtrch/cSV1UgrJnwUUxDa
ef0rty2k1Cz66jLdScK5vQ9IPXtamFSvnl0xdE8H/FAh3aTPaE8bEmNtJZlMKpnz
SDBh+oF8HqcIStw+KxwfGExxqjWMrfhu6DtK2eWUAtaJhBOqbchPM8xQljeSM9xf
iOefVNlI8JhD1mb9nxc4Q8UBUQvX4yMPFF1bFOdLvt30yNoDN9HWOaEhUTCDsG3X
ME6WW5HwcCSrv0WBZEMNvSE6Lzzpng3LILVCJ8zab5vuZDCQOc2TZYEhMbUjUDM3
IuM47fgxMMxF/mL50V0yeUKH32rMVhlATc6qu/m1dkmU8Sf4kaWD5QazYw6A3OAS
VYCmO2a0OYctyPDQ0RTp5A1NDvZdV3LFOxxHVp3i1fuBYYzMTYCQNFu31xR13NgE
SJ/AwSiItOkcyqex8Va3e0lMWeUgFaiEAin6OJRpmkkGj80feRQXEgyDet4fsZfu
+Zd4KKTIRJLpfSYFplhym3kT2BFfrsU4YjRosoYwjviQYZ4ybPUHNs2iTG7sijbt
8uaZFURww3y8nDnAtOFr94MlI1fZEoDlSfB1D++N6xybVCi0ITz8fAr/73trdf+L
HaAZBav6+CuBQug4urv7qv094PPK306Xlynt8xhW6aWWrL3DkJiy4Pmi1KZHQ3xt
zwIDAQABo0IwQDAdBgNVHQ4EFgQUVnNYZJX5khqwEioEYnmhQBWIIUkwDgYDVR0P
AQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAC9c
mTz8Bl6MlC5w6tIyMY208FHVvArzZJ8HXtXBc2hkeqK5Duj5XYUtqDdFqij0lgVQ
YKlJfp/imTYpE0RHap1VIDzYm/EDMrraQKFz6oOht0SmDpkBm+S8f74TlH7Kph52
gDY9hAaLMyZlbcp+nv4fjFg4exqDsQ+8FxG75gbMY/qB8oFM2gsQa6H61SilzwZA
Fv97fRheORKkU55+MkIQpiGRqRxOF3yEvJ+M0ejf5lG5Nkc/kLnHvALcWxxPDkjB
JYOcCj+esQMzEhonrPcibCTRAUH4WAP+JWgiH5paPHxsnnVI84HxZmduTILA7rpX
DhjvLpr3Etiga+kFpaHpaPi8TD8SHkXoUsCjvxInebnMMTzD9joiFgOgyY9mpFui
TdaBJQbpdqQACj7LzTWb4OE4y2BThihCQRxEV+ioratF4yUQvNs+ZUH7G6aXD+u5
dHn5HrwdVw1Hr8Mvn4dGp+smWg9WY7ViYG4A++MnESLn/pmPNPW56MORcr3Ywx65
LvKRRFHQV80MNNVIIb/bE/FmJUNS0nAiNs2fxBx1IK1jcmMGDw4nztJqDby1ORrp
0XZ60Vzk50lJLVU3aPAaOpg+VBeHVOmmJ1CJeyAvP/+/oYtKR5j/K3tJPsMpRmAY
QqszKbrAKbkTidOIijlBO8n9pu0f9GBj39ItVQGL
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICOjCCAcGgAwIBAgIQQvLM2htpN0RfFf51KBC49DAKBggqhkjOPQQDAzBfMQsw
CQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1T
ZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwHhcN
MjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEYMBYG
A1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1YmxpYyBT
ZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwdjAQBgcqhkjOPQIBBgUrgQQA
IgNiAAR2+pmpbiDt+dd34wc7qNs9Xzjoq1WmVk/WSOrsfy2qw7LFeeyZYX8QeccC
WvkEN/U0NSt3zn8gj1KjAIns1aeibVvjS5KToID1AZTc8GgHHs3u/iVStSBDHBv+
6xnOQ6OjQjBAMB0GA1UdDgQWBBTRItpMWfFLXyY4qp3W7usNw/upYTAOBgNVHQ8B
Af8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNnADBkAjAn7qRa
qCG76UeXlImldCBteU/IvZNeWBj7LRoAasm4PdCkT0RHlAFWovgzJQxC36oCMB3q
4S6ILuH5px0CMk7yn2xVdOOurvulGu7t0vzCAxHrRVxgED1cf5kDW21USAGKcw==
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
  if(install && (!tmIsVersion(tmAvailable) || tmAvailable==String(TECHMAN_FW_VERSION))) {
    tmState="buscar_primero";Serial.println("[TECHMAN OTA] Pulsa Buscar antes de instalar");return;
  }
  // Al instalar NO volver a consultar el manifiesto, que fragmenta el heap.
  if(!install){
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
  if(tmHttpCode!=200){tmState="conexion_manifest_http_"+String(tmHttpCode);return;}
  if(!tmIsVersion(ver)){tmState="contenido_manifest_invalido";return;}
  if(ver==String(TECHMAN_FW_VERSION)){tmAvailable="";tmState="actualizado";return;}
  tmAvailable=ver;
  tmState="disponible";return;
  } // final de consulta de manifiesto
  // Evitar OOM conocido antes de crear el segundo cliente TLS.
  if(ESP.getMaxFreeBlockSize()<14000){
    tmState="ram_insuficiente_ota";
    Serial.println("[TECHMAN OTA] Instalacion cancelada: RAM contigua insuficiente");
    return;
  }
  Serial.printf("[TECHMAN OTA] Descarga directa heap=%u max=%u\n", ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  tmState="instalando";
  tmBusy=true;
  Serial.printf("[TECHMAN OTA] Iniciando firmware: heap=%u maxBlock=%u\n",ESP.getFreeHeap(),ESP.getMaxFreeBlockSize());
  BearSSL::X509List fwRoots(tmRoots);
  BearSSL::WiFiClientSecure fw;
  fw.setTrustAnchors(&fwRoots);
  fw.setBufferSizes(1024,1024);
  fw.setTimeout(20000);
  // Descargar por HTTPClient con lectura de cabecera explicitamente comprobada.
  // ESPhttpUpdate utiliza peekBytes(); con algunos servidores HTTPS este método
  // devuelve -106 al no disponer todavía de datos en el buffer BearSSL.
  HTTPClient fwHttp;
  fwHttp.useHTTP10(true);
  fwHttp.setTimeout(30000);
  fwHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int fwStatus=-1;
  if(fwHttp.begin(fw,base+"/firmware.bin")) fwStatus=fwHttp.GET();
  Serial.printf("[TECHMAN OTA] Firmware HTTP=%d size=%d heap=%u bloque=%u\\n",
       fwStatus,fwHttp.getSize(),ESP.getFreeHeap(),ESP.getMaxFreeBlockSize());
  if(fwStatus!=200 || fwHttp.getSize()<100000){
    tmState="firmware_http_"+String(fwStatus);
    fwHttp.end();tmBusy=false;return;
  }
  const size_t length=(size_t)fwHttp.getSize();
  WiFiClient *stream=fwHttp.getStreamPtr();
  if(stream==nullptr){
    tmState="stream_nulo";Serial.println("[TECHMAN OTA] ERROR stream nulo");
    fwHttp.end();tmBusy=false;return;
  }
  if(ESP.getMaxFreeBlockSize()<8000){
    tmState="ram_insuficiente_descarga";
    Serial.printf("[TECHMAN OTA] Descarga abortada antes de flash: heap=%u max=%u\\n",ESP.getFreeHeap(),ESP.getMaxFreeBlockSize());
    fwHttp.end();tmBusy=false;return;
  }
  Serial.println("[TECHMAN OTA] Leyendo cabecera firmware...");
  uint8_t magic[4]={0};
  size_t got=stream->readBytes((char*)magic,4);
  Serial.printf("[TECHMAN OTA] Cabecera: recibidos=%u hex=%02X %02X %02X %02X\\n",
       (unsigned)got,magic[0],magic[1],magic[2],magic[3]);
  if(got!=4 || magic[0]!=0xE9){
    tmState="cabecera_no_valida";fwHttp.end();tmBusy=false;return;
  }
  if(!Update.begin(length,U_FLASH)){
    tmState="flash_begin_"+String(Update.getError());fwHttp.end();tmBusy=false;return;
  }
  if(Update.write(magic,4)!=4){
    tmState="flash_cabecera_"+String(Update.getError());Update.end(false);fwHttp.end();tmBusy=false;return;
  }
  size_t total=4;
  uint8_t buf[512];
  uint32_t lastData=millis();
  int lastProgress=-1;
  while(total<length){
    const size_t available=stream->available();
    if(available){
      size_t batch=min((size_t)sizeof(buf),min(available,length-total));
      int n=stream->read(buf,batch);
      if(n>0){
        if(Update.write(buf,(size_t)n)!=(size_t)n)break;
        total+=(size_t)n;lastData=millis();
        int pct=(int)(total*100UL/length);
        if(pct/10!=lastProgress){lastProgress=pct/10;Serial.printf("[TECHMAN OTA] %d%% (%u/%u)\\n",pct,(unsigned)total,(unsigned)length);}
      }
    } else if(millis()-lastData>30000UL || !stream->connected()){
      break;
    }
    delay(1);
  }
  Serial.printf("[TECHMAN OTA] Transferencia %u/%u bytes error=%u\\n",(unsigned)total,(unsigned)length,Update.getError());
  if(total==length && Update.end(true)){
    fwHttp.end();
    tmState="instalado_reiniciando";
    Serial.println("[TECHMAN OTA] Firmware validado, reiniciando");
    Serial.flush();ESP.restart();return;
  }
  tmState="transferencia_incompleta_"+String(total)+"_de_"+String(length)+"_err_"+String(Update.getError());
  Update.end(false);fwHttp.end();
  tmBusy=false;

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
