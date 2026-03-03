#include <Arduino.h>
#include "pwm.h"
#include "config.h"

// =============================================================================
// HAL - PWM: TIM1 CH1 (PA8) @ 16kHz
// =============================================================================
//
// CONFIGURAÇÃO DO TIM1:
//   PSC = 0    → Clock do timer = 72MHz
//   ARR = 4499 → Frequência = 72.000.000 / 4500 = 16.000 Hz
//   Resolução: 4500 degraus (0 a 4499)
//   Modo: PWM Modo 1 (OC1M = 110), preload habilitado.
//
// ATENÇÃO — TIM1 É ADVANCED TIMER:
//   Requer MOE (Main Output Enable) no registrador BDTR.
//   Sem esta linha, o PA8 não gera sinal — erro silencioso mais comum!
//
// PINO PA8: CNF = 10 (alt func push-pull), MODE = 11 (50MHz) → registrador CRH
// =============================================================================

void setupHardwarePWM() {
  // 1. Habilitar clocks do GPIOA e TIM1
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

  // 2. Configurar PA8: saída alternativa push-pull, 50MHz
  GPIOA->CRH &= ~(GPIO_CRH_CNF8  | GPIO_CRH_MODE8);
  GPIOA->CRH |=  (GPIO_CRH_CNF8_1 | GPIO_CRH_MODE8_0 | GPIO_CRH_MODE8_1);

  // 3. Frequência e resolução
  TIM1->PSC = TIM1_PSC;
  TIM1->ARR = TIM1_ARR;
  TIM1->CNT = 0;

  // 4. PWM Modo 1 no CH1 (OC1M = 110): saída alta enquanto CNT < CCR1
  TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
  TIM1->CCMR1 |=  (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
  TIM1->CCMR1 |=   TIM_CCMR1_OC1PE;  // Preload: atualização suave no overflow

  // 5. Saída CH1: polaridade ativa alta, canal habilitado
  TIM1->CCER &= ~TIM_CCER_CC1P;
  TIM1->CCER |=  TIM_CCER_CC1E;

  // 6. OBRIGATÓRIO para Advanced Timer: Main Output Enable
  TIM1->BDTR |= TIM_BDTR_MOE;

  // 7. Duty cycle inicial = 0
  TIM1->CCR1 = 0;

  // 8. Auto-reload preload + iniciar contagem
  TIM1->CR1 |= TIM_CR1_ARPE;
  TIM1->CR1 |= TIM_CR1_CEN;
}

void aplicarPWM(float valor) {
  TIM1->CCR1 = (uint32_t)constrain(valor, 0.0f, (float)MAX_PWM);
}
