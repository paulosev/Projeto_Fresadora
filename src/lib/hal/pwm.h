#pragma once

// =============================================================================
// HAL - PWM: TIM1 CH1 (PA8) @ 16kHz
// =============================================================================
// Interface para configuração e controle do sinal PWM de potência.
// =============================================================================

// Inicializa o TIM1 em modo PWM, 16kHz, resolução MAX_PWM (4499 degraus).
void setupHardwarePWM();

// Aplica um valor de duty cycle ao TIM1 CCR1.
// Valor é automaticamente limitado ao range [0, MAX_PWM].
void aplicarPWM(float valor);
