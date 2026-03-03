#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "display.h"
#include "encoder.h"
#include "grafico.h"
#include "pids.h"
#include "monitor.h"
#include "maquina.h"
#include "controle.h"
#include "autotune.h"
#include "config.h"

// =============================================================================
// APP - DISPLAY: Interface Gráfica SSD1306 128×64
// =============================================================================
//
// ESTADOS DA UI:
//   UI_TELAS   — navegando entre as 5 telas de monitoramento
//   UI_MENU    — menu de configuração (lista de itens)
//   UI_EDICAO  — editando um valor numérico
//
// MOTOR ON/OFF:
//   O motor é controlado por motorLigado. Quando false, setpointRPM é
//   forçado a 0 — o controle PID continua rodando normalmente mas sem
//   setpoint, o que naturalmente desacelera e para o motor de forma suave.
//
// REDRAW:
//   O display só é redesenhado a cada DISPLAY_UPDATE_MS (100ms).
//   Isso libera o loop() para outras tarefas nos outros 90ms.
// =============================================================================

// ─── Instância do display ────────────────────────────────────────────────────
static Adafruit_SSD1306 oled(DISPLAY_W, DISPLAY_H, &Wire, -1);

// ─── Estado geral da UI ──────────────────────────────────────────────────────
enum EstadoUI { UI_TELAS, UI_MENU, UI_EDICAO };
static EstadoUI estadoUI   = UI_TELAS;
static uint8_t  telaAtual  = 0;
static bool     motorLigado = false;
static uint32_t ultimoRedraw = 0;

// ─── Estado do menu ──────────────────────────────────────────────────────────
static const uint8_t NUM_ITENS_MENU = 8;
static const char* MENU_ITENS[NUM_ITENS_MENU] = {
  "Setpoint RPM",
  "Kp Corrente",
  "Ki Corrente",
  "Kp RPM",
  "Ki RPM",
  "Soft-Start",
  "Autotune",
  "<- Voltar"
};
static uint8_t menuIdx    = 0;
static uint8_t menuScroll = 0; // primeiro item visível (3 linhas cabem)

// ─── Estado de edição de valor ───────────────────────────────────────────────
static uint8_t itemEditando = 0;
static float   valorEdicao  = 0.0f;
static float   valorMin     = 0.0f;
static float   valorMax     = 0.0f;
static float   valorPasso   = 1.0f;
static bool    piscarLigado = true;
static uint32_t ultimoPiscar = 0;

// Soft-start: mantemos estado local aqui — editado como 0/1
static bool softStartAtivo = true;

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Mapeia float [inMin, inMax] → int [outMin, outMax]
static int mapFloat(float val, float inMin, float inMax, int outMin, int outMax) {
  if (inMax - inMin == 0) return outMin;
  return (int)(outMin + (val - inMin) * (outMax - outMin) / (inMax - inMin));
}

// Lê ganho do PID de corrente
static float lerGanhoCorrente(uint8_t idx) {
  if (idx == 0) return pidI_Kp;
  if (idx == 1) return pidI_Ki;
  return pidI_Kd;
}

// Lê ganho do PID de RPM
static float lerGanhoRPM(uint8_t idx) {
  if (idx == 0) return pidRPM_Kp;
  if (idx == 1) return pidRPM_Ki;
  return pidRPM_Kd;
}

// ─── Abre edição para o item de menu selecionado ─────────────────────────────
static void abrirEdicao(uint8_t item) {
  itemEditando = item;
  estadoUI     = UI_EDICAO;

  switch (item) {
    case 0: // Setpoint RPM
      valorEdicao = setpointRPM;
      valorMin    = (float)RPM_MINIMO;
      valorMax    = (float)RPM_MAXIMO;
      valorPasso  = 50.0f;
      break;
    case 1: // Kp Corrente
      valorEdicao = lerGanhoCorrente(0);
      valorMin    = 0.0f; valorMax = 200.0f; valorPasso = 0.5f;
      break;
    case 2: // Ki Corrente
      valorEdicao = lerGanhoCorrente(1);
      valorMin    = 0.0f; valorMax = 500.0f; valorPasso = 1.0f;
      break;
    case 3: // Kp RPM
      valorEdicao = lerGanhoRPM(0);
      valorMin    = 0.0f; valorMax = 20.0f; valorPasso = 0.05f;
      break;
    case 4: // Ki RPM
      valorEdicao = lerGanhoRPM(1);
      valorMin    = 0.0f; valorMax = 10.0f; valorPasso = 0.05f;
      break;
    case 5: // Soft-Start
      valorEdicao = softStartAtivo ? 1.0f : 0.0f;
      valorMin    = 0.0f; valorMax = 1.0f; valorPasso = 1.0f;
      break;
    default:
      estadoUI = UI_MENU; // Autotune e Voltar não abrem edição
      break;
  }
}

// ─── Confirma e aplica valor editado ─────────────────────────────────────────
static void confirmarEdicao() {
  switch (itemEditando) {
    case 0:
      setpointRPM = valorEdicao;
      break;
    case 1:
      setGanhosCorrente(valorEdicao, pidI_Ki, pidI_Kd);
      break;
    case 2:
      setGanhosCorrente(pidI_Kp, valorEdicao, pidI_Kd);
      break;
    case 3:
      setGanhosRPM(valorEdicao, pidRPM_Ki, pidRPM_Kd);
      break;
    case 4:
      setGanhosRPM(pidRPM_Kp, valorEdicao, pidRPM_Kd);
      break;
    case 5:
      softStartAtivo = (valorEdicao >= 0.5f);
      break;
  }
  estadoUI = UI_MENU;
}

// ─────────────────────────────────────────────────────────────────────────────
// DESENHO DAS TELAS
// ─────────────────────────────────────────────────────────────────────────────

static void desenharTelaPrincipal() {
  noInterrupts();
  float rpm = inputRPM;
  float sp  = setpointRPM;
  float pwm = outputPWM_Val;
  interrupts();

  // Linha 1: RPM atual e setpoint
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("RPM "));
  oled.setTextSize(2);
  oled.print((int)rpm);
  oled.setTextSize(1);
  oled.print(F("/"));
  oled.print((int)sp);

  // Linha 2: barra de progresso do PWM
  int barW = mapFloat(pwm, 0, MAX_PWM, 0, 124);
  oled.drawRect(0, 18, 128, 8, WHITE);
  oled.fillRect(2, 20, barW, 4, WHITE);

  // Linha 3: estado do motor + indicador SP
  oled.setCursor(0, 30);
  oled.print(F("Motor:"));
  if (motorLigado) {
    oled.fillRoundRect(38, 28, 40, 10, 2, WHITE);
    oled.setTextColor(BLACK);
    oled.setCursor(42, 30);
    oled.print(F("LIGADO"));
    oled.setTextColor(WHITE);
  } else {
    oled.drawRoundRect(38, 28, 46, 10, 2, WHITE);
    oled.setCursor(42, 30);
    oled.print(F("DESL."));
  }
  oled.setCursor(90, 30);
  oled.print(F("SP"));
  oled.print((char)24); // seta cima
  oled.print((char)25); // seta baixo

  // Linha 4: indicador de tela
  oled.setCursor(0, 56);
  oled.print(F("[1/5]PRINCIPAL"));
}

static void desenharTelaMonitorMotor() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("-- MONITOR MOTOR --"));
  oled.drawLine(0, 10, 128, 10, WHITE);

  noInterrupts();
  float corrente = inputCurrent;
  interrupts();

  oled.setCursor(0, 14);
  oled.print(F("Vmot: "));
  oled.print(tensaoMotor, 1);
  oled.print(F("V"));

  oled.setCursor(0, 26);
  oled.print(F("I:    "));
  oled.print(corrente, 1);
  oled.print(F("A"));

  oled.setCursor(0, 38);
  oled.print(F("Pot:  "));
  oled.print(potenciaMotor, 0);
  oled.print(F("W"));

  oled.setCursor(0, 56);
  oled.print(F("[2/5]MTR MOTOR"));
}

static void desenharTelaMonitorFonte() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("-- MONITOR FONTE --"));
  oled.drawLine(0, 10, 128, 10, WHITE);

  oled.setCursor(0, 14);
  oled.print(F("Vfonte: "));
  oled.print(tensaoFonte, 1);
  oled.print(F("V"));

  oled.setCursor(0, 28);
  oled.print(F("Status: "));
  if (fonteLigada) {
    oled.fillRoundRect(50, 26, 40, 10, 2, WHITE);
    oled.setTextColor(BLACK);
    oled.setCursor(54, 28);
    oled.print(F("LIGADA"));
    oled.setTextColor(WHITE);
  } else {
    oled.drawRoundRect(50, 26, 52, 10, 2, WHITE);
    oled.setCursor(54, 28);
    oled.print(F("DESL."));
  }

  oled.setCursor(0, 56);
  oled.print(F("[3/5]MTR FONTE"));
}

static void desenharTelaGrafico() {
  // Área do gráfico: x=0..127, y=0..52  (12px reservados para legenda em baixo)
  const int GRAPH_TOP    = 0;
  const int GRAPH_BOTTOM = 52;
  const int GRAPH_H      = GRAPH_BOTTOM - GRAPH_TOP;

  // Linhas de grade horizontais (3 linhas)
  for (int g = 1; g <= 3; g++) {
    int gy = GRAPH_TOP + (GRAPH_H * g / 4);
    for (int x = 0; x < 128; x += 4)
      oled.drawPixel(x, gy, WHITE);
  }

  // Plota RPM (linha contínua) e SP (linha tracejada)
  for (uint8_t i = 1; i < GRAFICO_AMOSTRAS; i++) {
    // Índice no buffer circular — mais antigo à esquerda, mais novo à direita
    uint8_t ia = (grafIdx + i - 1) % GRAFICO_AMOSTRAS;
    uint8_t ib = (grafIdx + i)     % GRAFICO_AMOSTRAS;

    int x0 = i - 1;
    int x1 = i;

    // RPM — linha sólida
    int y0rpm = mapFloat(grafRPM[ia], grafMin, grafMax, GRAPH_BOTTOM, GRAPH_TOP);
    int y1rpm = mapFloat(grafRPM[ib], grafMin, grafMax, GRAPH_BOTTOM, GRAPH_TOP);
    oled.drawLine(x0, y0rpm, x1, y1rpm, WHITE);

    // Setpoint — pixel a cada 2 colunas (tracejado)
    if (i % 2 == 0) {
      int ysp = mapFloat(grafSP[ib], grafMin, grafMax, GRAPH_BOTTOM, GRAPH_TOP);
      oled.drawPixel(x1, ysp, WHITE);
    }
  }

  // Legenda inferior
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.print(F("RPM"));
  oled.setCursor(28, 56);
  oled.print(F("...SP"));
  oled.setCursor(70, 56);
  if (graficoPausado) {
    oled.print(F("[PAUSADO]"));
  } else {
    oled.print(F("[4/5]GRAF"));
  }
}

static void desenharTelaDiagnostico() {
  noInterrupts();
  float pwm = outputPWM_Val;
  float spI = setpointCurrent;
  interrupts();

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("-- DIAGNOSTICO  --"));
  oled.drawLine(0, 10, 128, 10, WHITE);

  oled.setCursor(0, 14);
  oled.print(F("PWM:  "));
  oled.print((int)pwm);
  oled.print(F("/"));
  oled.print(MAX_PWM);

  // Barra PWM
  int barW = mapFloat(pwm, 0, MAX_PWM, 0, 120);
  oled.drawRect(0, 24, 128, 6, WHITE);
  oled.fillRect(1, 25, barW, 4, WHITE);

  oled.setCursor(0, 34);
  oled.print(F("SP_I: "));
  oled.print(spI, 1);
  oled.print(F("A"));

  noInterrupts();
  float sp = setpointRPM;
  interrupts();
  oled.setCursor(0, 44);
  oled.print(F("SP_RPM: "));
  oled.print((int)sp);

  oled.setCursor(0, 56);
  oled.print(F("[5/5]DIAG"));
}

static void desenharMenu() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("=== CONFIG ==="));
  oled.drawLine(0, 10, 128, 10, WHITE);

  // Mostra 3 itens por vez com scroll
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t idx = menuScroll + i;
    if (idx >= NUM_ITENS_MENU) break;

    int y = 14 + i * 16;
    if (idx == menuIdx) {
      oled.fillRect(0, y - 1, 128, 12, WHITE);
      oled.setTextColor(BLACK);
    }
    oled.setCursor(4, y);
    oled.print(MENU_ITENS[idx]);
    oled.setTextColor(WHITE);
  }

  // Indicador de scroll
  oled.setCursor(118, 56);
  oled.print(menuIdx + 1);
}

static void desenharEdicao() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("["));
  oled.print(MENU_ITENS[itemEditando]);
  oled.print(F("]"));
  oled.drawLine(0, 10, 128, 10, WHITE);

  oled.setCursor(0, 16);
  oled.print(F("Valor:"));

  // Valor pisca a cada 400ms
  if (millis() - ultimoPiscar > 400) {
    piscarLigado = !piscarLigado;
    ultimoPiscar = millis();
  }

  if (piscarLigado) {
    oled.setTextSize(2);
    oled.setCursor(50, 14);
    if (itemEditando == 5) {
      oled.print(valorEdicao >= 0.5f ? F("ON ") : F("OFF"));
    } else {
      oled.print(valorEdicao, itemEditando >= 3 ? 2 : 0);
    }
    oled.setTextSize(1);
  }

  oled.setCursor(0, 40);
  oled.print(F("Min:"));
  oled.print(valorMin, 0);
  oled.setCursor(64, 40);
  oled.print(F("Max:"));
  oled.print(valorMax, 0);

  oled.setCursor(0, 52);
  oled.print(F("(*)Confirma  (L)Cancela"));
}

static void desenharErro(const char* msg) {
  // Pisca fundo branco
  static bool fundo = false;
  static uint32_t t = 0;
  if (millis() - t > 600) { fundo = !fundo; t = millis(); }

  if (fundo) oled.fillScreen(WHITE);
  oled.setTextColor(fundo ? BLACK : WHITE);

  oled.setTextSize(1);
  oled.setCursor(10, 4);
  oled.print(F("!!! ERRO CRITICO !!!"));
  oled.drawLine(0, 14, 128, 14, fundo ? BLACK : WHITE);

  oled.setTextSize(1);
  oled.setCursor(0, 20);
  oled.print(msg);

  oled.setCursor(0, 50);
  oled.print(F("Reset p/ continuar"));
  oled.setTextColor(WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCESSAMENTO DO ENCODER POR ESTADO
// ─────────────────────────────────────────────────────────────────────────────

static void processarEncoderTelas() {
  // Pressão longa → menu config (qualquer tela)
  if (encBotaoLongo) {
    estadoUI = UI_MENU;
    menuIdx  = 0; menuScroll = 0;
    return;
  }

  // Tela do gráfico: pressionar pausa/retoma
  if (telaAtual == 3 && encBotaoCurto) {
    graficoPausado = !graficoPausado;
    return;
  }

  // Tela principal: pressionar = liga/desliga motor
  if (telaAtual == 0 && encBotaoCurto) {
    motorLigado = !motorLigado;
    if (!motorLigado) {
      noInterrupts();
      setpointRPM = 0.0f;
      interrupts();
    }
    return;
  }

  // Tela principal: girar = ajusta setpoint
  if (telaAtual == 0 && encDelta != 0) {
    noInterrupts();
    float sp = setpointRPM;
    interrupts();
    sp += encDelta * 50.0f; // 50 RPM por passo
    sp = constrain(sp, (float)RPM_MINIMO, (float)RPM_MAXIMO);
    noInterrupts();
    setpointRPM = sp;
    interrupts();
    encDelta = 0;
    return;
  }

  // Outras telas: girar = troca de tela (circular)
  if (encDelta != 0) {
    int8_t d = encDelta > 0 ? 1 : -1;
    telaAtual = (telaAtual + 5 + d) % 5;
    encDelta  = 0;
  }
}

static void processarEncoderMenu() {
  if (encBotaoLongo) {
    estadoUI = UI_TELAS;
    return;
  }

  if (encDelta != 0) {
    int8_t d = encDelta > 0 ? 1 : -1;
    menuIdx = (menuIdx + NUM_ITENS_MENU + d) % NUM_ITENS_MENU;
    // Scroll: mantém item selecionado visível
    if (menuIdx < menuScroll)             menuScroll = menuIdx;
    if (menuIdx >= menuScroll + 3)        menuScroll = menuIdx - 2;
    encDelta = 0;
    return;
  }

  if (encBotaoCurto) {
    if (menuIdx == NUM_ITENS_MENU - 1) {
      // "← Voltar"
      estadoUI = UI_TELAS;
    } else if (menuIdx == 6) {
      // Autotune — sai do display temporariamente
      estadoUI = UI_TELAS;
      oled.clearDisplay();
      oled.setCursor(0, 20);
      oled.setTextSize(1);
      oled.print(F("Iniciando Autotune..."));
      oled.display();
      executarAutotuneCorrente();
    } else {
      abrirEdicao(menuIdx);
    }
  }
}

static void processarEncoderEdicao() {
  if (encBotaoLongo) {
    estadoUI = UI_MENU; // Cancela
    return;
  }

  if (encBotaoCurto) {
    confirmarEdicao();
    return;
  }

  if (encDelta != 0) {
    valorEdicao += encDelta * valorPasso;
    valorEdicao  = constrain(valorEdicao, valorMin, valorMax);
    encDelta = 0;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// API PÚBLICA
// ─────────────────────────────────────────────────────────────────────────────

void setupDisplay() {
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDR)) {
    // Display não encontrado — segue sem display (não trava o sistema)
    return;
  }
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
  oled.setCursor(20, 10);
  oled.print(F("FRESADORA STM32"));
  oled.setCursor(20, 24);
  oled.print(F("Inicializando..."));
  oled.setCursor(25, 40);
  oled.print(F("v1.0 STM32F103"));
  oled.display();
}

void atualizarDisplay() {
  // Limita redesenho a DISPLAY_UPDATE_MS
  if (millis() - ultimoRedraw < (uint32_t)DISPLAY_UPDATE_MS) return;
  ultimoRedraw = millis();

  // Processa encoder primeiro
  lerEncoder();
  switch (estadoUI) {
    case UI_TELAS:  processarEncoderTelas();  break;
    case UI_MENU:   processarEncoderMenu();   break;
    case UI_EDICAO: processarEncoderEdicao(); break;
  }

  // Redesenha
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(1);

  switch (estadoUI) {
    case UI_TELAS:
      switch (telaAtual) {
        case 0: desenharTelaPrincipal();     break;
        case 1: desenharTelaMonitorMotor();  break;
        case 2: desenharTelaMonitorFonte();  break;
        case 3: desenharTelaGrafico();       break;
        case 4: desenharTelaDiagnostico();   break;
      }
      break;
    case UI_MENU:   desenharMenu();   break;
    case UI_EDICAO: desenharEdicao(); break;
  }

  oled.display();
}
