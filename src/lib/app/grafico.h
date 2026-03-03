#pragma once

/**
 * @file grafico.h
 * @brief App — Buffer circular para o gráfico de RPM vs Setpoint.
 *
 * Armazena os últimos GRAFICO_AMOSTRAS valores de RPM e setpoint em
 * buffers circulares. Cada coluna de pixel do display (0–127) corresponde
 * a uma amostra, sendo a mais recente sempre na coluna 127 (direita).
 *
 * @par Janela de Tempo
 * GRAFICO_AMOSTRAS (128) × GRAFICO_INTERVALO_MS (78ms) ≈ 10 segundos.
 *
 * @par Escala Y Automática
 * A cada chamada a atualizarGrafico(), os valores mínimo e máximo do
 * buffer são recalculados para ajuste automático da escala vertical.
 *
 * @par Pausa
 * graficoPausado = true congela o buffer — útil para analisar transientes.
 */

/** @brief Buffer circular de RPM (GRAFICO_AMOSTRAS entradas). */
extern float grafRPM[128];

/** @brief Buffer circular de setpoint (GRAFICO_AMOSTRAS entradas). */
extern float grafSP[128];

/** @brief Índice da próxima escrita no buffer circular. */
extern uint8_t grafIdx;

/** @brief Valor mínimo atual do buffer de RPM (para escala Y). */
extern float grafMin;

/** @brief Valor máximo atual do buffer de RPM + SP (para escala Y). */
extern float grafMax;

/** @brief Se true, o buffer não é atualizado (pausa na captura). */
extern bool graficoPausado;

/**
 * @brief Inicializa os buffers com zero e reseta o índice.
 * Deve ser chamada no setup().
 */
void setupGrafico();

/**
 * @brief Coleta uma nova amostra de RPM e setpoint no buffer circular.
 *
 * Deve ser chamada no loop() a cada GRAFICO_INTERVALO_MS.
 * Se graficoPausado == true, retorna imediatamente sem alterar o buffer.
 * Atualiza grafMin e grafMax para escala Y automática.
 */
void atualizarGrafico();
