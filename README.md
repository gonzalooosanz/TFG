# MQTT-API

## Introducción
El software en este repositorio contiene los siguientes componentes
- Proxy UART-TCP, encargado de retransmitir información bidireccionalmente entre el puerto serie y un socket TCP. 
- API MQTT, encargado de encapsular los detalles de la librería paho y proporcionar una API sencilla y eficiente.
- Client, es una aplicación sencilla de prueba que ilustra como sería el uso de la API.

Mas información sobre la idea del proxy:
https://www.digi.com/resources/documentation/Digidocs/90001541/tasks/t_use_mqtt.htm

## Instalación y uso:
A continuación viene un esquema de los pasos necesarios para hacer funcionar el sistema.

### Pasos a seguir en Raspberry PI (Instalación):
1. Instalación de Eclipse paho para poder compilar y ejecutar tanto la API como la aplicación cliente.
2. Configurar acceso a UART.
3. (Opcional) Instalación de git para poder clonar el repositorio.
4. Clonar repositorio ([MQTT_API]).
5. Compilar proxy ('client/proxy.c'), API - MQTT ('proxy/client/MQTT_API.c') y aplicación cliente ('client/client.c').

### Pasos a seguir en PC (Instalación):
1. (Opcional) Instalar XCTU (en caso de que haya que configurar módulos XBee).
2. Instalación del broker MQTT (mosquitto)
3. (Opcional) Instalación de git para poder clonar el repositorio.
4. Clonar repositorio ([MQTT_API]).
5. Compilar proxy ('server/proxy.c').

### Puesta en marcha
1 En PC:
  - Conectar Hardware XBee (FTDI).
  - Ejecutar proxy ('server/proxy').
2. En Raspberry PI:
  - Conectar Hardware XBee (UART).
  - Ejecutar proxy ('client/proxy').
  - Ejecutar cliente ('client/client.c').


