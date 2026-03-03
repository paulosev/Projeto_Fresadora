/*
 * =============================================================================
 * SISTEMA DE CONTROLE DE MOTOR UNIVERSAL (FRESADORA 1800W) - STM32F103
 * =============================================================================
 *
 * ESTRUTURA DO PROJETO:
 *
 *  Projeto_Fresadora_PIO/
 *  │
 *  ├── platformio.ini                ← Board, framework, bibliotecas externas
 *  │
 *  ├── include/
 *  │   └── config.h                  ← Todas as constantes, pinos e limites
 *  │
 *  ├── src/
 *  │   └── main.cpp                  ← Entry point: setup() e loop()
 *  │
 *  └── lib/                          ← Bibliotecas locais do projeto
 *      │
 *      ├── hal/                      ← Hardware Abstraction Layer
 *      │   │                            Tudo que toca registrador fica aqui.
 *      │   │                            Trocar de STM32F1 para F4? Só esta pasta muda.
 *      │   │
 *      │   ├── pwm.h / pwm.cpp       ← TIM1 CH1 (PA8): PWM 16kHz
 *      │   ├── rpm.h / rpm.cpp       ← TIM2 CH1 (PA0): Input Capture + ISR
 *      │   ├── corrente.h / .cpp     ← ADC PA1: ACS712-30A
 *      │   └── controle.h / .cpp     ← TIM3 @ 2kHz: loop de controle + ISR
 *      │
 *      ├── control/                  ← Lógica de controle pura
 *      │   │                            Não conhece pinos nem registradores.
 *      │   │
 *      │   ├── pids.h / pids.cpp     ← Variáveis globais, PIDs em cascata
 *      │   └── autotune.h / .cpp     ← Step Test sTune, aplicação de ganhos
 *      │
 *      └── app/                      ← Lógica de aplicação (alto nível)
 *          │                            Orquestra hal/ e control/ pelo nome.
 *          │                            Adicione display, botões, Serial aqui.
 *          │
 *          └── maquina.h / .cpp      ← executarSoftStart(), paradaEmergencia()
 *
 * =============================================================================
 *
 * FEATURES:
 *  1. Controle em cascata (Dual-Loop): RPM → Corrente → PWM
 *  2. PWM 16kHz via TIM1 CH1 (PA8), resolução de 4500 degraus
 *  3. Medição de RPM por Input Capture (TIM2 CH1, PA0), precisão de 1µs
 *  4. Loop de controle determinístico via TIM3 @ 2kHz (prioridade máxima)
 *     O loop() executa apenas tarefas secundárias no tempo que sobra (~95% livre)
 *  5. Soft-Start, Anti-Stall, Overcurrent
 *  6. Autotune da malha de corrente via sTune (Ziegler-Nichols)
 *
 * HARDWARE (STM32F103C8T6 — Blue Pill):
 *  PA8 → TIM1 CH1    PWM de potência (IR2110S + IRFP460)
 *  PA1 → ADC1 CH1    Sensor de corrente ACS712-30A
 *  PA0 → TIM2 CH1    Sensor Hall (Input Capture)
 *  Vref ADC: 3.3V
 *
 * PRIORIDADES NVIC:
 *  TIM3 (controle 2kHz): 0  ← máxima
 *  TIM2 (Input Capture): 1
 *  TIM1 (PWM):           sem ISR
 *
 * =============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "lib/hal/pwm.h"
#include "lib/hal/rpm.h"
#include "lib/hal/corrente.h"
#include "lib/control/controle.h"
#include "lib/control/pids.h"
#include "lib/control/autotune.h"
#include "lib/app/maquina.h"

void setup() {
  Serial.begin(115200);

  setupHardwarePWM();    // TIM1 @ 16kHz
  setupRPM();            // TIM2 Input Capture
  setupPIDs();           // Ganhos e sample times
  setupTimerControle();  // TIM3 @ 2kHz — inicia o loop de controle

  // Autotune opcional — TIM3 já rodando, setpointRPM = 0, motor parado
  Serial.println(F("\n>>> Pressione 'A' em ate 5s para iniciar o AUTOTUNE..."));
  unsigned long t = millis();
  while (millis() - t < 5000) {
    if (Serial.available() > 0 && (Serial.read() | 0x20) == 'a') {
      executarAutotuneCorrente();
      break;
    }
  }

  setpointRPM = 0.0f;
  Serial.println(F(">>> CONTROLE FRESADORA STM32 ATIVO <<<"));
}

void loop() {
  // O controle do motor roda inteiramente na ISR do TIM3 (lib/hal/controle.cpp).
  // O loop() executa apenas tarefas de baixa prioridade no tempo que sobra.

  executarSoftStart();   // Rampa gradual até SOFTSTART_ALVO (lib/app/maquina.cpp)

  // --- Adicione aqui tarefas secundárias ---
  // atualizarDisplay();
  // lerBotoes();
  // processarSerial();
}
