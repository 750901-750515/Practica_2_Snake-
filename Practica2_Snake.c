#include "ripes_system.h"

// Definición de constantes
#define COLOR_SERPIENTE 0xff0000  // Rojo
#define COLOR_MANZANA 0x00ff00   // Verde
#define LONGITUD_MAXIMA 50       // Longitud máxima de la serpiente
#define TAMANO_PIXEL 2           // Tamaño del pixel
#define VELOCIDAD_JUEGO 12000     // Velocidad de juego
#define INICIO_X 10              // Coordenada inicial X de la serpiente
#define INICIO_Y 10              // Coordenada inicial Y de la serpiente

// Tipos de datos
typedef enum { JUEGO_TERMINADO, JUGANDO } Estado;
typedef enum { ARRIBA, ABAJO, IZQUIERDA, DERECHA } Direccion;
typedef enum { FALSO, VERDADERO } Booleano;

// Estructuras
typedef struct {
    unsigned x, y;
} Coordenada;

typedef struct {
    Coordenada posicion;
    Direccion direccion;
} Segmento;

typedef struct {
    Segmento cuerpo[LONGITUD_MAXIMA];
    int longitud;
    Coordenada manzana;
} Serpiente;

// Variables globales
unsigned *leds = (unsigned *)LED_MATRIX_0_BASE;
unsigned *interruptor = (unsigned *)SWITCHES_0_BASE;
unsigned *botonArriba = (unsigned *)D_PAD_0_UP;
unsigned *botonAbajo = (unsigned *)D_PAD_0_DOWN;
unsigned *botonIzquierda = (unsigned *)D_PAD_0_LEFT;
unsigned *botonDerecha = (unsigned *)D_PAD_0_RIGHT;
unsigned semilla = 12345;  // Semilla para RNG

// Prototipos de funciones
void configurar_serpiente(Serpiente *serpiente);
void limpiar_pantalla();
void retrasar();
void dibujar_juego(Serpiente *serpiente);
void generar_manzana(Serpiente *serpiente);
void mover_serpiente(Serpiente *serpiente);
Booleano detectar_colision_pared(Serpiente *serpiente);
Booleano detectar_colision_serpiente(Serpiente *serpiente);
Booleano comer_manzana(Serpiente *serpiente);
void actualizar_direccion(Serpiente *serpiente);
void mover_cabeza(Serpiente *serpiente);
unsigned obtener_numero_aleatorio();
unsigned obtener_aleatorio_mod(unsigned max);
void reiniciar_juego(Serpiente *serpiente);

// Función principal
void main() {
    Serpiente serpiente;
    Estado estado = JUGANDO;
    Booleano reiniciar = FALSO;
    unsigned contador = 0;

    configurar_serpiente(&serpiente);
    limpiar_pantalla();

    while (VERDADERO) {
        while (estado == JUGANDO) {
            limpiar_pantalla();
            if (!(serpiente.manzana.x && serpiente.manzana.y)) 
                generar_manzana(&serpiente);

            dibujar_juego(&serpiente);
            mover_serpiente(&serpiente);
            actualizar_direccion(&serpiente);
            mover_cabeza(&serpiente);

            estado = (detectar_colision_pared(&serpiente) || detectar_colision_serpiente(&serpiente)) ? JUEGO_TERMINADO : JUGANDO;

            if (comer_manzana(&serpiente)) {
                ++serpiente.longitud;
                serpiente.manzana.x = 0;
                serpiente.manzana.y = 0;
            }

            retrasar();
            ++contador;
        }

        reiniciar = (*(interruptor) & 0x01 && estado == JUEGO_TERMINADO) ? VERDADERO : FALSO;

        if (reiniciar) {
            reiniciar_juego(&serpiente);
            estado = JUGANDO;
        }
    }
}

// Funciones auxiliares

// Configura la serpiente inicial
void configurar_serpiente(Serpiente *serpiente) {
    serpiente->longitud = 1;
    serpiente->cuerpo[0].posicion.x = INICIO_X;
    serpiente->cuerpo[0].posicion.y = INICIO_Y;
    serpiente->cuerpo[0].direccion = DERECHA;
    serpiente->manzana.x = 0;
    serpiente->manzana.y = 0;
}

// Limpia todos los LEDs
void limpiar_pantalla() {
    for (int i = 0; i < LED_MATRIX_0_HEIGHT * LED_MATRIX_0_WIDTH; ++i) {
        leds[i] = 0x000000;  // Apagar todos los LEDs
    }
}

// Función de pausa
void retrasar() {
    volatile unsigned pausa = 0;
    for (int i = 0; i < VELOCIDAD_JUEGO; ++i, ++pausa);
}

// Dibuja el estado del juego en la pantalla LED
void dibujar_juego(Serpiente *serpiente) {
    for (int i = 0; i < serpiente->longitud; ++i) {
        for (int x = 0; x < TAMANO_PIXEL; ++x) {
            for (int y = 0; y < TAMANO_PIXEL; ++y) {
                if (i == 0)
                    leds[(serpiente->manzana.y + y) * LED_MATRIX_0_WIDTH + (serpiente->manzana.x + x)] = COLOR_MANZANA;

                leds[(serpiente->cuerpo[i].posicion.y + y) * LED_MATRIX_0_WIDTH + (serpiente->cuerpo[i].posicion.x + x)] = COLOR_SERPIENTE;
            }
        }
    }
}

// Genera una nueva manzana en la pantalla
void generar_manzana(Serpiente *serpiente) {
    int x = obtener_aleatorio_mod(LED_MATRIX_0_WIDTH - TAMANO_PIXEL);
    int y = obtener_aleatorio_mod(LED_MATRIX_0_HEIGHT - TAMANO_PIXEL);

    if (x % 2) --x;
    if (y % 2) --y;

    serpiente->manzana.x = (x < TAMANO_PIXEL) ? TAMANO_PIXEL : x;
    serpiente->manzana.y = (y < TAMANO_PIXEL) ? TAMANO_PIXEL : y;
}

// Mueve la serpiente a la siguiente posición
void mover_serpiente(Serpiente *serpiente) {
    for (int i = serpiente->longitud - 1; i > 0; --i) {
        serpiente->cuerpo[i].posicion = serpiente->cuerpo[i - 1].posicion;
    }
}

// Verifica si la serpiente ha chocado contra una pared
Booleano detectar_colision_pared(Serpiente *serpiente) {
    return (serpiente->cuerpo[0].posicion.x < TAMANO_PIXEL || 
            serpiente->cuerpo[0].posicion.x >= LED_MATRIX_0_WIDTH - TAMANO_PIXEL || 
            serpiente->cuerpo[0].posicion.y < TAMANO_PIXEL || 
            serpiente->cuerpo[0].posicion.y >= LED_MATRIX_0_HEIGHT - TAMANO_PIXEL) ? VERDADERO : FALSO;
}

// Verifica si la serpiente ha chocado consigo misma
Booleano detectar_colision_serpiente(Serpiente *serpiente) {
    for (int i = 1; i < serpiente->longitud; ++i) {
        if (serpiente->cuerpo[0].posicion.x == serpiente->cuerpo[i].posicion.x && 
            serpiente->cuerpo[0].posicion.y == serpiente->cuerpo[i].posicion.y) {
            return VERDADERO;
        }
    }
    return FALSO;
}

// Verifica si la serpiente ha comido la manzana
Booleano comer_manzana(Serpiente *serpiente) {
    return (serpiente->cuerpo[0].posicion.x == serpiente->manzana.x && 
            serpiente->cuerpo[0].posicion.y == serpiente->manzana.y) ? VERDADERO : FALSO;
}

// Actualiza la dirección de la serpiente
void actualizar_direccion(Serpiente *serpiente) {
    if (*botonArriba && serpiente->cuerpo[0].direccion != ABAJO) 
        serpiente->cuerpo[0].direccion = ARRIBA;

    else if (*botonAbajo && serpiente->cuerpo[0].direccion != ARRIBA) 
        serpiente->cuerpo[0].direccion = ABAJO;

    else if (*botonIzquierda && serpiente->cuerpo[0].direccion != DERECHA) 
        serpiente->cuerpo[0].direccion = IZQUIERDA;

    else if (*botonDerecha && serpiente->cuerpo[0].direccion != IZQUIERDA) 
        serpiente->cuerpo[0].direccion = DERECHA;
}

// Mueve la cabeza de la serpiente
void mover_cabeza(Serpiente *serpiente) {
    switch (serpiente->cuerpo[0].direccion) {
        case ARRIBA: serpiente->cuerpo[0].posicion.y -= TAMANO_PIXEL; break;
        case ABAJO: serpiente->cuerpo[0].posicion.y += TAMANO_PIXEL; break;
        case IZQUIERDA: serpiente->cuerpo[0].posicion.x -= TAMANO_PIXEL; break;
        case DERECHA: serpiente->cuerpo[0].posicion.x += TAMANO_PIXEL; break;
    }
}

// Generador de números aleatorios
unsigned obtener_numero_aleatorio() {
    semilla = semilla * 1103515245 + 12345;
    return (semilla / 65536) % 32768;
}

// Devuelve un número aleatorio dentro de un rango específico
unsigned obtener_aleatorio_mod(unsigned max) {
    return obtener_numero_aleatorio() % max;
}

// Reinicia el juego
void reiniciar_juego(Serpiente *serpiente) {
    configurar_serpiente(serpiente);
    limpiar_pantalla();
}
