#include <Arduino.h>
#include "autotune.h"
#include "pids.h"
#include "lib/hal/pwm.h"
#include "lib/hal/corrente.h"
#include "controle.h"
#include "config.h"
#include <sTune.h>

// =============================================================================
// CONTROL - AUTOTUNE: Sintonia Automática da Malha de Corrente (sTune)
// =============================================================================
//
// MÉTODO: Ziegler-Nichols Step Test (ZN_PID)
//   Aplica um degrau de PWM e mede a resposta da corrente para calcular
//   Kp, Ki e Kd automaticamente.
//
// PARÂMETROS ESCALADOS PARA RESOLUÇÃO DE 4500 DEGRAUS (STM32):
//   pwmInicial = 450  (~10% de MAX_PWM)
//   pwmDegrau  = 1350 (~30% de MAX_PWM)
//
// SERIAL PLOTTER (dois canais separados por vírgula):
//   Canal 1: corrente real (0–20A)
//   Canal 2: PWM normalizado (0–20), fator = MAX_PWM / 20 = 225
// =============================================================================

static const float PLOTTER_FATOR = (float)MAX_PWM / 20.0f;

sTune tuner(
  (float*)&inputCurrent,
  (float*)&outputPWM_Val,
  tuner.ZN_PID,
  (sTune::Action)0,
  (sTune::SerialMode)1
);

void executarAutotuneCorrente() {
  Serial.println(F("\n[!] INICIANDO AUTOTUNE DE CORRENTE EM 5 SEGUNDOS..."));
  Serial.println(F("[!] MANTENHA O EIXO DESIMPEDIDO E AFASTE-SE!"));
  delay(5000);

  pausarTimerControle();

  tuner.Configure(450, 1350, 0, 1, 5, 1, MAX_PWM);

  while ((int)tuner.Run() != 0) {
    inputCurrent = lerCorrente();
    aplicarPWM(outputPWM_Val);

    Serial.print(inputCurrent);
    Serial.print(",");
    Serial.println(outputPWM_Val / PLOTTER_FATOR);

    if (inputCurrent > MAX_AMPERE) {
      aplicarPWM(0);
      Serial.println(F("ERRO: CORRENTE ALTA NO TESTE! AUTOTUNE ABORTADO."));
      retormarTimerControle();
      return;
    }
  }

  float kp = tuner.GetKp();
  float ki = tuner.GetKi();
  float kd = tuner.GetKd();
  pidCorrente.SetTunings(kp, ki, kd);

  Serial.println(F(">>> AUTOTUNE CONCLUIDO COM SUCESSO! <<<"));
  Serial.print(F("Novos Ganhos - Kp: ")); Serial.print(kp);
  Serial.print(F("  Ki: "));              Serial.print(ki);
  Serial.print(F("  Kd: "));              Serial.println(kd);

  aplicarPWM(0);
  delay(2000);

  retormarTimerControle();
}
