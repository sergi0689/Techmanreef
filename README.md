# Techmanreef — ESP8266

**ESTADO: migración incompleta. No flashear los binarios generados actualmente.**

Repositorio adaptado al entorno NodeMCU ESP8266 (`nodemcuv2`). Incluye `platformio.ini`, cabecera de configuración sin secretos y workflow `build-esp8266.yml`.

**Pendiente:** el archivo `src/main.cpp` en GitHub sigue siendo el programa de prueba antiguo: la operación de subida del código real fue bloqueada. Para añadir el código real, utilizar el ZIP preparado de Techmanreef_ESP8266, sin publicar credenciales.

El firmware original permite carga OTA *local* desde `/update`; no implementa descarga OTA desatendida desde GitHub. El repositorio es privado y los artefactos de GitHub Actions no son URLs de firmware descargables anónimamente por el ESP8266.

No subir `include/techman_config_local.h`. No exponer HTTP ni el puerto de actualización a Internet.
