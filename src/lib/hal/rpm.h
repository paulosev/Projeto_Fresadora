#pragma once

// =============================================================================
// HAL - RPM: TIM2 CH1 (PA0) — Input Capture
// =============================================================================
// Medição de RPM via captura de período entre pulsos Hall.
// A ISR do TIM2 está implementada em rpm.cpp.
// =============================================================================

// Configura TIM2 em modo Input Capture, borda de descida, em PA0.
void setupRPM();

// Retorna o RPM calculado a partir do último período capturado.
// Retorna 0.0 se o motor estiver parado (timeout de RPM_TIMEOUT_MS sem pulso).
// Aplica filtro EMA para suavizar a leitura antes de passar ao PID.
float calcularRPM();
