#pragma once

/**
 * @file display.h
 * @brief App — Interface gráfica: SSD1306 + Encoder KY-040.
 *
 * Gerencia a navegação entre telas, o menu de configuração e a edição
 * de valores via encoder. Toda a lógica de UI roda no loop() — nunca
 * na ISR do TIM3.
 *
 * @par Telas disponíveis
 * | ID | Nome          | Descrição                                      |
 * |----|---------------|------------------------------------------------|
 * |  0 | Principal     | RPM + setpoint + estado do motor + barra PWM  |
 * |  1 | Monitor Motor | Tensão motor + corrente + potência             |
 * |  2 | Monitor Fonte | Tensão fonte + status ligada/desligada         |
 * |  3 | Gráfico PID   | RPM vs Setpoint, 128 amostras (~10s)           |
 * |  4 | Diagnóstico   | PWM bruto + setpoint de corrente interno       |
 *
 * @par Navegação
 * - Girar encoder: avança entre telas (circular 0→4→0)
 * - Tela 0, girar: ajusta setpointRPM
 * - Tela 0, pressionar: liga/desliga motor
 * - Qualquer tela, pressionar longo: abre menu de configuração
 * - Tela 3, pressionar: pausa/retoma gráfico
 *
 * @par Menu de Configuração
 * Acessado por pressão longa. Permite editar:
 * Setpoint RPM, Kp/Ki corrente, Kp/Ki RPM, Soft-Start ON/OFF, Autotune.
 *
 * @par Bibliotecas necessárias (platformio.ini)
 * - adafruit/Adafruit SSD1306
 * - adafruit/Adafruit GFX Library
 */

/**
 * @brief Inicializa o display SSD1306 via I²C e exibe a tela de boot.
 * Deve ser chamada no setup() após Serial.begin().
 */
void setupDisplay();

/**
 * @brief Processa o encoder e atualiza o display.
 * Deve ser chamada a cada iteração do loop().
 * Internamente limita o redesenho a DISPLAY_UPDATE_MS (100ms = 10Hz).
 */
void atualizarDisplay();
