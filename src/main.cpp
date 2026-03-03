/*
 * =============================================================================
 * SISTEMA DE CONTROLE DE MOTOR UNIVERSAL (FRESADORA 1800W) - STM32F103
 * =============================================================================
 *
 * ESTRUTURA DO PROJETO:
 *
 *  Projeto_Fresadora/
 *  ├── platformio.ini
 *  ├── include/config.h
 *  ├── src/main.cpp
 *  └── lib/
 *      ├── hal/
 *      │   ├── pwm.h / .cpp        TIM1 CH1 (PA8): PWM 16kHz
 *      │   ├── rpm.h / .cpp        PA0: medição de RPM
 *      │   ├── corrente.h / .cpp   ADC PA1: ACS712-30A
 *      │   ├── controle.h / .cpp   TIM3 @ 2kHz: loop de controle
 *      │   └── monitor.h / .cpp    PA2/PA3: tensão e potência
 *      ├── control/
 *      │   ├── pids.h / .cpp       PIDs em cascata
 *      │   └── autotune.h / .cpp   Sintonia automática sTune
 *      └── app/
 *          ├── maquina.h / .cpp    Soft-start, parada de emergência
 *          ├── encoder.h / .cpp    KY-040: rotação + botão via polling
 *          ├── grafico.h / .cpp    Buffer circular RPM vs Setpoint (~10s)
 *          └── display.h / .cpp    SSD1306: 5 telas + menu de configuração
 *
 * FLUXO DE EXECUÇÃO:
 *
 *   TIM3 ISR @ 2kHz — prioridade 0 — CONTROLE
 *     lerCorrente() → calcularRPM() → processarCascata() → segurança
 *
 *   Hall ISR (PA0) — prioridade 1
 *     período entre pulsos → RPM
 *
 *   loop() — tempo livre (~95% CPU) — MONITORAMENTO + UI
 *     executarSoftStart()
 *     atualizarMonitor()     → tensão + potência
 *     atualizarGrafico()     → buffer circular 10s
 *     atualizarDisplay()     → encoder + redesenho 10Hz
 * =============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "pwm.h"
#include "rpm.h"
#include "corrente.h"
#include "controle.h"
#include "monitor.h"
#include "pids.h"
#include "autotune.h"
#include "maquina.h"
#include "encoder.h"
#include "grafico.h"
#include "display.h"

static uint32_t ultimoGrafico = 0;

void setup() {
  Serial.begin(115200);

  setupDisplay();        // SSD1306 — tela de boot imediata
  setupEncoder();        // KY-040
  setupHardwarePWM();    // TIM1 @ 16kHz
  setupRPM();            // PA0 interrupção Hall
  setupPIDs();           // Ganhos e sample times
  setupGrafico();        // Buffer circular
  setupTimerControle();  // TIM3 @ 2kHz — inicia o loop de controle

  Serial.println(F("\n>>> Pressione 'A' em ate 5s para iniciar o AUTOTUNE..."));
  unsigned long t = millis();
  while (millis() - t < 5000) {
    atualizarDisplay();
    if (Serial.available() > 0 && (Serial.read() | 0x20) == 'a') {
      executarAutotuneCorrente();
      break;
    }
  }

  setpointRPM = 0.0f;
  Serial.println(F(">>> CONTROLE FRESADORA STM32 ATIVO <<<"));
}

void loop() {
  executarSoftStart();
  atualizarMonitor();

  if (millis() - ultimoGrafico >= (uint32_t)GRAFICO_INTERVALO_MS) {
    atualizarGrafico();
    ultimoGrafico = millis();
  }

  atualizarDisplay();
}