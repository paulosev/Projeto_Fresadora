#pragma once

/**
 * @file encoder.h
 * @brief HAL — Leitura do encoder rotativo KY-040 (CLK/DT/SW).
 *
 * Detecta rotação (sentido horário / anti-horário) e eventos do botão
 * (pressão curta e pressão longa) via polling no loop().
 *
 * @par Pinos (definidos em config.h)
 * - PB12 = CLK (A)
 * - PB13 = DT  (B)
 * - PB14 = SW  (botão, ativo baixo com INPUT_PULLUP)
 *
 * @par Detecção de Rotação
 * Lê CLK e DT a cada chamada de lerEncoder(). Detecta borda de descida
 * no CLK e compara com DT para determinar sentido:
 * - DT HIGH na borda de CLK → sentido horário  → delta = +1
 * - DT LOW  na borda de CLK → anti-horário     → delta = -1
 *
 * @par Eventos do Botão
 * - Pressão curta  (< ENC_LONG_PRESS_MS): encBotaoCurto = true por 1 ciclo
 * - Pressão longa  (≥ ENC_LONG_PRESS_MS): encBotaoLongo = true por 1 ciclo
 */

/**
 * @brief Delta acumulado de rotação desde a última leitura.
 * Positivo = horário, negativo = anti-horário.
 * Zerado após cada chamada a lerEncoder().
 */
extern volatile int8_t encDelta;

/** @brief true por exatamente 1 ciclo de loop() após pressão curta. */
extern volatile bool encBotaoCurto;

/** @brief true por exatamente 1 ciclo de loop() após pressão longa. */
extern volatile bool encBotaoLongo;

/**
 * @brief Inicializa os pinos do encoder com INPUT_PULLUP.
 * Deve ser chamada no setup().
 */
void setupEncoder();

/**
 * @brief Lê o estado atual do encoder e atualiza encDelta, encBotaoCurto e encBotaoLongo.
 * Deve ser chamada a cada iteração do loop().
 * Consome ~10µs por chamada (apenas leitura digital + lógica).
 */
void lerEncoder();
