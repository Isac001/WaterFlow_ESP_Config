# 🚨 Sistema de Detecção de Vazamento com NodeMCU 1.0 (ESP8266) e Sensor de Água

Projeto para monitoramento contínuo de vazamentos usando **NodeMCU 1.0 (ESP8266)** com regulador de tensão e sensor de fluxo de água (*waterflow*).

---

## 📦 Componentes Necessários

| Componente                 | Descrição                                                              |
|---------------------------|------------------------------------------------------------------------|
| **NodeMCU 1.0 (ESP8266)** | Placa de desenvolvimento com Wi-Fi integrado                          |
| **Sensor de Água/Waterflow** | Modelo compatível com saída digital (ex: YF-S201)                     |
| **Regulador de Tensão**    | LM7805 ou similar (conversão para 3.3V seguros)                        |
| **Resistor 10kΩ**          | Pull-down para o sensor                                                |
| **Fonte de Alimentação**   | 5V (USB ou bateria externa)                                           |
| **Jumpers e Protoboard**   | Conexões seguras                                                      |

---

## ⚡ Esquema de Conexões

| NodeMCU 1.0 (ESP8266) | Sensor Waterflow       | Regulador de Tensão        |
|-----------------------|-----------------------|----------------------------|
| 3V3 (3.3V)            | VCC (Positivo)        |                            |
| GND (Negativo)        | GND (Negativo)        | GND (Entrada)              |
| D1 (GPIO5)            | OUT (Sinal)           |                            |
|                       | VCC (Entrada)         | 5V (Fonte Externa)         |
|                       |                       | LM7805: Vin(5V) → Vout(3.3V) |

> ⚠️ **Observação:**  
> - O regulador LM7805 é usado para converter a fonte externa de 5V para 3.3V seguros para o sensor.


## 🔧 Configuração do Ambiente

1. **Instale as Bibliotecas (Arduino IDE)**  
Adicione estas bibliotecas via **Sketch > Include Library > Manage Libraries**:  
   - `ESP8266WiFi` — Para conexão Wi-Fi (`#include <ESP8266WiFi.h>`)  
   - `ArduinoJson` — Para manipulação de dados (opcional)  
   - `PubSubClient` — Se for usar MQTT (opcional)

2. **Configure a NodeMCU 1.0 na IDE**  
   - Em **Tools > Board**, selecione: `NodeMCU 1.0 (ESP-12E Module)`  
   - Ajuste a velocidade para **115200 baud** no Serial Monitor.

