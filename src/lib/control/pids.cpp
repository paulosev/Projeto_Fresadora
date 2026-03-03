#include <Arduino.h>
#include "pids.h"
#include "pwm.h"
#include "config.h"

volatile float inputRPM           = 0.0f;
volatile float setpointRPM        = 0.0f;
volatile float outCurrentSetpoint = 0.0f;
volatile float inputCurrent       = 0.0f;
volatile float setpointCurrent    = 0.0f;
volatile float outputPWM_Val      = 0.0f;

// Ganhos armazenados localmente — QuickPID não expõe GetTunings()
float pidRPM_Kp = PID_RPM_KP;
float pidRPM_Ki = PID_RPM_KI;
float pidRPM_Kd = PID_RPM_KD;
float pidI_Kp   = PID_CORRENTE_KP;
float pidI_Ki   = PID_CORRENTE_KI;
float pidI_Kd   = PID_CORRENTE_KD;

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
  pidRPM.SetTunings(pidRPM_Kp, pidRPM_Ki, pidRPM_Kd);
  pidRPM.SetMode(QuickPID::Control::automatic);

  pidCorrente.SetSampleTimeUs(PID_CORRENTE_TS);
  pidCorrente.SetOutputLimits(0, MAX_PWM);
  pidCorrente.SetTunings(pidI_Kp, pidI_Ki, pidI_Kd);
  pidCorrente.SetDerivativeMode(QuickPID::dMode::dOnMeas);
  pidCorrente.SetMode(QuickPID::Control::automatic);
}

void setGanhosRPM(float kp, float ki, float kd) {
  pidRPM_Kp = kp; pidRPM_Ki = ki; pidRPM_Kd = kd;
  pidRPM.SetTunings(kp, ki, kd);
}

void setGanhosCorrente(float kp, float ki, float kd) {
  pidI_Kp = kp; pidI_Ki = ki; pidI_Kd = kd;
  pidCorrente.SetTunings(kp, ki, kd);
}

void processarCascata() {
  pidRPM.Compute();
  setpointCurrent = outCurrentSetpoint;
  pidCorrente.Compute();
  aplicarPWM(outputPWM_Val);
}
