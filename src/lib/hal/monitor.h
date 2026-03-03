#pragma once

/**
 * @file monitor.h
 * @brief HAL — Monitoramento de tensão da fonte, tensão do motor e potência.
 *
 * Lê dois divisores de tensão via ADC (PA2 e PA3) e calcula:
 * - Tensão média da fonte DC
 * - Tensão média sobre o MOSFET (usado apenas internamente no cálculo)
 * - Tensão média do motor = V_fonte - V_mosfet
 * - Potência média do motor = V_motor × inputCurrent
 *
 * @par Divisor de Tensão (R1 = 180kΩ, R2 = 3kΩ)
 * @code
 * Razão = (R1 + R2) / R2 = 61
 * V_real = V_adc × razão
 * V_max  = 3.3V × 61 = 201.3V
 * @endcode
 *
 * @par Prioridade de Execução
 * Esta feature é exclusivamente de monitoramento — não influencia o controle.
 * Deve ser chamada no loop(), nunca na ISR do TIM3.
 * O filtro EMA com alfa baixo (0.05) garante leituras estáveis sem sobrecarregar o ADC.
 *
 * @par Detecção de Fonte Ligada
 * tensaoFonte > TENSAO_FONTE_MINIMA indica que a fonte está ativa.
 * Útil para inibir o controle se a fonte for desligada inesperadamente.
 */

/**
 * @brief Tensão média da fonte DC em Volts (filtrada por EMA).
 * @note Válida após primeira chamada a atualizarMonitor().
 *       Útil para detectar se a fonte está ligada (> TENSAO_FONTE_MINIMA).
 */
extern float tensaoFonte;

/**
 * @brief Tensão média sobre o motor em Volts (filtrada por EMA).
 * @note Calculada como: tensaoFonte - tensaoMosfet.
 *       Representa a tensão efetiva aplicada à carga.
 */
extern float tensaoMotor;

/**
 * @brief Potência média do motor em Watts.
 * @note Calculada como: tensaoMotor × inputCurrent.
 *       inputCurrent é a mesma variável usada pelo PID de corrente.
 */
extern float potenciaMotor;

/**
 * @brief Indica se a fonte DC está ligada.
 * @note true quando tensaoFonte > TENSAO_FONTE_MINIMA.
 */
extern bool fonteLigada;

/**
 * @brief Atualiza todas as leituras de monitoramento.
 *
 * Realiza as leituras ADC em PA2 (fonte) e PA3 (MOSFET), aplica o
 * filtro EMA, calcula tensaoMotor, potenciaMotor e atualiza fonteLigada.
 *
 * Deve ser chamada periodicamente no loop(). Não chamar na ISR do TIM3.
 * Cada chamada realiza 2 leituras ADC — tempo de execução ~5µs.
 */
void atualizarMonitor();
