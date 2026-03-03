#pragma once

// =============================================================================
// APP - MÁQUINA: Lógica de Alto Nível
// =============================================================================
// Soft-start, proteções e gerenciamento de estado da fresadora.
// Esta camada não acessa registradores diretamente.
// =============================================================================

// Executa um passo da rampa de soft-start.
// Deve ser chamada periodicamente no loop().
void executarSoftStart();

// Corte de emergência imediato.
// Zera o PWM, para o TIM3 e trava o sistema em loop de erro no Serial.
// Chamada pela ISR do TIM3 em caso de sobrecorrente ou stall.
void paradaEmergencia(const char* msg);
