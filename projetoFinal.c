#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "lib/buzzer.h"
#include "lib/button.h"
#include "lib/led.h"
#include "lib/matriz_leds.h"
#include "lib/ssd1306.h"
#include "assets/morseCodeLib.h"

// macros
#define BUZZER_PIN 10
#define BTN_A 5
#define BTN_B 6
#define BTN_JOY 22
#define MATRIX_PIN 7
#define LED_G 11
#define LED_R 13
#define DISPLAY_SDA 14
#define DISPLAY_SCL 15
#define CLK_DIV 16.0
#define BUZZER_FREQ 1000
#define DURATION1 100
#define DURATION2 300

// Protótipos
void init();
uint setup_pwm(uint pin);
void button_callback(uint gpio, uint32_t events);
bool debounce(uint32_t *last_time);
uint find_char(char* code);
void handle_leds();
void handle_display(uint ind);
void draw_cursor(uint x0, uint y0, bool set);
void routine_selection();
// Morse para latino
void first_routine(uint ind);
// Latino para Morse
void second_routine();

// Variaveis globais

// Matriz de leds
PIO pio;
uint sm;

// Display
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// PWM dos leds
uint slice_r;
uint slice_g;

// Controle dos botoes
static uint32_t last_time = 0;
static volatile uint32_t last_press = 0;

// Desenhos para a matriz
extern Matriz_leds_config* numeros[];
extern Matriz_leds_config* letras[];

// Estados dos leds
static volatile bool led_r_state = false;
static volatile bool led_g_state = false;

// Codigo morse
char code[10] = "";

// Variaveis para controlar a posicao dos caracteres no display
uint x = 0;
uint y = 0;

typedef enum {
    FIRST_ROUTINE,
    SECOND_ROUTINE
} Routine;

// Variavel para selecionar qual rotina executar
static volatile Routine current_routine = FIRST_ROUTINE;

static volatile bool button_select_routine = false;

// Funcao principal
int main() {
    init();
    uint ind = -1;

    while (true) {
        //draw_cursor(0, 0, true);
        routine_selection();
        if(current_routine == FIRST_ROUTINE) {
            first_routine(ind);
        } else {
            second_routine();
        }
        
        sleep_ms(10);
    }
}

void init() {
    stdio_init_all();
    
    // Inicializando buzzer
    buzzer_init(BUZZER_PIN);

    // Inicializando botoes
    button_init(BTN_A);
    button_init(BTN_B);
    button_init(BTN_JOY);

    // Inicializando leds COM PWM
    slice_r = setup_pwm(LED_R);
    slice_g = setup_pwm(LED_G);

    pwm_set_gpio_level(LED_R, 0);
    pwm_set_gpio_level(LED_G, 0);

    // Inicializando matriz de leds
    pio = pio0;
    sm = configurar_matriz(pio, MATRIX_PIN);
    limpar_matriz(pio, sm);

    // Inicializando display
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);
    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Habilitando interrupcoes
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_RISE, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_RISE, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_JOY, GPIO_IRQ_EDGE_RISE, true, &button_callback);

}

uint setup_pwm(uint pin){
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurando frequencia
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, CLK_DIV);
    pwm_init(slice_num, &config, true);

    return slice_num;
}

// Modificar para verificar 
void button_callback(uint gpio, uint32_t events) {
    if(debounce(&last_time)) {
        last_press = get_absolute_time();
        if (gpio == BTN_A) {
            led_r_state = true;
            strcat(code, ".");
        } else if(gpio == BTN_B) {
            led_g_state = true;
            strcat(code, "-");
        } else {
            // Espaço entre as letras
            x += 8;
        }
    }
}

bool debounce(uint32_t *last_time) {
    uint32_t curr_time = to_us_since_boot(get_absolute_time());

    if (curr_time - *last_time > 200000) {
        *last_time = curr_time;
        return true;
    }
    return false;
}

uint find_char(char* code) {
    if(code[0] == '\0') {
        return -1;
    }
    for(int i = 0; i < 36; i++) {
        if(strcmp(code, morseCode[i]) == 0) {
            code[0] = '\0';
            return i;
        }
    }
}

void handle_leds() {
    if (led_r_state) {
        pwm_set_gpio_level(LED_R, 6559.6); // Duty cycle 10%
        buzz(BUZZER_PIN, BUZZER_FREQ, DURATION1);
        led_r_state = false;
    } else {
        pwm_set_gpio_level(LED_R, 0);
    }

    if (led_g_state) {
        pwm_set_gpio_level(LED_G, 6559.6); // Duty cycle 10%
        buzz(BUZZER_PIN, BUZZER_FREQ, DURATION2);
        led_g_state = false;
    } else {
        pwm_set_gpio_level(LED_G, 0);
    }
}

void handle_display(uint ind) {
    if (ind > 25) {
        ind = ind - 26;
        printf("Imprimindo numero %d\n", ind);
        ssd1306_draw_char(ssd, x, y, ind + 48);
        imprimir_desenho(*numeros[ind], pio, sm);
    } else {
        printf("Imprimindo letra %c\n", ind + 65);
        ssd1306_draw_char(ssd, x, y, ind + 65);
        imprimir_desenho(*letras[ind], pio, sm);
    }
    render_on_display(ssd, &frame_area);
    x += 8;

    if (x >= 128) {
        x = 0;
        y += 8;
    }

    if (y >= 64) {
        y = 0;
        x = 0;
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, &frame_area);
    }
}

/*
void draw_cursor(uint x0, uint y0, bool set) {
    ssd1306_draw_line(ssd, x0, y0 + 8, x0 + 8, y0 + 8, set);
    ssd1306_draw_line(ssd, x0 + 8, y0 + 8, x0 + 4, y0, set);
    ssd1306_draw_line(ssd, x0, y0 + 8, x0 + 4, y0, set);
    render_on_display(ssd, &frame_area);
}
*/
void routine_selection() {
    while(!button_select_routine) {
        ssd1306_draw_string(ssd, 0, 0, "Escolha a rotina:");
        ssd1306_draw_string(ssd, 0, 19, "A Morse p Latino");
        ssd1306_draw_string(ssd, 0, 39, "B Latino p Morse");
        render_on_display(ssd, &frame_area);
    }
}

void first_routine(uint ind){
    absolute_time_t curr_time = get_absolute_time();
    // Verificando se o código morse foi digitado
    if(absolute_time_diff_us(last_press, curr_time) > 1000000) {
        ind = find_char(code);
    }
    handle_leds();
    if (ind != -1) {
        handle_display(ind);
    }
}

void second_routine() {
    // A implementar
}