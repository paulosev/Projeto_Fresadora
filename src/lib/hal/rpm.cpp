#include <Arduino.h>
#include "rpm.h"
#include "config.h"

// =============================================================================
// HAL - RPM: PA0 — Interrupção de borda via attachInterrupt()
// =============================================================================
// O STM32duino já define TIM2_IRQHandler em HardwareTimer.cpp.
// Declarar extern "C" void TIM2_IRQHandler() causaria "multiple definition".
// Solução: usar attachInterrupt() no pino PA0 — o framework roteia
// internamente para o canal correto do TIM2 sem conflito.
//
// micros() no STM32F103 @ 72MHz tem resolução de ~1µs — equivalente ao
// Input Capture manual para a faixa de RPM deste projeto (1k–10k RPM).
// =============================================================================

static volatile uint32_t ultimoMicros     = 0;
static volatile uint32_t periodoPulsos_us = 0;
static volatile uint32_t ultimaCaptura_ms = 0;

static void hallISR() {
  uint32_t agora = micros();
  uint32_t delta = agora - ultimoMicros;

  if (delta > RPM_FILTRO_RUIDO) {
    periodoPulsos_us = delta;
    ultimaCaptura_ms = millis();
  }
  ultimoMicros = agora;
}

void setupRPM() {
  pinMode(PIN_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), hallISR, FALLING);
}

float calcularRPM() {
  noInterrupts();
  uint32_t p = periodoPulsos_us;
  uint32_t t = ultimaCaptura_ms;
  interrupts();

  if ((millis() - t) > (uint32_t)RPM_TIMEOUT_MS || p == 0) return 0.0f;

  float rpmBruto = (1000000.0f / (float)p * 60.0f) / (float)PULSOS_POR_VOLTA;

  static float filtrado = 0.0f;
  filtrado = (filtrado * 0.8f) + (rpmBruto * 0.2f);
  return filtrado;
}