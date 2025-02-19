#ifndef BUZZER_H
#define BUZZER_H

void buzzer_init(uint8_t pin);
void buzz(uint8_t BUZZER_PIN, uint16_t freq, uint16_t duration);

#endif