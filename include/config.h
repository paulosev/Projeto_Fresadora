#pragma once

// =============================================================================
// CONFIGURAÇÃO CENTRAL DO PROJETO - Projeto_Fresadora
// =============================================================================
// Todas as constantes de hardware, limites e parâmetros de controle estão
// centralizados aqui. Para ajustar qualquer parâmetro, edite apenas este arquivo.
// =============================================================================

// --- PINOS DE HARDWARE ---
#define PIN_PWM          PA8    // TIM1 CH1 - Saída PWM de potência
#define PIN_ACS712       PA1    // ADC1 CH1 - Sensor de corrente ACS712-30A
#define PIN_HALL         PA0    // TIM2 CH1 - Sensor Hall (Input Capture)

// --- PARÂMETROS DO MOTOR ---
#define PULSOS_POR_VOLTA 4      // Número de pulsos por revolução do anel magnético
#define RPM_MINIMO       1000   // RPM mínimo operacional
#define RPM_MAXIMO       10000  // RPM máximo operacional

// --- LIMITES DE SEGURANÇA ---
#define MAX_AMPERE       20.0f  // Limite de corrente do IRFP460 (Amperes)
#define PWM_STALL_LIMITE 0.88f  // Fração do PWM máximo que indica possível stall
#define RPM_STALL_LIMITE 100.0f // RPM abaixo do qual considera motor travado
#define SP_STALL_MINIMO  500.0f // Setpoint mínimo para checar condição de stall

// --- CONFIGURAÇÃO DO PWM (TIM1) ---
// PSC = 0, ARR = 4499 → f = 72MHz / 4500 = 16.000 Hz
#define TIM1_PSC         0
#define TIM1_ARR         4499
#define MAX_PWM          (TIM1_ARR)  // Resolução: 4500 degraus (0 a 4499)

// --- CONFIGURAÇÃO DO INPUT CAPTURE (TIM2) ---
// PSC = 71 → Clock = 72MHz / 72 = 1MHz → resolução de 1µs
// ARR = 0xFFFF → período máximo = 65.535ms → RPM mínimo ≈ 229 RPM
#define TIM2_PSC         71
#define TIM2_ARR         0xFFFF
#define RPM_FILTRO_RUIDO 250    // Pulsos menores que 250µs são ignorados (anti-ruído)
#define RPM_TIMEOUT_MS   250    // Tempo sem pulso para considerar motor parado (ms)

// --- CONFIGURAÇÃO DO LOOP DE CONTROLE (TIM3) ---
// PSC = 71, ARR = 499 → f = 1MHz / 500 = 2.000 Hz (período de 500µs)
#define TIM3_PSC         71
#define TIM3_ARR         499

// --- PRIORIDADES NVIC ---
#define NVIC_PRIO_TIM3   0      // Loop de controle — máxima prioridade
#define NVIC_PRIO_TIM2   1      // Input Capture Hall — logo abaixo

// --- PARÂMETROS DO ADC (ACS712-30A com Vcc = 3.3V) ---
#define ADC_VREF         3.3f
#define ADC_RESOLUCAO    4095.0f
#define ACS712_OFFSET    1.65f  // Vcc / 2
#define ACS712_SENS      0.066f // 66mV/A (modelo 30A)

// --- GANHOS PID (valores iniciais — rode Autotune para calibrar) ---
// Malha de Velocidade (Externa) - 100Hz
#define PID_RPM_KP       1.2f
#define PID_RPM_KI       0.5f
#define PID_RPM_KD       0.05f
#define PID_RPM_TS       10000  // Sample time em µs (100Hz)

// Malha de Corrente (Interna) - 2kHz
// Ganhos escalados x9 em relação ao Arduino (resolução 4500 vs 500).
// Execute o Autotune na primeira utilização para refinar.
#define PID_CORRENTE_KP  18.0f
#define PID_CORRENTE_KI  135.0f
#define PID_CORRENTE_KD  0.0f
#define PID_CORRENTE_TS  500    // Sample time em µs (2kHz)

// --- SOFT-START ---
#define SOFTSTART_ALVO   5000.0f // RPM alvo de trabalho
#define SOFTSTART_PASSO  0.1f    // Incremento de RPM por ciclo do loop()
