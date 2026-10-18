# Techman Reef

Proyecto privado de desarrollo del controlador de acuarios Techman Reef (ESP32 + radio nRF24L01).

## Estado actual

**Solo infraestructura inicial de compilación**. Todavía NO contiene el control real de Evergrow ni de Jebao, la interfaz de usuario ni el sistema de actualizaciones OTA.

**NO grabar el firmware generado en un controlador en funcionamiento.** La compilación actual es únicamente un programa de prueba para verificar GitHub Actions; no transmite señales de radio ni controla bombas o luces.

## Compilación automática

El archivo `.github/workflows/build-esp32.yml` ejecuta PlatformIO en GitHub Actions cuando hay cambios en `main` o cuando se inicia manualmente.

El resultado se guarda como artefacto descargable `Techmanreef-ESP32-BASE-NO-FLASH`. **No se publica a clientes ni se envía a dispositivos.**

## Configuración de prueba

- Entorno: PlatformIO + framework Arduino
- Placa temporal: `esp32dev` (ESP32 clásico genérico)
- Configuración: `platformio.ini`
- Firmware mínimo: `src/main.cpp`

La placa del producto comercial aún se debe concretar; si se elige ESP32-S3, habrá que cambiar el entorno antes de migrar el firmware.

## Próximas etapas

1. Seleccionar placa definitiva y documentar el cableado nRF24L01.
2. Incorporar el firmware existente y migrar de ESP8266 a ESP32 sin modificar el comportamiento comprobado de Jebao.
3. Añadir interfaz Reef responsive y programación Evergrow.
4. Implementar OTA segura con verificación, doble partición y recuperación.
5. Preparar proceso de pruebas y lanzamiento de actualizaciones por fases.

No almacenar contraseñas Wi-Fi, claves API ni credenciales en el repositorio.
