# Conecta+ WALL-E — Robô Inteligente para CitizenTech

Evolução do carrinho-robô WALL-E (ESP32 + HC-SR04 + Servo SG90 + Ponte H L298N + TinyML/Edge Impulse) aplicada ao contexto **CitizenTech** do **Conecta+**: apoio à mobilidade, segurança e acessibilidade de pessoas idosas em espaços públicos e cooperativas.

## Integrantes do Grupo
- Debora da Silva Amaral — RM: 550412
- Eduardo Pielich — RM: 99767
- Lívia Namba Seraphim — RM: 97819

## Evolução em Relação ao CP2
- Integração da inferência TinyML (`livns-project-1_inferencing.h`) diretamente na tomada de decisão autônoma do robô.
- Tratamento explícito de falha/ausência de leitura (`sem_leitura`) e baixa confiança da IA com parada preventiva.
- Interface Web (`http://192.168.4.1`) adaptada à identidade **Conecta+** com exibição de diagnóstico da IA e alertas de segurança.

## Hardware e Conexões
- **Microcontrolador:** ESP32 (Wi-Fi AP: `WALL-E` | Senha: `wallE123`)
- **Ponte H L298N:** `IN1=25`, `IN2=26`, `IN3=27`, `IN4=14`
- **Ultrassônico HC-SR04:** `TRIG=5`, `ECHO=18`
- **Servo SG90:** `SERVO_PIN=15`

## Como Executar
1. Instale a biblioteca `.zip` exportada do Edge Impulse (`livns-project-1_inferencing.h`) e `ESP32Servo` na Arduino IDE.
2. Abra `firmware_esp32/conecta_mais_robo_gs/conecta_mais_robo_gs.ino` e faça upload para o ESP32.
3. Conecte-se à rede Wi-Fi `WALL-E` e acesse `http://192.168.4.1`.
4. Use o modo manual, o botão **Classificar (IA)** ou o **Modo Autônomo (IA)** e acompanhe os logs no Serial Monitor (`115200` baud).

## Vídeo Demonstrativo
- Link do vídeo: [https://drive.google.com/file/d/1zaX0Qg_zC2uNCv0Sr7_b083ZU6TfM-sH/view?usp=sharing]
