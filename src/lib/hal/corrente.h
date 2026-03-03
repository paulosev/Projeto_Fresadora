#pragma once

// =============================================================================
// HAL - CORRENTE: ADC PA1 — ACS712-30A
// =============================================================================
// Leitura de corrente via sensor ACS712.
// =============================================================================

// Lê o ADC em PA1 e retorna a corrente em Amperes (magnitude, sempre positiva).
// Conversão: 12 bits, Vref = 3.3V, offset = 1.65V, sensibilidade = 66mV/A.
float lerCorrente();
