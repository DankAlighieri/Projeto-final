#include <stdio.h>
#include "pico/stdlib.h"
#include "buzzer.h"

void buzzer_init(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void buzz(uint8_t BUZZER_PIN, uint16_t freq, uint16_t duration) {
    int period = 1000000 / freq;
    int pulse = period / 2;
    int cycles = freq * duration / 1000;
    for (int j = 0; j < cycles; j++) {
        gpio_put(BUZZER_PIN, 1);
        sleep_us(pulse);
        gpio_put(BUZZER_PIN, 0);
        sleep_us(pulse);
    }    
}