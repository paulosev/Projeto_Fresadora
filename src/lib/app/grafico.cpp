#include <Arduino.h>
#include "grafico.h"
#include "pids.h"
#include "config.h"

// =============================================================================
// APP - GRÁFICO: Buffer Circular RPM vs Setpoint (~10s janela)
// =============================================================================
// Uso de memória:
//   2 buffers × 128 floats × 4 bytes = 1024 bytes (1KB de RAM)
//   STM32F103C8T6 tem 20KB de RAM — sem problema.
//
// ESCALA Y AUTOMÁTICA:
//   grafMin = mínimo entre todos os valores de RPM no buffer
//   grafMax = máximo entre RPM e SP no buffer
//   A tela de display mapeia [grafMin, grafMax] → [63, 0] pixels
// =============================================================================

float   grafRPM[GRAFICO_AMOSTRAS] = {0};
float   grafSP [GRAFICO_AMOSTRAS] = {0};
uint8_t grafIdx     = 0;
float   grafMin     = 0.0f;
float   grafMax     = 10000.0f;
bool    graficoPausado = false;

void setupGrafico() {
  for (uint8_t i = 0; i < GRAFICO_AMOSTRAS; i++) {
    grafRPM[i] = 0.0f;
    grafSP[i]  = 0.0f;
  }
  grafIdx = 0;
  grafMin = 0.0f;
  grafMax = 10000.0f;
}

void atualizarGrafico() {
  if (graficoPausado) return;

  // Leitura atômica das variáveis volatile do PID
  noInterrupts();
  float rpm = inputRPM;
  float sp  = setpointRPM;
  interrupts();

  // Escreve no buffer circular
  grafRPM[grafIdx] = rpm;
  grafSP [grafIdx] = sp;
  grafIdx = (grafIdx + 1) % GRAFICO_AMOSTRAS;

  // Recalcula escala Y — percorre buffer completo
  float vmin =  99999.0f;
  float vmax = -99999.0f;
  for (uint8_t i = 0; i < GRAFICO_AMOSTRAS; i++) {
    if (grafRPM[i] < vmin) vmin = grafRPM[i];
    if (grafRPM[i] > vmax) vmax = grafRPM[i];
    if (grafSP [i] > vmax) vmax = grafSP[i];
  }
  // Margem de 5% para não colar nas bordas
  float margem = (vmax - vmin) * 0.05f;
  grafMin = max(0.0f, vmin - margem);
  grafMax = vmax + margem + 1.0f; // +1 evita divisão por zero se todos iguais
}
