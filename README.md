# Fresadora STM32 — Sistema de Controle de Motor Universal 1800W

Firmware para controle de velocidade e torque de motor universal de 1800W em DC pulsado, desenvolvido para o **STM32F103C8T6 (Blue Pill)** com PlatformIO.

---

## Hardware

| Componente | Modelo | Observação |
|---|---|---|
| MCU | STM32F103C8T6 (Blue Pill) | 72MHz, 64KB Flash |
| Driver gate | IR2110S | VCC 12V / VDD 3.3V compatível |
| MOSFET | IRFP460 | 500V / 20A |
| Diodo roda livre | MUR1560 | Obrigatório — protege o MOSFET |
| Sensor corrente | ACS712-30A | Saída analógica 66mV/A |
| Sensor RPM | Efeito Hall | Anel magnético de 4 pulsos/volta |
| Display | SSD1306 0.96" | I²C — monitoramento e configuração |
| Interface | Encoder KY-040 | Rotativo com botão — navegação dos menus |

### Pinagem

| Pino STM32 | Função | Periférico |
|---|---|---|
| PA8 | PWM de potência | TIM1 CH1 |
| PA1 | Leitura de corrente (ACS712) | ADC1 CH1 |
| PA0 | Sensor Hall (RPM) | Interrupção externa |
| PA2 | Tensão da fonte (divisor) | ADC1 CH2 |
| PA3 | Tensão sobre o MOSFET (divisor) | ADC1 CH3 |
| PB6 | SCL — Display SSD1306 | I²C1 |
| PB7 | SDA — Display SSD1306 | I²C1 |
| PB12 | CLK (A) — Encoder KY-040 | GPIO |
| PB13 | DT  (B) — Encoder KY-040 | GPIO |
| PB14 | SW (botão) — Encoder KY-040 | GPIO |

> **Atenção:** A referência do ADC é **3.3V** (diferente do Arduino que usa 5V). O offset do ACS712 nesta configuração é 1.65V.

> **Divisor de tensão:** R1 = 180kΩ, R2 = 3kΩ → razão 61 → tensão máxima medida 201V. Ajuste `DIVISOR_TENSAO` em `config.h` se usar resistores diferentes.

---

## Arquitetura do Software

```
Projeto_Fresadora/
│
├── platformio.ini              ← Board, framework, bibliotecas externas
│
├── include/
│   └── config.h                ← Todas as constantes, pinos e limites
│
├── src/
│   └── main.cpp                ← Entry point: setup() e loop()
│
└── lib/
    ├── hal/
    │   ├── pwm.h / .cpp        ← TIM1 CH1 (PA8): PWM 16kHz
    │   ├── rpm.h / .cpp        ← PA0: medição de RPM por período entre pulsos
    │   ├── corrente.h / .cpp   ← ADC PA1: leitura ACS712-30A
    │   ├── controle.h / .cpp   ← TIM3 @ 2kHz: loop de controle determinístico
    │   ├── monitor.h / .cpp    ← PA2/PA3: tensão da fonte, motor e potência
    │   └── encoder.h / .cpp    ← KY-040: rotação e botão via polling
    │
    ├── control/
    │   ├── pids.h / .cpp       ← PIDs em cascata (velocidade + corrente)
    │   └── autotune.h / .cpp   ← Sintonia automática via sTune
    │
    └── app/
        ├── maquina.h / .cpp    ← Soft-start, parada de emergência
        ├── grafico.h / .cpp    ← Buffer circular RPM vs Setpoint (~10s)
        └── display.h / .cpp    ← SSD1306: 5 telas + menu de configuração
```

### Camadas

- **HAL** (`pwm`, `rpm`, `corrente`, `controle`, `monitor`, `encoder`) — acesso direto ao hardware. Única camada que toca registradores e pinos. Para portar para outro STM32, apenas esta camada muda.
- **Control** (`pids`, `autotune`) — algoritmos de controle puros. Não conhece pinos nem registradores.
- **App** (`maquina`, `grafico`, `display`) — lógica de aplicação. Orquestra HAL e Control. Gerencia a interface com o usuário.

### Fluxo de Execução

```
TIM3 ISR @ 2kHz — prioridade 0 (máxima) — CONTROLE
    ├── lerCorrente()        ADC PA1
    ├── calcularRPM()        período entre pulsos Hall
    ├── processarCascata()   PID RPM → PID Corrente → PWM
    └── verificações de segurança (sobrecorrente, stall)

Hall ISR (PA0) — prioridade 1
    └── mede período entre pulsos → base do cálculo de RPM

loop() — ~95% do CPU livre — MONITORAMENTO + UI
    ├── executarSoftStart()   rampa gradual de velocidade
    ├── atualizarMonitor()    tensão fonte/motor + potência
    ├── atualizarGrafico()    buffer circular RPM vs SP (~10s)
    └── atualizarDisplay()    encoder + redesenho SSD1306 @ 10Hz
```

### Controle em Cascata

```
Setpoint RPM ──► [ PID RPM (100Hz) ] ──► Setpoint Corrente
                        ↑                         │
                  Sensor Hall                      ▼
                                     [ PID Corrente (2kHz) ] ──► PWM (TIM1)
                                              ↑
                                        Sensor ACS712
```

---

## Interface Gráfica (Display + Encoder)

### Telas

| ID | Tela | Conteúdo | Interação |
|---|---|---|---|
| 0 | **Principal** | RPM atual + setpoint + barra PWM + estado do motor | Girar = ajusta SP · Pressionar = liga/desliga |
| 1 | **Monitor Motor** | Tensão do motor + corrente + potência | Somente leitura |
| 2 | **Monitor Fonte** | Tensão da fonte + status ligada/desligada | Somente leitura |
| 3 | **Gráfico PID** | RPM vs Setpoint — 128 amostras ≈ 10s, escala Y automática | Pressionar = pausa/retoma captura |
| 4 | **Diagnóstico** | PWM bruto + setpoint de corrente interno | Somente leitura |

### Navegação

- **Girar encoder** — avança entre telas (circular 0 → 4 → 0)
- **Pressionar longo** (≥ 800ms) — abre o Menu de Configuração em qualquer tela

### Menu de Configuração

Acessado por pressão longa. Girar navega pelos itens, pressionar seleciona, pressionar longo cancela/volta.

| Item | Faixa | Passo |
|---|---|---|
| Setpoint RPM | 1000 – 10000 RPM | 50 RPM |
| Kp Corrente | 0 – 200 | 0.5 |
| Ki Corrente | 0 – 500 | 1.0 |
| Kp RPM | 0 – 20 | 0.05 |
| Ki RPM | 0 – 10 | 0.05 |
| Soft-Start | ON / OFF | — |
| Autotune | — | inicia o Step Test |

---

## Monitoramento de Tensão e Potência

Dois divisores de tensão resistivos (R1 = 180kΩ, R2 = 3kΩ) com filtro RC anti-aliasing em cada canal:

- **Tensão da fonte** (PA2) — detecta se a fonte está ligada (`tensaoFonte > TENSAO_FONTE_MINIMA`)
- **Tensão sobre o MOSFET** (PA3) — usada apenas internamente no cálculo
- **Tensão do motor** = tensão da fonte − tensão do MOSFET
- **Potência do motor** = tensão do motor × corrente (Watts)

Filtro EMA com α = 0.05 em ambas as leituras. Executado no `loop()` sem impacto no controle.

---

## Configuração

Todos os parâmetros estão centralizados em `include/config.h`:

| Parâmetro | Valor padrão | Descrição |
|---|---|---|
| `MAX_AMPERE` | 20.0 A | Limite de corrente (proteção IRFP460) |
| `MAX_PWM` | 4499 | Resolução do TIM1 (16kHz) |
| `PULSOS_POR_VOLTA` | 4 | Anel magnético do sensor Hall |
| `SOFTSTART_ALVO` | 5000 RPM | Velocidade de trabalho |
| `DIVISOR_TENSAO` | 61.0 | (R1+R2)/R2 — ajustar conforme resistores |
| `TENSAO_FONTE_MINIMA` | 10.0 V | Limiar de detecção de fonte ligada |
| `PID_RPM_KP/KI/KD` | 1.2 / 0.5 / 0.05 | Ganhos malha de velocidade |
| `PID_CORRENTE_KP/KI` | 18.0 / 135.0 | Ganhos malha de corrente (pré-autotune) |
| `GRAFICO_AMOSTRAS` | 128 | Pontos no buffer do gráfico |
| `GRAFICO_INTERVALO_MS` | 78 ms | Intervalo de coleta → janela ≈ 10s |
| `ENC_LONG_PRESS_MS` | 800 ms | Duração mínima para pressão longa |
| `DISPLAY_UPDATE_MS` | 100 ms | Intervalo de redesenho do display (10Hz) |

> Os ganhos de corrente são estimados. Execute o **Autotune** na primeira utilização para calibrar.

---

## Como Compilar e Fazer Upload

### Pré-requisitos

- [VS Code](https://code.visualstudio.com/)
- Extensão [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
- ST-Link V2

### Passos

1. Abrir VS Code → **File → Open Folder** → selecionar a pasta do projeto
2. PlatformIO instala automaticamente o toolchain STM32 e as bibliotecas na primeira abertura
3. Conectar o ST-Link: `SWDIO`, `SWCLK`, `GND`, `3.3V`
4. Clicar em **Upload** (ícone de seta) na barra inferior do PlatformIO

### Upload via Serial (alternativa sem ST-Link)

```ini
; platformio.ini
upload_protocol = serial
```

Colocar `BOOT0 = 1`, resetar a placa, fazer upload, depois `BOOT0 = 0` e resetar novamente.

---

## Autotune

O Autotune calibra os ganhos da malha de corrente automaticamente usando Ziegler-Nichols (biblioteca sTune).

**Via Serial (na inicialização):**
1. Fazer upload do firmware
2. Abrir o Monitor Serial (115200 baud)
3. Dentro de 5 segundos após o boot, pressionar `A`
4. **Afastar-se do eixo** — o motor girará brevemente
5. Aguardar a conclusão — os novos ganhos serão exibidos no Serial
6. Copiar os valores para `config.h` em `PID_CORRENTE_KP/KI/KD`
7. Recompilar e fazer upload

**Via Display:**
- Pressionar longo em qualquer tela → Menu de Configuração → Autotune

O Serial Plotter exibe dois canais durante o teste:
- Canal 1: corrente real (0–20A)
- Canal 2: PWM normalizado (0–20)

---

## Segurança

| Proteção | Condição | Ação |
|---|---|---|
| Sobrecorrente | `inputCurrent > MAX_AMPERE` | Zera PWM + para TIM3 + loop de erro no Serial e Display |
| Stall | PWM > 88% **e** RPM < 100 **e** setpoint > 500 | Idem |
| Soft-Start | Sempre na inicialização (se ativo) | Rampa gradual até `SOFTSTART_ALVO` |

Em caso de erro crítico o sistema trava intencionalmente, o display exibe a causa piscando e o Serial imprime a mensagem a cada 2s. Reset de hardware necessário para retomar.

---

## Bibliotecas Externas

| Biblioteca | Versão | Autor | Função |
|---|---|---|---|
| [QuickPID](https://github.com/Dlloydev/QuickPID) | ^3.1.9 | Dlloydev | Controle PID de alta performance |
| [sTune](https://github.com/Dlloydev/sTune) | ^2.3.0 | Dlloydev | Autotune por Step Test |
| [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) | ^2.5.7 | Adafruit | Driver do display OLED |
| [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) | ^1.11.9 | Adafruit | Primitivas gráficas |

Instaladas automaticamente pelo PlatformIO via `platformio.ini`.

---

## Documentação

A documentação completa da API está na pasta `docs/`. Abrir `docs/index.html` no navegador — não requer servidor.

Inclui referência de todas as funções e variáveis, fluxograma da interface UI e tabelas de configuração de hardware.

---

## Licença

Projeto pessoal — uso livre.
