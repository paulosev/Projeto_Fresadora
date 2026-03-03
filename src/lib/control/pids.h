#pragma once
#include <QuickPID.h>

/**
 * @file pids.h
 * @brief Control — PIDs em cascata: malha de velocidade (RPM) e corrente.
 */

/** @brief RPM medido pelo sensor Hall (entrada da malha externa). */
extern volatile float inputRPM;
/** @brief Setpoint de velocidade em RPM (escrito pelo soft-start). */
extern volatile float setpointRPM;
/** @brief Saída da malha de RPM = setpoint de corrente para malha interna. */
extern volatile float outCurrentSetpoint;
/** @brief Corrente medida pelo ACS712 (entrada da malha interna). */
extern volatile float inputCurrent;
/** @brief Setpoint de corrente (ponte entre malha externa e interna). */
extern volatile float setpointCurrent;
/** @brief Saída da malha de corrente = duty cycle para o TIM1. */
extern volatile float outputPWM_Val;

/** @brief Instância do PID de velocidade (malha externa, 100Hz). */
extern QuickPID pidRPM;
/** @brief Instância do PID de corrente (malha interna, 2kHz). */
extern QuickPID pidCorrente;

// Ganhos armazenados separadamente — QuickPID não tem GetTunings()
extern float pidRPM_Kp, pidRPM_Ki, pidRPM_Kd;
extern float pidI_Kp,   pidI_Ki,   pidI_Kd;

/**
 * @brief Inicializa ambas as malhas PID com ganhos e sample times.
 */
void setupPIDs();

/**
 * @brief Aplica novos ganhos ao PID de RPM e atualiza as variáveis globais.
 */
void setGanhosRPM(float kp, float ki, float kd);

/**
 * @brief Aplica novos ganhos ao PID de corrente e atualiza as variáveis globais.
 */
void setGanhosCorrente(float kp, float ki, float kd);

/**
 * @brief Executa um ciclo completo do controle em cascata.
 * @warning Chamar exclusivamente da ISR do TIM3.
 */
void processarCascata();
