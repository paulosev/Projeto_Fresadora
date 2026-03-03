#include <Arduino.h>
#include "maquina.h"
#include "lib/control/pids.h"
#include "lib/hal/pwm.h"
#include "lib/control/controle.h"
#include "config.h"

// =============================================================================
// APP - MÁQUINA: Lógica de Alto Nível
// =============================================================================
//
// SOFT-START:
//   Incrementa setpointRPM gradualmente até SOFTSTART_ALVO para proteger
//   a mecânica e os MOSFETs de partidas abruptas.
//
// PARADA DE EMERGÊNCIA:
//   Zera CCR1 do TIM1 diretamente para o corte mais rápido possível.
//   Para o TIM3 para impedir que o loop de controle reative o PWM.
//   O TIM1 continua rodando para manter o gate do IR2110S em estado definido.
// =============================================================================

void executarSoftStart() {
  float sp = setpointRPM;
  if      (sp < SOFTSTART_ALVO) setpointRPM = sp + SOFTSTART_PASSO;
  else if (sp > SOFTSTART_ALVO) setpointRPM = sp - SOFTSTART_PASSO;
}

void paradaEmergencia(const char* msg) {
  TIM1->CCR1 = 0;
  pausarTimerControle();

  while (1) {
    Serial.print(F("!!! ERRO CRITICO: "));
    Serial.println(msg);
    delay(2000);
  }
}
