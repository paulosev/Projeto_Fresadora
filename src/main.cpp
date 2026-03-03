/*
 * =============================================================================
 * SISTEMA DE CONTROLE DE MOTOR UNIVERSAL (FRESADORA 1800W) - STM32F103
 * =============================================================================
 *
 * ESTRUTURA DO PROJETO:
 *
 *  Projeto_Fresadora/
 *  │
 *  ├── platformio.ini          ← Board, framework, bibliotecas externas
 *  │
 *  ├── include/
 *  │   └── config.h            ← Todas as constantes, pinos e limites
 *  │
 *  ├── src/
 *  │   └── main.cpp            ← Entry point: setup() e loop()
 *  │
 *  └── lib/                    ← Bibliotecas locais
 *      │
 *      ├── hal/
 *      │   ├── pwm.h / .cpp        ← TIM1 CH1 (PA8): PWM 16kHz
 *      │   ├── rpm.h / .cpp        ← PA0: medição de RPM por período
 *      │   ├── corrente.h / .cpp   ← ADC PA1: leitura ACS712-30A
 *      │   ├── controle.h / .cpp   ← TIM3 @ 2kHz: loop de controle
 *      │   └── monitor.h / .cpp    ← PA2/PA3: tensão e potência (monitoramento)
 *      │
 *      ├── control/
 *      │   ├── pids.h / .cpp       ← PIDs em cascata (velocidade + corrente)
 *      │   └── autotune.h / .cpp   ← Sintonia automática via sTune
 *      │
 *      └── app/
 *          └── maquina.h / .cpp    ← Soft-start, parada de emergência
 *
 * FLUXO DE EXECUÇÃO:
 *
 *   TIM3 ISR @ 2kHz — prioridade 0 (máxima) — CONTROLE
 *     lerCorrente() → calcularRPM() → processarCascata() → segurança
 *
 *   Hall ISR (PA0) — prioridade 1
 *     Período entre pulsos → base do cálculo de RPM
 *
 *   loop() — tempo livre (~95% CPU) — MONITORAMENTO
 *     executarSoftStart() → atualizarMonitor() → display/serial/botões
 * =============================================================================
 */

#include <Arduino.h>
#include "lib/hal/pwm.h"
#include "lib/hal/rpm.h"
#include "lib/hal/corrente.h"
#include "lib/control/controle.h"
#include "lib/hal/monitor.h"
#include "lib/control/pids.h"
#include "lib/control/autotune.h"
#include "lib/app/maquina.h"

void setup() {
  Serial.begin(115200);

  setupHardwarePWM();    // TIM1 @ 16kHz
  setupRPM();            // PA0 interrupção Hall
  setupPIDs();           // Ganhos e sample times
  setupTimerControle();  // TIM3 @ 2kHz — inicia o loop de controle

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
  // --- CONTROLE ---
  // Roda inteiramente na ISR do TIM3. Nada aqui interfere no controle.

  // Rampa gradual de velocidade até SOFTSTART_ALVO
  executarSoftStart();

  // --- MONITORAMENTO (baixa prioridade) ---
  // Lê tensão da fonte, tensão do motor e calcula potência.
  // Executa no tempo livre do loop() — nunca bloqueia o controle.
  atualizarMonitor();

  // --- TAREFAS SECUNDÁRIAS (adicione aqui) ---
  // atualizarDisplay();
  // lerBotoes();
  // processarSerial();
}