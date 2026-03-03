#include <Arduino.h>
#include "corrente.h"
#include "config.h"

// =============================================================================
// HAL - CORRENTE: ADC PA1 — ACS712-30A
// =============================================================================
//
// ACS712-30A COM VCC = 3.3V:
//   Saída central (zero corrente) = Vcc / 2 = 1.65V
//   Sensibilidade = 66mV/A
//   Corrente (A) = (Tensão_lida - 1.65) / 0.066
//
// ADC STM32F103:
//   Resolução: 12 bits (0–4095), Referência: 3.3V
// =============================================================================

float lerCorrente() {
  int   leitura = analogRead(PIN_ACS712);
  float tensao  = (leitura * ADC_VREF) / ADC_RESOLUCAO;
  float amp     = (tensao - ACS712_OFFSET) / ACS712_SENS;
  return fabsf(amp);
}
