#include <Arduino.h>
#include "monitor.h"
#include "pids.h"
#include "config.h"

// =============================================================================
// HAL - MONITOR: Tensão da Fonte, Tensão do Motor e Potência Média
// =============================================================================
//
// CIRCUITO (para cada canal):
//
//   V_alta ──[ R1 = 180kΩ ]──┬──[ R2 = 3kΩ ]── GND
//                             │
//                           PA2/PA3 (ADC, max 3.3V)
//                             │
//                    [ C = 100nF ]── GND  (filtro RC anti-aliasing)
//
// CONVERSÃO:
//   V_adc  = leitura × ADC_VREF / ADC_RESOLUCAO
//   V_real = V_adc × DIVISOR_TENSAO
//
// CÁLCULOS:
//   tensaoMotor  = tensaoFonte - tensaoMosfet
//   potenciaMotor = tensaoMotor × inputCurrent
//
// FILTRAGEM EMA:
//   filtrado = filtrado × (1 - alfa) + novo × alfa
//   alfa = MONITOR_EMA_ALFA (0.05) — mais suave que o RPM (0.2)
//   Justificativa: tensão de fonte varia lentamente, suavização maior
//   reduz ruído sem impactar a utilidade do dado de monitoramento.
// =============================================================================

float tensaoFonte  = 0.0f;
float tensaoMotor  = 0.0f;
float potenciaMotor = 0.0f;
bool  fonteLigada  = false;

static float _tensaoFonteEMA  = 0.0f;
static float _tensaoMosfetEMA = 0.0f;

static float lerTensao(uint8_t pino) {
  float vadc = analogRead(pino) * ADC_VREF / ADC_RESOLUCAO;
  return vadc * DIVISOR_TENSAO;
}

void atualizarMonitor() {
  // Leituras brutas
  float vFonteBruta  = lerTensao(PIN_TENSAO_FONTE);
  float vMosfetBruta = lerTensao(PIN_TENSAO_MOSFET);

  // Filtro EMA
  _tensaoFonteEMA  = _tensaoFonteEMA  * (1.0f - MONITOR_EMA_ALFA) + vFonteBruta  * MONITOR_EMA_ALFA;
  _tensaoMosfetEMA = _tensaoMosfetEMA * (1.0f - MONITOR_EMA_ALFA) + vMosfetBruta * MONITOR_EMA_ALFA;

  // Tensão da fonte: diretamente da EMA
  tensaoFonte = _tensaoFonteEMA;

  // Tensão do motor: diferença entre fonte e MOSFET
  // Garante que nunca seja negativa por ruído de leitura
  tensaoMotor = max(0.0f, _tensaoFonteEMA - _tensaoMosfetEMA);

  // Potência média: usa inputCurrent do PID (já filtrada pelo controle)
  // noInterrupts/interrupts garante leitura atômica da variável volatile
  noInterrupts();
  float corrente = inputCurrent;
  interrupts();
  potenciaMotor = tensaoMotor * corrente;

  // Status da fonte
  fonteLigada = (tensaoFonte > TENSAO_FONTE_MINIMA);
}
