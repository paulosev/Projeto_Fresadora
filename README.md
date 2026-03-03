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

### Pinagem

| Pino STM32 | Função | Periférico |
|---|---|---|
| PA8 | PWM de potência | TIM1 CH1 |
| PA1 | Leitura de corrente (ACS712) | ADC1 CH1 |
| PA0 | Sensor Hall (RPM) | Interrupção externa |

> **Atenção:** A referência do ADC é **3.3V** (diferente do Arduino que usa 5V). O offset do ACS712 nesta configuração é 1.65V.

---

## Arquitetura do Software

```
Projeto_Fresadora_PIO/
│
├── platformio.ini          ← Board, framework, bibliotecas externas
│
├── include/
│   └── config.h            ← Todas as constantes, pinos e limites
│
├── src/
│   └── main.cpp            ← Entry point: setup() e loop()
│
└── lib/                    ← Bibliotecas locais
    ├── pwm.h / .cpp        ← [HAL]     TIM1 CH1 (PA8): PWM 16kHz
    ├── rpm.h / .cpp        ← [HAL]     PA0: medição de RPM por período
    ├── corrente.h / .cpp   ← [HAL]     ADC PA1: leitura ACS712-30A
    ├── controle.h / .cpp   ← [HAL]     TIM3 @ 2kHz: loop de controle
    ├── pids.h / .cpp       ← [CONTROL] PIDs em cascata (velocidade + corrente)
    ├── autotune.h / .cpp   ← [CONTROL] Sintonia automática via sTune
    └── maquina.h / .cpp    ← [APP]     Soft-start, parada de emergência
```

### Camadas

- **HAL** (`pwm`, `rpm`, `corrente`, `controle`) — acesso direto ao hardware. Única camada que toca registradores. Para portar para outro STM32, apenas esta camada muda.
- **Control** (`pids`, `autotune`) — algoritmos de controle puros. Não conhece pinos nem registradores.
- **App** (`maquina`) — lógica de aplicação de alto nível. Orquestra HAL e Control pelo nome.

### Fluxo de execução

```
loop() — tarefas de baixa prioridade (display, botões, serial)
    ↕ nunca bloqueia o controle

TIM3 ISR @ 2kHz — prioridade máxima (NVIC = 0)
    ├── lerCorrente()       ADC PA1
    ├── calcularRPM()       período entre pulsos Hall
    ├── processarCascata()  PID RPM → PID Corrente → PWM
    └── verificações de segurança (sobrecorrente, stall)

Hall ISR (PA0) — prioridade 1
    └── mede período entre pulsos → base do cálculo de RPM
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

## Configuração

Todos os parâmetros estão centralizados em `include/config.h`. Os principais:

| Parâmetro | Valor padrão | Descrição |
|---|---|---|
| `MAX_AMPERE` | 20.0 A | Limite de corrente (proteção IRFP460) |
| `MAX_PWM` | 4499 | Resolução do TIM1 (16kHz) |
| `PULSOS_POR_VOLTA` | 4 | Anel magnético do sensor Hall |
| `SOFTSTART_ALVO` | 5000 RPM | Velocidade de trabalho |
| `PID_RPM_KP/KI/KD` | 1.2 / 0.5 / 0.05 | Ganhos malha de velocidade |
| `PID_CORRENTE_KP/KI` | 18.0 / 135.0 | Ganhos malha de corrente (pré-autotune) |

> Os ganhos de corrente são estimados. Execute o **Autotune** na primeira utilização para calibrar.

---

## Como Compilar e Fazer Upload

### Pré-requisitos

- [VS Code](https://code.visualstudio.com/)
- Extensão [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
- ST-Link V2

### Passos

1. Abrir VS Code → **File → Open Folder** → selecionar `Projeto_Fresadora_PIO/`
2. PlatformIO instala automaticamente o toolchain STM32 e as bibliotecas na primeira abertura
3. Conectar o ST-Link: `SWDIO`, `SWCLK`, `GND`, `3.3V`
4. Clicar em **Upload** (ícone de seta) na barra inferior do PlatformIO

### Upload via Serial (alternativa sem ST-Link)

Alterar em `platformio.ini`:
```ini
upload_protocol = serial
```
Colocar `BOOT0 = 1`, resetar a placa, fazer upload, depois `BOOT0 = 0` e resetar novamente.

---

## Autotune

O Autotune calibra os ganhos da malha de corrente automaticamente usando o método Ziegler-Nichols (biblioteca sTune).

**Procedimento:**

1. Fazer upload do firmware
2. Abrir o Monitor Serial (115200 baud)
3. Dentro de 5 segundos após o boot, pressionar `A`
4. **Afastar-se do eixo** — o motor girará brevemente durante o teste
5. Aguardar a conclusão — os novos ganhos serão exibidos no Serial
6. Copiar os valores de `Kp`, `Ki`, `Kd` para `config.h` em `PID_CORRENTE_KP/KI/KD`
7. Recompilar e fazer upload

O Serial Plotter do PlatformIO exibe dois canais durante o teste:
- Canal 1: corrente real (0–20A)
- Canal 2: PWM normalizado (0–20)

---

## Segurança

| Proteção | Condição | Ação |
|---|---|---|
| Sobrecorrente | `inputCurrent > MAX_AMPERE` | Zera PWM + para TIM3 + loop de erro Serial |
| Stall | PWM > 88% **e** RPM < 100 **e** setpoint > 500 | Idem |
| Soft-Start | Sempre na inicialização | Rampa gradual até `SOFTSTART_ALVO` |

Em caso de erro crítico, o sistema trava intencionalmente e imprime a causa no Serial a cada 2 segundos. Um reset de hardware é necessário para retomar.

---

## Bibliotecas Externas

| Biblioteca | Versão | Autor | Função |
|---|---|---|---|
| [QuickPID](https://github.com/Dlloydev/QuickPID) | ^3.1.9 | Dlloydev | Controle PID de alta performance |
| [sTune](https://github.com/Dlloydev/sTune) | ^2.3.0 | Dlloydev | Autotune por Step Test |

Instaladas automaticamente pelo PlatformIO via `platformio.ini`.

---

## Licença

Projeto pessoal — uso livre.
