#include <Arduino.h>

// ==== Pines ====
// Sensores
#define sensor_4to_piso 36
#define sensor_3er_piso 39
#define sensor_2do_piso 34
#define sensor_1er_piso 35

// Botones externos
#define boton_ext_4to_piso 26
#define boton_ext_3er_piso 25
#define boton_ext_2do_piso 33
#define boton_ext_1er_piso 32

// Botones internos
#define boton_int_4to_piso 21
#define boton_int_3er_piso 17
#define boton_int_2do_piso 16
#define boton_int_1er_piso 27

// Siete segmentos
#define S2 19
#define S1 23

// Relés
#define salida_rele_direccion 18
#define salida_rele_arranque 22

// Led de espera
#define led_espera 2

enum STATE {
  S_ENTRE_PISOS,
  S_SUBIENDO,
  S_PAR_SUB,
  S_BAJANDO,
  S_PAR_BAJ,
  S_PARADO,
  S_BAJANDO_AL_PISO_MAS_CERCANO
};

bool b1, b2, b3, b4, bc1, bc2, bc3, bc4, s1, s2, s3, s4;

bool peticiones_internas[5] = {false, false, false, false, false};

bool peticiones_externas[5] = {false, false, false, false, false};

bool *peticiones;

bool espera, espera_anterior;

bool ledOn, lecturaAnterior;

bool _hay_peticiones_internas = false;

bool mov_pet_ext = false;

int pet_actuales = 0;  // 0 externas  1 internas

STATE state = S_ENTRE_PISOS;

int piso_actual = 0;

int parado = 1;
int arrancado = 0;
int motor = parado;

int subiendo = 1;
int bajando = 0;
int direccion = bajando;

ulong runTime;
ulong tiempoEspera;
ulong tEsperaLed;

ulong segundosEspera = 5;
ulong msEsperaLed = 500;

ulong suma_verificacion = 0;
ulong suma_verificacion_anterior = 0;

const char* stateToString(STATE state) {
  switch (state) {
    case S_ENTRE_PISOS: return "S_ENTRE_PISOS";
    case S_SUBIENDO: return "S_SUBIENDO";
    case S_PAR_SUB: return "S_PAR_SUB";
    case S_BAJANDO: return "S_BAJANDO";
    case S_PAR_BAJ: return "S_PAR_BAJ";
    case S_PARADO: return "S_PARADO";
    case S_BAJANDO_AL_PISO_MAS_CERCANO: return "S_BAJANDO_AL_PISO_MAS_CERCANO";
    default: return "ESTADO_DESCONOCIDO";
  }
}

bool hay_peticiones_internas() {
  for (int i = 1; i <= 4; i++) {
    if (peticiones_internas[i]) return true;
  }
  return false;
}

void registro_debug() {
  Serial.print(s1);
  Serial.print(" ");
  Serial.print(s2);
  Serial.print(" ");
  Serial.print(s3);
  Serial.print(" ");
  Serial.print(s4);
  Serial.print(" - ");
  
  Serial.print(b1);
  Serial.print(" ");
  Serial.print(b2);
  Serial.print(" ");
  Serial.print(b3);
  Serial.print(" ");
  Serial.print(b4);
  Serial.print(" - ");
  
  Serial.print(bc1);
  Serial.print(" ");
  Serial.print(bc2);
  Serial.print(" ");
  Serial.print(bc3);
  Serial.print(" ");
  Serial.print(bc4);
  
  Serial.println();

  Serial.print("Piso actual: ");
  Serial.print(piso_actual);

  Serial.println();

  Serial.print("Estado actual: ");
  Serial.print(stateToString(state));

  Serial.println();

  Serial.print("Peticiones internas: [");
  for (int i = 1; i <= 4; i++) {
    if (peticiones_internas[i]) {
      Serial.print(" X ");
    } else {
      Serial.print(" - ");
    }
  }

  Serial.println();

  Serial.print("Peticiones externas: [");
  for (int i = 1; i <= 4; i++) {
    if (peticiones_externas[i]) {
      Serial.print(" X ");
    } else {
      Serial.print(" - ");
    }
  }

  Serial.println();

  Serial.print("Peticiones: [");
  for (int i = 1; i <= 4; i++) {
    if (peticiones[i]) {
      Serial.print(" X ");
    } else {
      Serial.print(" - ");
    }
  }

  Serial.println();

  Serial.print("hay peticiones internas: ");
  Serial.print(_hay_peticiones_internas);

  Serial.println();

  Serial.print("Espera: ");
  Serial.print(espera);

  Serial.println();
}

const char* bitsPiso(int piso) {
  switch (piso) {
      case 1: return "00";
      case 2: return "01";
      case 3: return "11";
      case 4: return "10";
      default: return "22";
  }
}

const char* bitsState(STATE _state) {
  switch (_state) {
      case S_ENTRE_PISOS: return "000";
      case S_BAJANDO_AL_PISO_MAS_CERCANO: return "001";
      case S_PAR_SUB: return "010";
      case S_PAR_BAJ: return "011";
      case S_SUBIENDO: return "100";
      case S_BAJANDO: return "101";
      case S_PARADO: return "110";
      default: return "111";
  }
}

int boolToInt(bool value) {
  if (value) return 1;
  else return 0;
}

void esperaIntermitente() {
  ledOn = digitalRead(led_espera) == HIGH;
  if (ledOn != lecturaAnterior) tEsperaLed = runTime + msEsperaLed;  
  
  if (runTime > tEsperaLed) {
    int proximoEstadoLed = ledOn ? 0 : 1;
    digitalWrite(led_espera, proximoEstadoLed);
  }
  
  lecturaAnterior = digitalRead(led_espera) == HIGH;
}

void setup() {
  pinMode(sensor_4to_piso, INPUT_PULLUP);
  pinMode(sensor_3er_piso, INPUT_PULLUP);
  pinMode(sensor_2do_piso, INPUT_PULLUP);
  pinMode(sensor_1er_piso, INPUT_PULLUP);

// Botones externos
  pinMode(boton_ext_4to_piso, INPUT_PULLUP);
  pinMode(boton_ext_3er_piso, INPUT_PULLUP);
  pinMode(boton_ext_2do_piso, INPUT_PULLUP);
  pinMode(boton_ext_1er_piso, INPUT_PULLUP);

// Botones internos
  pinMode(boton_int_4to_piso, INPUT_PULLUP);
  pinMode(boton_int_3er_piso, INPUT_PULLUP);
  pinMode(boton_int_2do_piso, INPUT_PULLUP);
  pinMode(boton_int_1er_piso, INPUT_PULLUP);

// Siete segmentos
  pinMode(S2, OUTPUT);
  pinMode(S1, OUTPUT);

// Relés
  pinMode(salida_rele_direccion, OUTPUT);
  pinMode(salida_rele_arranque, OUTPUT);

// LED de espera
  pinMode(led_espera, OUTPUT);

  digitalWrite(salida_rele_arranque, HIGH);
  digitalWrite(salida_rele_direccion, HIGH);

  Serial.begin(115200);

}


void loop() {

  runTime = millis();

  _hay_peticiones_internas = hay_peticiones_internas();

  s1 = !digitalRead(sensor_1er_piso); 
  s2 = digitalRead(sensor_2do_piso); 
  s3 = digitalRead(sensor_3er_piso); 
  s4 = !digitalRead(sensor_4to_piso); 

  b1 = digitalRead(boton_ext_1er_piso); 
  b2 = digitalRead(boton_ext_2do_piso); 
  b3 = digitalRead(boton_ext_3er_piso); 
  b4 = digitalRead(boton_ext_4to_piso); 

  bc1 = digitalRead(boton_int_1er_piso); 
  bc2 = digitalRead(boton_int_2do_piso); 
  bc3 = digitalRead(boton_int_3er_piso); 
  bc4 = digitalRead(boton_int_4to_piso); 

  if (!bc1 && s1 && !peticiones_internas[1]) {
    peticiones_internas[1] = true;
  }

  if (!bc2 && s2 && !peticiones_internas[2]) {
    peticiones_internas[2] = true;
  }

  if (!bc3 && s3 && !peticiones_internas[3]) {
    peticiones_internas[3] = true;
  }

  if (!bc4 && s4 && !peticiones_internas[4]) {
    peticiones_internas[4] = true;
  }

  if (!b1 && s1 && !peticiones_externas[1]) {
    peticiones_externas[1] = true;
  }

  if (!b2 && s2 && !peticiones_externas[2]) {
    peticiones_externas[2] = true;
  }

  if (!b3 && s3 && !peticiones_externas[3]) {
    peticiones_externas[3] = true;
  }

  if (!b4 && s4 && !peticiones_externas[4]) {
    peticiones_externas[4] = true;
  }

  // Se priorizan las peticiones internas sobre las externas
  if (_hay_peticiones_internas && !mov_pet_ext) {
    peticiones = peticiones_internas;
    pet_actuales = 1;
  } else {
    peticiones = peticiones_externas;
    pet_actuales = 0;
  }

  // peticiones = peticiones_externas;

  switch (state) {

    case S_ENTRE_PISOS: {
      mov_pet_ext = false;
      if (!s1) {
        piso_actual = 1;
        state = S_PARADO;
      } else if (!s2) {
        piso_actual = 2;
        state = S_PARADO;
      } else if (!s3) {
        piso_actual = 3;
        state = S_PARADO;
      } else if (!s4) {
        piso_actual = 4;
        state = S_PARADO;
      } else {
        state = S_BAJANDO_AL_PISO_MAS_CERCANO;
      }
      break;
    }

    case S_BAJANDO_AL_PISO_MAS_CERCANO: {
      mov_pet_ext = false;
      if (!s3) {
        piso_actual = 3;
        state = S_PARADO;
      } else if (!s2) {
        piso_actual = 2;
        state = S_PARADO;
      } else if (!s1) {
        piso_actual = 1;
        state = S_PARADO;
      }
      break;
    }

    case S_SUBIENDO: {
      if (pet_actuales == 0) {
        mov_pet_ext = true;
      } else {
        mov_pet_ext = false;
      }
      if (!s2 && peticiones[2]) {
        piso_actual = 2;
        espera = true;
        peticiones[2] = false;
        state = S_PAR_SUB;
      } else if (!s3 && peticiones[3]) {
        piso_actual = 3;
        espera = true;
        peticiones[3] = false;
        state = S_PAR_SUB;
      } else if (!s4) {
        piso_actual = 4;
        espera = true;
        peticiones[4] = false;
        state = S_PAR_BAJ;
      }
      break;
    }

    case S_PAR_SUB: {
      mov_pet_ext = false;
      if (!espera) {
        bool peticiones_arriba = false;
        for (int i = piso_actual + 1; i <= 4; i++) {
          if (peticiones[i]) {
            peticiones_arriba = true;
            state = S_SUBIENDO;
            break;
          }
        }
        if (!peticiones_arriba) {
          espera = false;
          state = S_PAR_BAJ;
        }
      }
      break;
    }

    case S_PAR_BAJ: {
      mov_pet_ext = false;
      if (!espera) {
        bool peticiones_abajo = false;
        for (int i = piso_actual - 1; i >= 1; i--) {
          if (peticiones[i]) {
            peticiones_abajo = true;
            state = S_BAJANDO;
            break;
          }
        }
        if (!peticiones_abajo) {
          state = S_PARADO;
        }
      }
      break;
    }

    case S_BAJANDO: {
      if (pet_actuales == 0) {
        mov_pet_ext = true;
      } else {
        mov_pet_ext = false;
      }
      if (!s3 && peticiones[3]) {
        piso_actual = 3;
        espera = true;
        peticiones[3] = false;
        state = S_PAR_BAJ;
      } else if (!s2 && peticiones[2]) {
        piso_actual = 2;
        espera = true;
        peticiones[2] = false;
        state = S_PAR_BAJ;
      } else if (!s1) {
        piso_actual = 1;
        espera = true;
        peticiones[1] = false;
        state = S_PAR_SUB;
      }
      break;
    }

    case S_PARADO: {
      mov_pet_ext = false;
      bool _peticiones_arriba = false;
      bool _peticiones_abajo = false;
      for (int i = piso_actual + 1; i <= 4; i++) {
        if (peticiones[i]) {
          _peticiones_arriba = true;
          state = S_SUBIENDO;
          break;
        }
      }
      if (!_peticiones_arriba) {
        for (int i = piso_actual - 1; i >= 1; i--) {
          if (peticiones[i]) {
            _peticiones_abajo = true;
            state = S_BAJANDO;
            break;
          }
        }
      }
      break;
    }

    default: {
      break;
    }
  }

  if (state == S_SUBIENDO) {
    motor = arrancado;
    direccion = subiendo;
  } else if (state == S_BAJANDO || state == S_BAJANDO_AL_PISO_MAS_CERCANO) {
    motor = arrancado;
    direccion = bajando;
  } else {
    motor = parado;
  }

  digitalWrite(salida_rele_arranque, motor);
  digitalWrite(salida_rele_direccion, direccion);

  if (espera) {
    esperaIntermitente();
  } else {
    digitalWrite(led_espera, LOW);
  }
  
  if (!s1) {
    digitalWrite(S2, 0); digitalWrite(S1, 0);
  }

  if (!s2) {
    digitalWrite(S2, 0); digitalWrite(S1, 1);
  }

  if (!s3) {
    digitalWrite(S2, 1); digitalWrite(S1, 1);
  }

  if (!s4) {
    digitalWrite(S2, 1); digitalWrite(S1, 0);
  }

// disparar el timer
  
  if (espera && !espera_anterior) tiempoEspera = runTime + segundosEspera * 1000;  
  
  if (runTime > tiempoEspera) {
    espera = false;
  }
  
  espera_anterior = espera;

  // DEBUG

  const char* bits_piso_actual = bitsPiso(piso_actual);
  int piso_actual_bit_1 = bits_piso_actual[0] - '0';
  int piso_actual_bit_2 = bits_piso_actual[1] - '0';

  const char* bits_estado_actual = bitsState(state);
  int estado_actual_bit_1 = bits_estado_actual[0] - '0';
  int estado_actual_bit_2 = bits_estado_actual[1] - '0';
  int estado_actual_bit_3 = bits_estado_actual[2] - '0';

  suma_verificacion_anterior = suma_verificacion;
  suma_verificacion = 1 * s1 + 2 * s2 + 4 * s3 + 8 * s4
  + 16 * bc1 + 32 * bc2 + 64 * bc3 + 128 * bc4
  + 256 * b1 + 512 * b2 + 1024 * b3 + 2048 * b4
  + 4096 * piso_actual_bit_1 + 8192 * piso_actual_bit_2
  + 16384 * estado_actual_bit_1 + 32768 * estado_actual_bit_2
  + 65536 * estado_actual_bit_3 + 131072 * boolToInt(espera)
  + 262144 * boolToInt(_hay_peticiones_internas);

  int multiplicador = 262144;

  ulong suma_peticiones = 0;
  for (int i = 1; i <= 4; i++) {
    multiplicador *= 2;
    suma_peticiones += multiplicador * boolToInt(peticiones_externas[i]); 
  }
  for (int i = 1; i <= 4; i++) {
    multiplicador *= 2;
    suma_peticiones += multiplicador * boolToInt(peticiones_internas[i]); 
  }

  suma_verificacion += suma_peticiones;

  if (suma_verificacion != suma_verificacion_anterior) {
    registro_debug();
  }

  delay(100);

}