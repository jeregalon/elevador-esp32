#include <Arduino.h>

// ==== Pines ====
// Sensores
#define sensor_4to_piso 36 // antes 23
#define sensor_3er_piso 39 // antes 22
#define sensor_2do_piso 34 // antes 19
#define sensor_1er_piso 35 // antes 18

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

// Salidas para marcar piso en el siete segmentos
#define S2 19
#define S1 23

// ==== Estado del elevador ====
enum EstadoElevador { PARADO, SUBIENDO, BAJANDO };
EstadoElevador estado = PARADO;

int piso_actual = 0; // 0 = entre pisos
int destino = 0;
bool solicitudes[5] = {false, false, false, false, false}; // índices 1-4

// ==== Timer de reporte ====
unsigned long ultimoReporte = 0;
const unsigned long intervaloReporte = 1000;

unsigned long ultimoCambioElevador = 0;
const unsigned long intervaloCambioElevador = 1000;
int escrituraSieteSeg = 1;

// ==== Clase antirrebote ====
class Boton {
  int pin;
  bool estado_anterior;
  unsigned long ultimo_cambio;
  const unsigned long debounce_delay = 50;

public:
  Boton(int p) : pin(p), estado_anterior(false), ultimo_cambio(0) {
    pinMode(pin, INPUT_PULLUP);
  }

  bool presionado() {
    bool lectura = digitalRead(pin) == LOW;
    unsigned long ahora = millis();
    if (lectura != estado_anterior && (ahora - ultimo_cambio) > debounce_delay) {
      ultimo_cambio = ahora;
      estado_anterior = lectura;
      if (lectura == true) return true;
    }
    return false;
  }
};

// ==== Botones ====
Boton botones[8] = {
  Boton(boton_ext_1er_piso),
  Boton(boton_ext_2do_piso),
  Boton(boton_ext_3er_piso),
  Boton(boton_ext_4to_piso),
  Boton(boton_int_1er_piso),
  Boton(boton_int_2do_piso),
  Boton(boton_int_3er_piso),
  Boton(boton_int_4to_piso)
};

// ==== Funciones ====
int leerPisoActual() {
  if (digitalRead(sensor_1er_piso) == LOW) return 1;
  if (digitalRead(sensor_2do_piso) == LOW) return 2;
  if (digitalRead(sensor_3er_piso) == LOW) return 3;
  if (digitalRead(sensor_4to_piso) == LOW) return 4;
  return 0; // entre pisos
}

void leerBotones() {
  for (int i = 0; i < 8; i++) {
    if (botones[i].presionado()) {
      int piso = (i % 4) + 1;
      solicitudes[piso] = true;
    }
  }
}

int proximoDestino() {
  if (estado == SUBIENDO) {
    for (int i = piso_actual + 1; i <= 4; i++) {
      if (solicitudes[i]) return i;
    }
  } else if (estado == BAJANDO) {
    for (int i = piso_actual - 1; i >= 1; i--) {
      if (solicitudes[i]) return i;
    }
  }

  // Si no hay solicitudes en la dirección actual, buscar cualquier otra
  for (int i = 1; i <= 4; i++) {
    if (solicitudes[i]) return i;
  }

  return 0;
}

void moverElevador() {
  if (destino > piso_actual) {
    estado = SUBIENDO;
    Serial.println("Subiendo...");
  } else if (destino < piso_actual) {
    estado = BAJANDO;
    Serial.println("Bajando...");
  } else {
    estado = PARADO;
  }
}

void detenerse() {
  solicitudes[piso_actual] = false;
  Serial.print("Llegó al piso ");
  Serial.println(piso_actual);
  estado = PARADO;
}

// Mostrar estado
void reporteEstado() {
  Serial.print("Piso actual: ");
  Serial.print(piso_actual == 0 ? "Entre pisos" : String(piso_actual));
  Serial.print(" | Destino: ");
  Serial.print(destino == 0 ? "Ninguno" : String(destino));
  Serial.print(" | Solicitudes: ");
  for (int i = 1; i <= 4; i++) {
    Serial.print(solicitudes[i] ? "[X]" : "[ ]");
  }
  Serial.println();
}

// ==== Setup y Loop ====
void setup() {
  Serial.begin(9600);
  pinMode(sensor_1er_piso, INPUT);
  pinMode(sensor_2do_piso, INPUT);
  pinMode(sensor_3er_piso, INPUT);
  pinMode(sensor_4to_piso, INPUT);

  pinMode(S2, OUTPUT);
  pinMode(S1, OUTPUT);
  Serial.println("Elevador listo.");
}

void loop() {
  if (millis() - ultimoReporte >= intervaloReporte) {
    reporteEstado();
    ultimoReporte = millis();
  }

  leerBotones();
  int nuevo_piso = leerPisoActual();

  if (nuevo_piso != 0) {
    piso_actual = nuevo_piso;
  }

  if (piso_actual == 0) {
    estado = PARADO;
    return;
  }

  if (estado == PARADO) {
    destino = proximoDestino();
    if (destino != 0 && destino != piso_actual) {
      moverElevador();
    }
  } else {
    if (solicitudes[piso_actual]) {
      detenerse();
      destino = proximoDestino();  // Actualiza para continuar si hay más pisos
    }
  }

  if (millis() - ultimoCambioElevador >= intervaloCambioElevador) {

    ultimoCambioElevador = millis();
  }

      switch (piso_actual) {
      case 1:
        digitalWrite(S2, 0);
        digitalWrite(S1, 0);
        break;
      case 2:
        digitalWrite(S2, 0);
        digitalWrite(S1, 1);
        break;
      case 3:
        digitalWrite(S2, 1);
        digitalWrite(S1, 1);
        break;
      case 4:
        digitalWrite(S2, 1);
        digitalWrite(S1, 0);
        break;
      default:
        digitalWrite(S2, 0);
        digitalWrite(S1, 0);
    }
  
}
