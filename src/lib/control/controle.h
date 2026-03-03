#pragma once

// =============================================================================
// HAL - CONTROLE: TIM3 @ 2kHz — Loop de Controle Determinístico
// =============================================================================
// Garante que leitura de sensores e execução dos PIDs ocorram SEMPRE a cada
// 500µs, independente do que o loop() estiver fazendo.
// A ISR do TIM3 está implementada em controle.cpp.
// =============================================================================

// Inicializa o TIM3 a 2kHz com prioridade máxima no NVIC.
// Deve ser chamado após setupHardwarePWM(), setupRPM() e setupPIDs().
void setupTimerControle();

// Suspende o TIM3 — usado pelo Autotune para assumir controle exclusivo do PWM.
void pausarTimerControle();

// Retoma o TIM3 após o Autotune.
void retormarTimerControle();
