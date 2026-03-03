#pragma once
#include <QuickPID.h>

// =============================================================================
// CONTROL - PIDs: Malhas de Velocidade e Corrente
// =============================================================================
// Variáveis globais de estado do controle em cascata.
// Definidas em pids.cpp — compartilhadas entre hal/controle.cpp,
// app/maquina.cpp e control/autotune.cpp.
// =============================================================================

// Malha de velocidade (externa)
extern volatile float inputRPM;
extern volatile float setpointRPM;
extern volatile float outCurrentSetpoint;

// Malha de corrente (interna)
extern volatile float inputCurrent;
extern volatile float setpointCurrent;
extern volatile float outputPWM_Val;

// Instâncias dos PIDs (acessadas pelo Autotune para aplicar novos ganhos)
extern QuickPID pidRPM;
extern QuickPID pidCorrente;

// Inicializa ganhos, sample times e limites de saída de ambas as malhas.
void setupPIDs();

// Executa um ciclo completo do controle em cascata.
// Chamado exclusivamente pela ISR do TIM3 (hal/controle.cpp).
void processarCascata();
