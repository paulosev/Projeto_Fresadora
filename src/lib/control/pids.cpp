#include <Arduino.h>
#include "pids.h"
#include "lib/hal/pwm.h"
#include "config.h"

// =============================================================================
// CONTROL - PIDs: Malhas de Velocidade e Corrente
// =============================================================================
//
// ARQUITETURA DE CONTROLE EM CASCATA:
//
//   Setpoint RPM → [ PID RPM ] → Setpoint Corrente → [ PID Corrente ] → PWM
//         ↑                              ↑
//   Sensor Hall (TIM2)           Sensor ACS712 (ADC)
//
// MALHA EXTERNA (RPM) - 100Hz:
//   Gera um alvo de corrente limitado a MAX_AMPERE.
//
// MALHA INTERNA (CORRENTE) - 2kHz:
//   Ajusta o duty cycle PWM. D = 0 e dOnMeas para evitar ruído elétrico.
//   Saída limitada a MAX_PWM (4499 degraus do TIM1).
// =============================================================================

volatile float inputRPM           = 0.0f;
volatile float setpointRPM        = 0.0f;
volatile float outCurrentSetpoint = 0.0f;
volatile float inputCurrent       = 0.0f;
volatile float setpointCurrent    = 0.0f;
volatile float outputPWM_Val      = 0.0f;

QuickPID pidRPM(
  (float*)&inputRPM,
  (float*)&outCurrentSetpoint,
  (float*)&setpointRPM
);

QuickPID pidCorrente(
  (float*)&inputCurrent,
  (float*)&outputPWM_Val,
  (float*)&setpointCurrent
);

void setupPIDs() {
  pidRPM.SetSampleTimeUs(PID_RPM_TS);
  pidRPM.SetOutputLimits(0, MAX_AMPERE);
  pidRPM.SetTunings(PID_RPM_KP, PID_RPM_KI, PID_RPM_KD);
  pidRPM.SetMode(QuickPID::Control::automatic);

  pidCorrente.SetSampleTimeUs(PID_CORRENTE_TS);
  pidCorrente.SetOutputLimits(0, MAX_PWM);
  pidCorrente.SetTunings(PID_CORRENTE_KP, PID_CORRENTE_KI, PID_CORRENTE_KD);
  pidCorrente.SetDerivativeMode(QuickPID::dMode::dOnMeas);
  pidCorrente.SetMode(QuickPID::Control::automatic);
}

void processarCascata() {
  pidRPM.Compute();
  setpointCurrent = outCurrentSetpoint;
  pidCorrente.Compute();
  aplicarPWM(outputPWM_Val);
}
