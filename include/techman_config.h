#pragma once
// Los valores por defecto permiten compilar en GitHub sin almacenar contraseñas.
// Crea include/techman_config_local.h en tu ordenador para grabar por USB.
#if __has_include("techman_config_local.h")
#include "techman_config_local.h"
#endif
#ifndef TECHMAN_WIFI_SSID
#define TECHMAN_WIFI_SSID ""
#endif
#ifndef TECHMAN_WIFI_PASS
#define TECHMAN_WIFI_PASS ""
#endif
#ifndef TECHMAN_WEB_PASS
#define TECHMAN_WEB_PASS "CHANGE_ME_BEFORE_USE"
#endif
#ifndef TECHMAN_AP_PASS
#define TECHMAN_AP_PASS "CHANGE_ME_BEFORE_USE"
#endif
