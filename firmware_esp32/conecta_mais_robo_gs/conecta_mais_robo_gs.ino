#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ===== EDGE IMPULSE =====
#include <livns-project-1_inferencing.h>

#if EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 1
#error "Seu modelo espera mais de 1 eixo; este código coleta só a distância do HC-SR04."
#endif

// ===== REDE WI-FI DO CONECTA+ / WALL-E =====
const char* ssid = "WALL-E";
const char* password = "wallE123";

WebServer server(80);

// ===== HC-SR04 =====
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

// ===== SERVO SG90 =====
const int SERVO_PIN = 15;
Servo sensorServo;

// ===== L298N =====
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 14;

// ===== CONFIGURACOES =====
const float DISTANCIA_OBSTACULO = 25.0;
const float CONFIANCA_MINIMA = 0.60;

bool modoAutomatico = false;
unsigned long ultimoComando = 0;
const unsigned long TEMPO_SEGURANCA = 700;

String ultimoDiagnosticoIA = "IA: aguardando classificação";
String ultimoAlertaSeguranca = "Seguro: nenhum alerta ativo";

String pagina = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Conecta+ | Assistente WALL-E</title>
<style>
body {
  font-family: Arial, sans-serif;
  text-align: center;
  background: #10202b;
  color: #f4f7f9;
  margin: 0;
  padding: 20px;
}
h1 { color: #ffc928; margin-bottom: 4px; }
.sub { color: #9ec5d8; font-size: 14px; margin-top: 0; }
button {
  border: 0;
  border-radius: 16px;
  padding: 18px;
  font-size: 17px;
  font-weight: bold;
  background: #ffc928;
  color: #10202b;
  touch-action: none;
  user-select: none;
  cursor: pointer;
}
.dir {
  width: 95px;
  height: 75px;
  font-size: 28px;
}
.stop { background: #ff5959; color: white; }
.auto { background: #4cd98a; color: #0c2216; }
.tools { margin: 10px 4px; padding: 14px 18px; }
.grid {
  display: grid;
  grid-template-columns: 100px 100px 100px;
  gap: 10px;
  justify-content: center;
  margin: 22px auto;
}
.card {
  margin: 12px auto;
  max-width: 440px;
  padding: 12px 16px;
  background: #1d3546;
  border-radius: 12px;
  text-align: left;
  font-size: 15px;
}
.card strong { color: #ffc928; }
</style>
</head>
<body>
<h1>Conecta+ • WALL-E</h1>
<p class="sub">CitizenTech — Apoio, Mobilidade e Segurança do Idoso</p>

<div class="card"><strong>Modo:</strong> <span id="status">Modo manual</span></div>

<div class="grid">
  <div></div>
  <button class="dir"
    onpointerdown="startMove('R',event)"
    onpointerup="stopMove(event)"
    onpointercancel="stopMove(event)"
    onpointerleave="stopMove(event)">⬆️</button>
  <div></div>

  <button class="dir"
    onpointerdown="startMove('B',event)"
    onpointerup="stopMove(event)"
    onpointercancel="stopMove(event)"
    onpointerleave="stopMove(event)">⬅️</button>

  <button class="dir stop"
    onpointerdown="stopMove(event)">■</button>

  <button class="dir"
    onpointerdown="startMove('F',event)"
    onpointerup="stopMove(event)"
    onpointercancel="stopMove(event)"
    onpointerleave="stopMove(event)">➡️</button>

  <div></div>
  <button class="dir"
    onpointerdown="startMove('L',event)"
    onpointerup="stopMove(event)"
    onpointercancel="stopMove(event)"
    onpointerleave="stopMove(event)">⬇️</button>
  <div></div>
</div>

<button class="tools auto" onclick="toggleAuto()">
  Modo Autônomo (IA)
</button>
<br>
<button class="tools" onclick="testServo()">Testar servo</button>
<button class="tools" onclick="getDistance()">Medir distância</button>
<button class="tools" onclick="classificar()">Classificar (IA)</button>

<div class="card"><strong>Distância:</strong> <span id="distancia">aguardando leitura</span></div>
<div class="card"><strong>TinyML:</strong> <span id="ia">aguardando classificação</span></div>
<div class="card"><strong>Segurança:</strong> <span id="alerta">nenhum alerta ativo</span></div>

<script>
let movimento = null;
let repeticao = null;

function enviar(comando) {
  fetch('/cmd?m=' + comando).catch(() => {});
}

function startMove(comando, event) {
  if (event) event.preventDefault();
  clearInterval(repeticao);
  movimento = comando;
  enviar(comando);
  repeticao = setInterval(() => enviar(comando), 250);
}

function stopMove(event) {
  if (event) event.preventDefault();
  clearInterval(repeticao);
  repeticao = null;
  movimento = null;
  enviar('S');
}

function toggleAuto() {
  clearInterval(repeticao);
  movimento = null;
  fetch('/auto')
    .then(r => r.text())
    .then(t => document.getElementById('status').innerText = t)
    .catch(() => {});
}

function testServo() {
  fetch('/servo').catch(() => {});
}

function getDistance() {
  fetch('/dist')
    .then(r => r.text())
    .then(t => document.getElementById('distancia').innerText = t)
    .catch(() => {});
}

function classificar() {
  document.getElementById('ia').innerText = 'coletando leituras...';
  fetch('/ia')
    .then(r => r.text())
    .then(t => {
      const partes = t.split('||');
      document.getElementById('ia').innerText = partes[0] || t;
      if (partes[1]) {
        document.getElementById('alerta').innerText = partes[1];
      }
    })
    .catch(() => document.getElementById('ia').innerText = 'falha na conexão');
}

window.addEventListener('blur', stopMove);
document.addEventListener('visibilitychange', () => {
  if (document.hidden) stopMove();
});
</script>
</body>
</html>
)rawliteral";

// ===== MOTORES =====
void pararMotores() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void frente() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void re() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void esquerda() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void direita() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ===== SENSOR =====
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracao == 0) return -1.0;

  return duracao * 0.0343 / 2.0;
}

// ===== INFERENCIA (EDGE IMPULSE) =====
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

struct ResultadoIA {
  String rotulo;
  float confianca;
  String resumo;
};

ResultadoIA executarInferencia() {
  ResultadoIA saida = {"sem_leitura", 0.0f, "Erro ao coletar dados"};

  for (size_t i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
    float d = medirDistancia();
    features[i] = (d < 0) ? 400.0f : d;
    delay((uint32_t)EI_CLASSIFIER_INTERVAL_MS);
  }

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  signal.get_data = &raw_feature_get_data;

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR erro = run_classifier(&signal, &result, false);
  if (erro != EI_IMPULSE_OK) {
    saida.rotulo = "erro_ia";
    saida.resumo = "Erro na IA: codigo " + String((int)erro);
    return saida;
  }

  size_t melhor = 0;
  String detalhes = "";
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > result.classification[melhor].value) {
      melhor = i;
    }
    detalhes += String(result.classification[i].label) + " " +
                String(result.classification[i].value * 100, 0) + "%  ";
  }

  saida.rotulo = String(result.classification[melhor].label);
  saida.confianca = result.classification[melhor].value;
  saida.resumo = saida.rotulo + " (" + String(saida.confianca * 100, 0) + "%) | " + detalhes;

  ultimoDiagnosticoIA = saida.resumo;
  Serial.println("IA -> " + saida.resumo);
  return saida;
}

// ===== ACAO AUTONOMA BASEADA NA IA =====
void aplicarDecisaoAutonoma(const ResultadoIA &ia) {
  if (ia.rotulo == "sem_leitura" || ia.rotulo == "erro_ia") {
    pararMotores();
    ultimoAlertaSeguranca = "Falha de leitura/sensor: parada preventiva por segurança";
    return;
  }

  if (ia.confianca < CONFIANCA_MINIMA) {
    pararMotores();
    ultimoAlertaSeguranca = "Baixa confiança da IA: parada cautelar";
    return;
  }

  if (ia.rotulo == "obstaculo_perto") {
    pararMotores();
    delay(150);
    re();
    delay(250);
    pararMotores();

    sensorServo.write(150);
    delay(350);
    float esq = medirDistancia();
    sensorServo.write(30);
    delay(350);
    float dir = medirDistancia();
    sensorServo.write(90);

    if (esq > dir && esq > DISTANCIA_OBSTACULO) {
      esquerda();
      delay(350);
    } else if (dir > DISTANCIA_OBSTACULO) {
      direita();
      delay(350);
    }
    pararMotores();
    ultimoAlertaSeguranca = "Obstáculo próximo detectado: manobra de segurança executada";
  } else if (ia.rotulo == "obstaculo_medio") {
    pararMotores();
    sensorServo.write(135);
    delay(250);
    sensorServo.write(45);
    delay(250);
    sensorServo.write(90);
    ultimoAlertaSeguranca = "Obstáculo em distância média: atenção no trajeto";
  } else {
    // Caminho livre
    direita();
    delay(180);
    pararMotores();
    ultimoAlertaSeguranca = "Trajeto livre: deslocamento assistido seguro";
  }
}

// ===== PAGINA E COMANDOS =====
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", pagina);
}

void handleCommand() {
  if (!server.hasArg("m")) {
    server.send(400, "text/plain", "Comando ausente");
    return;
  }

  String comando = server.arg("m");
  modoAutomatico = false;
  ultimoComando = millis();

  if (comando == "S") {
    pararMotores();
    server.send(200, "text/plain", "Parado");
    return;
  }

  if (comando == "F") frente();
  else if (comando == "B") re();
  else if (comando == "L") esquerda();
  else if (comando == "R") direita();
  else {
    pararMotores();
    server.send(400, "text/plain", "Comando invalido");
    return;
  }

  server.send(200, "text/plain", "OK");
}

void handleAuto() {
  pararMotores();
  modoAutomatico = !modoAutomatico;

  if (modoAutomatico) {
    ultimoComando = millis();
    server.send(200, "text/plain; charset=utf-8", "Modo autônomo (IA) ligado");
  } else {
    server.send(200, "text/plain; charset=utf-8", "Modo manual");
  }
}

void handleServo() {
  modoAutomatico = false;
  pararMotores();

  sensorServo.write(30);
  delay(400);
  sensorServo.write(90);
  delay(400);
  sensorServo.write(150);
  delay(400);
  sensorServo.write(90);

  server.send(200, "text/plain", "Servo testado");
}

void handleIA() {
  modoAutomatico = false;
  pararMotores();
  ResultadoIA ia = executarInferencia();
  aplicarDecisaoAutonoma(ia);
  server.send(200, "text/plain; charset=utf-8", ia.resumo + "||" + ultimoAlertaSeguranca);
}

void handleDistance() {
  float d = medirDistancia();
  if (d < 0) {
    server.send(200, "text/plain; charset=utf-8", "SEM ECO - verifique sensor e fios");
  } else {
    server.send(200, "text/plain", String(d, 1) + " cm");
  }
}

// ===== INICIALIZACAO =====
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pararMotores();
  digitalWrite(TRIG_PIN, LOW);

  sensorServo.setPeriodHertz(50);
  sensorServo.attach(SERVO_PIN, 500, 2400);
  sensorServo.write(90);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/cmd", handleCommand);
  server.on("/auto", handleAuto);
  server.on("/servo", handleServo);
  server.on("/dist", handleDistance);
  server.on("/ia", handleIA);
  server.begin();

  Serial.println();
  Serial.println("=== CONECTA+ WALL-E (GS CITIZENTECH) ===");
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Senha: ");
  Serial.println(password);
  Serial.print("Acesse: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("Motores iniciam parados.");
}

void loop() {
  server.handleClient();

  if (modoAutomatico) {
    ResultadoIA ia = executarInferencia();
    server.handleClient();
    aplicarDecisaoAutonoma(ia);
    server.handleClient();
  } else {
    if (millis() - ultimoComando > TEMPO_SEGURANCA) {
      pararMotores();
    }
  }
}