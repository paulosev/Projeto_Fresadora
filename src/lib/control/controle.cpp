#include <Arduino.h>
#include "controle.h"
#include "rpm.h"
#include "corrente.h"
#include "pwm.h"
#include "pids.h"
#include "autotune.h"
#include "maquina.h"
#include "config.h"


// =============================================================================
// HAL - CONTROLE: TIM3 @ 2kHz via HardwareTimer (STM32duino)
// =============================================================================
// O STM32duino já define TIM3_IRQHandler em HardwareTimer.cpp.
// Declarar extern "C" void TIM3_IRQHandler() causaria "multiple definition".
// Solução: usar HardwareTimer com attachInterrupt() para registrar o callback
// do loop de controle — o framework despacha para a função corretamente.
//
// Prioridade do TIM3 é ajustada manualmente após a configuração do HardwareTimer,
// pois a API não expõe SetPriority diretamente.
//
// Tempo estimado do callback: ~25µs de 500µs disponíveis → ~95% CPU livre.
// =============================================================================

HardwareTimer* timerControle = nullptr;

static void loopControle() {
  inputCurrent = lerCorrente();
  inputRPM     = calcularRPM();

  processarCascata();

  if (inputCurrent > MAX_AMPERE)
    paradaEmergencia("SOBRECORRENTE");

  if (outputPWM_Val > (MAX_PWM * PWM_STALL_LIMITE) &&
      inputRPM      < RPM_STALL_LIMITE              &&
      setpointRPM   > SP_STALL_MINIMO)
    paradaEmergencia("STALL / MOTOR TRAVADO");
}

void setupTimerControle() {
  timerControle = new HardwareTimer(TIM3);
  timerControle->setOverflow(2000, HERTZ_FORMAT);   // 2kHz
  timerControle->attachInterrupt(loopControle);

  // Ajusta prioridade do TIM3 para máxima após configuração do HardwareTimer
  NVIC_SetPriority(TIM3_IRQn, NVIC_PRIO_TIM3);

  timerControle->resume();
}

void pausarTimerControle() {
  if (timerControle) timerControle->pause();
}

void retormarTimerControle() {
  if (timerControle) timerControle->resume();
}