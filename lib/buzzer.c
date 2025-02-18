#include <stdio.h>
#include "pico/stdlib.h"
#include "buzzer.h"

void buzzer_init(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void buzz(uint8_t BUZZER_PIN, uint16_t freq[NOTE_QUANTITY], uint16_t duration[NOTE_QUANTITY]) {
    for (uint8_t i = 0; i < NOTE_QUANTITY; i++) {
        int period = 1000000 / freq[i];
        int pulse = period / 2;
        int cycles = freq[i] * duration[i] / 1000;

        for (int j = 0; j < cycles; j++) {
            gpio_put(BUZZER_PIN, 1);
            sleep_us(pulse);
            gpio_put(BUZZER_PIN, 0);
            sleep_us(pulse);
        }    
    }
}