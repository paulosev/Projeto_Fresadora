#include <Arduino.h>
#include "encoder.h"
#include "config.h"

// =============================================================================
// HAL - ENCODER: KY-040 — Rotação + Botão via Polling
// =============================================================================
// Polling é preferível a interrupção para o encoder neste projeto pois:
//   1. TIM2 e TIM3 já têm ISRs com prioridade máxima
//   2. O encoder não precisa de resposta em µs — 10ms de latência é imperceptível
//   3. Evita bounce causando falsos disparos na ISR
//
// DETECÇÃO DE ROTAÇÃO:
//   Monitora borda de descida no CLK. Quando CLK cai:
//   DT=HIGH → horário (+1) | DT=LOW → anti-horário (-1)
//
// BOTÃO:
//   Mede duração do pressionamento. Ao soltar:
//   < ENC_LONG_PRESS_MS → curto | ≥ ENC_LONG_PRESS_MS → longo
// =============================================================================

volatile int8_t encDelta      = 0;
volatile bool   encBotaoCurto = false;
volatile bool   encBotaoLongo = false;

static bool     _clkAnterior     = HIGH;
static bool     _swAnterior      = HIGH;
static uint32_t _swPressadoEm    = 0;
static bool     _swPressionado   = false;

void setupEncoder() {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW,  INPUT_PULLUP);
  _clkAnterior = digitalRead(PIN_ENC_CLK);
  _swAnterior  = digitalRead(PIN_ENC_SW);
}

void lerEncoder() {
  // Zera flags de evento (válidas só por 1 ciclo)
  encBotaoCurto = false;
  encBotaoLongo = false;

  // ── ROTAÇÃO ──
  bool clkAtual = digitalRead(PIN_ENC_CLK);
  if (_clkAnterior == HIGH && clkAtual == LOW) {
    // Borda de descida no CLK
    if (digitalRead(PIN_ENC_DT) == HIGH)
      encDelta++;   // Horário
    else
      encDelta--;   // Anti-horário
  }
  _clkAnterior = clkAtual;

  // ── BOTÃO ──
  bool swAtual = digitalRead(PIN_ENC_SW);

  if (_swAnterior == HIGH && swAtual == LOW) {
    // Borda de descida — botão pressionado
    _swPressadoEm  = millis();
    _swPressionado = true;
  }

  if (_swAnterior == LOW && swAtual == HIGH && _swPressionado) {
    // Borda de subida — botão solto
    uint32_t duracao = millis() - _swPressadoEm;
    if (duracao >= (uint32_t)ENC_LONG_PRESS_MS)
      encBotaoLongo = true;
    else if (duracao >= (uint32_t)ENC_DEBOUNCE_MS)
      encBotaoCurto = true;
    _swPressionado = false;
  }

  _swAnterior = swAtual;
}
