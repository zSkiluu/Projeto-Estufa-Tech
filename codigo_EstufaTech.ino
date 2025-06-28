#include <SimpleModbusSlave.h>
#include <LiquidCrystal.h>
#include <dht11.h>

dht11 DHT11;

const int rs = 12, en = 11, d4 = 5, d5 = 6, d6 = 7, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int MOTOR_LUZ = 4;
const int DHT11PIN = 3;
const int MOTOR_AGUA = 9;  // 2
const int LED_VERDE = 8;
const int LED_AMARELO = 2;  // 9
const int LED_VERMELHO = 10;
const int SENSOR_LUZ = A0;
const int BOTAO_LIGA = A1;
const int BOTAO_DESLIGA = A2;
const int BOTAO_AGUA = A4;
const int BOTAO_MOTOR = A3;
const int SENSOR_HUMIDADE = A5;

int cont = 0;
int spHumidade = 0;
int histerese_agua = 0;
int histerese_luz = 0;
int humidade = 0;
int spLuz = 0;

bool ligado = false;
bool automatico = false;
bool emergencia = false;

bool estadoMotor = LOW;
bool botaoAnteriorMotor = false;
bool scadaMotorAnterior = false;

bool estadoBomba = false;  // Estado inicial: bomba desligada
bool botaoAnteriorBomba = false;
bool scadaEstadoAnterior = false;

enum {
  VAL_LUZ,
  TEMP_VAL,
  HUMID_VAL,
  RELE_LUZ,
  RELE_AGUA,
  AUTOMATICO,
  SPHUMID,
  EMERGENCIA,
  SUBST_AGUA,
  ESTADO_SISTEMA,
  SPLUZ,
  HIST_AGUA,
  HIST_LUZ,
  HOLDING_REGS_SIZE
};

unsigned int holdingRegs[HOLDING_REGS_SIZE];

void setup() {
  modbus_configure(&Serial, 9600, SERIAL_8N1, 1, 0, HOLDING_REGS_SIZE, holdingRegs);
  modbus_update_comms(9600, SERIAL_8N1, 1);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Sistema");
  lcd.setCursor(0, 1);
  lcd.print("Desligado");

  pinMode(MOTOR_LUZ, OUTPUT);
  pinMode(MOTOR_AGUA, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(SENSOR_LUZ, INPUT);

  pinMode(BOTAO_LIGA, INPUT);
  pinMode(BOTAO_DESLIGA, INPUT);
  pinMode(BOTAO_AGUA, INPUT);
  pinMode(BOTAO_MOTOR, INPUT);
  pinMode(SENSOR_HUMIDADE, INPUT);

  // Estados iniciais
  digitalWrite(MOTOR_LUZ, HIGH);   // Desligado
  digitalWrite(MOTOR_AGUA, HIGH);  // Desligado (relé ativo em LOW)

  estadoBomba = false;
  holdingRegs[RELE_LUZ] = estadoMotor;
  holdingRegs[SUBST_AGUA] = estadoBomba;  // SCADA já vê a bomba desligada

  holdingRegs[HIST_LUZ] = 0;
  holdingRegs[HIST_AGUA] = 0;
}

void atualizarLCD(const char* linha1, const char* linha2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1);
  lcd.setCursor(0, 1);
  lcd.print(linha2);
}

void verificarBotoes() {
  if (digitalRead(BOTAO_LIGA) == HIGH)
    ligado = true;
  else if (digitalRead(BOTAO_DESLIGA) == HIGH)
    ligado = false;
  else
    ligado = holdingRegs[ESTADO_SISTEMA];  // Se nenhum botão foi pressionado, segue valor do SCADA

  holdingRegs[ESTADO_SISTEMA] = ligado;
}


void estadoLigado() {
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  spHumidade = holdingRegs[SPHUMID];
  spLuz = holdingRegs[SPLUZ];

  automatico = holdingRegs[AUTOMATICO];
  int humidadeAnalog = analogRead(SENSOR_HUMIDADE);
  humidade = map(humidadeAnalog, 1023, 315, 0, 100);


  DHT11.read(DHT11PIN);

  String mensagem = "Temp:" + String((int)DHT11.temperature) + " Humi:" + String(humidade) + "%";
  int luzAnalog = analogRead(SENSOR_LUZ);
  int luz = map(luzAnalog, 0, 996, 0, 100);

  char linha2[17];
  sprintf(linha2, "Luz:%03d%%-->%04d%", luz, luzAnalog);

  holdingRegs[VAL_LUZ] = luz;
  holdingRegs[HUMID_VAL] = (int)humidade;
  atualizarLCD(mensagem.c_str(), linha2);
  cont = 0;


  holdingRegs[TEMP_VAL] = (int)DHT11.temperature;

  // ==== Controle do MOTOR_LUZ ====
  bool botaoAtualMotor = digitalRead(BOTAO_MOTOR);
  bool scadaMotorAtual = holdingRegs[RELE_LUZ];

  bool botaoAtualAgua = digitalRead(BOTAO_AGUA);
  bool scadaAtual = holdingRegs[SUBST_AGUA];

  histerese_agua = holdingRegs[HIST_AGUA];
  histerese_luz = holdingRegs[HIST_LUZ];

  // ==== Controle do MOTOR_AGUA ====
  if (automatico) {
    // ==== Controle da BOMBA com histerese ====
    if (spHumidade < ((int)humidade - histerese_agua) && estadoBomba) {
      estadoBomba = false;
      digitalWrite(MOTOR_AGUA, HIGH);  // DESLIGA bomba
    } else if (spHumidade > ((int)humidade + histerese_agua) && !estadoBomba) {
      estadoBomba = true;
      digitalWrite(MOTOR_AGUA, LOW);  // LIGA bomba
    }
    holdingRegs[SUBST_AGUA] = estadoBomba;

    // ==== Controle do MOTOR_LUZ com histerese ====
    if (spLuz < ((int)luz - histerese_luz) && estadoMotor) {
      estadoMotor = false;
      digitalWrite(MOTOR_LUZ, HIGH);  // DESLIGA motor
    } else if (spLuz > ((int)luz + histerese_luz) && !estadoMotor) {
      estadoMotor = true;
      digitalWrite(MOTOR_LUZ, LOW);  // LIGA motor
    }
    holdingRegs[RELE_LUZ] = !estadoMotor;


  } else {
    // Controle manual - botão
    if (botaoAtualAgua == HIGH && botaoAnteriorBomba == LOW) {
      estadoBomba = !estadoBomba;
    }
    botaoAnteriorBomba = botaoAtualAgua;

    // SCADA comando manual (somente se mudou)
    if (scadaAtual != scadaEstadoAnterior) {
      estadoBomba = scadaAtual;
    }
    scadaEstadoAnterior = scadaAtual;

    if (botaoAtualMotor == HIGH && botaoAnteriorMotor == LOW) {
      estadoMotor = !estadoMotor;
      digitalWrite(MOTOR_LUZ, estadoMotor);
      holdingRegs[RELE_LUZ] = estadoMotor;
    }
    botaoAnteriorMotor = botaoAtualMotor;

    if (scadaMotorAtual != scadaMotorAnterior) {
      estadoMotor = scadaMotorAtual;
      digitalWrite(MOTOR_LUZ, estadoMotor);
    }
    scadaMotorAnterior = scadaMotorAtual;

    // Aplicar ao hardware e sincronizar
    digitalWrite(MOTOR_AGUA, !estadoBomba);  // LOW = ligado
    if (holdingRegs[SUBST_AGUA] != estadoBomba) {
      holdingRegs[SUBST_AGUA] = estadoBomba;
    }
  }
}

void estadoDesligado() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_AMARELO, LOW);

  digitalWrite(MOTOR_LUZ, HIGH);
  digitalWrite(MOTOR_AGUA, HIGH);

  atualizarLCD("Sistema         ", "Desligado        ");
}

void verificarEmergencia() {
  emergencia = holdingRegs[EMERGENCIA] == HIGH;
}

void estadoEmergencia() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  

  ligado = false;
  holdingRegs[ESTADO_SISTEMA] = ligado;

  digitalWrite(MOTOR_LUZ, HIGH);   // HIGH = desligado
  digitalWrite(MOTOR_AGUA, HIGH);  // HIGH = desligado

  atualizarLCD("Sistema Em      ", "Emergencia       ");

  if (++cont >= 50) {
    digitalWrite(LED_AMARELO, !digitalRead(LED_AMARELO));  // Piscar
    cont = 0;
  }
}


void loop() {
  modbus_update();
  verificarEmergencia();

  if (emergencia) {
    estadoEmergencia();
    return;
  }


  verificarBotoes();

  if (ligado) {
    estadoLigado();
  } else {
    estadoDesligado();
  }
}
