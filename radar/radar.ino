#define RADAR

#include "radar.c.h"

void setup (void) {
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial.println("=== RADAR ===");
}

void loop (void) {
    int vel = Radar();
    if (vel != 0) {
        Serial.print((vel > 0) ? "->" : "<-");
        Serial.print(' ');
        Serial.println(abs(vel));
    }
}
