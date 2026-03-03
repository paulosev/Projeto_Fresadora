#pragma once

// =============================================================================
// CONTROL - AUTOTUNE: Sintonia Automática da Malha de Corrente (sTune)
// =============================================================================
// Executa um Step Test na malha de corrente para calcular os ganhos PID ideais.
//
// ATENÇÃO: O motor girará brevemente durante o teste. Afaste-se do eixo!
// =============================================================================

// Executa o processo completo de autotune:
//   1. Suspende o TIM3 (controle determinístico).
//   2. Realiza o Step Test via sTune (método Ziegler-Nichols).
//   3. Aplica os novos ganhos ao pidCorrente.
//   4. Retoma o TIM3.
void executarAutotuneCorrente();
