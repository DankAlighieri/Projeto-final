#ifndef BUZZER_H
#define BUZZER_H

#define NOTE_QUANTITY 18

void buzzer_init(uint8_t pin);
void buzz(uint8_t BUZZER_PIN, uint16_t freq[NOTE_QUANTITY], uint16_t duration[NOTE_QUANTITY]);

#endif