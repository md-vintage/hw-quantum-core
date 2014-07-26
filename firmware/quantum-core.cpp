#include <Arduino.h>

#define CPU_PIN 11
#define MEM_PIN 10
#define LA1_PIN 5
#define LA5_PIN 6
#define LA15_PIN 9

#define ACTIVE_PIN A3
#define MAX_LA1_PIN A2
#define MAX_LA5_PIN A1
#define MAX_LA15_PIN A0

#define SERIAL_SPEED 115200
#define MAX_NO_DATA  5000


inline float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline void displayPercent(int pin, unsigned char percent) {
	analogWrite(pin, map(percent, 0, 100, 0, 255));
}

inline unsigned displayAverage(int pin, int max_pin, unsigned char avg) {
	if ( avg > 100 ) {
		digitalWrite(max_pin, HIGH);
		avg = 100;
	} else {
		digitalWrite(max_pin, LOW);
	}
	analogWrite(pin, (int) fmap(((float) avg) / 10.0, 0.0, 10.0, 0.0, 255.0));
}


void setup() {
	Serial.begin(SERIAL_SPEED);

	pinMode(ACTIVE_PIN, OUTPUT);
	pinMode(MAX_LA1_PIN, OUTPUT);
	pinMode(MAX_LA5_PIN, OUTPUT);
	pinMode(MAX_LA15_PIN, OUTPUT);

	// Power-up demo
	displayPercent(CPU_PIN, 100);
	delay(300);
	displayPercent(MEM_PIN, 100);
	delay(300);
	displayAverage(LA1_PIN, MAX_LA1_PIN, 110);
	delay(300);
	displayAverage(LA5_PIN, MAX_LA5_PIN, 110);
	delay(300);
	displayAverage(LA15_PIN, MAX_LA15_PIN, 110);
	delay(1500);
	displayAverage(LA1_PIN, MAX_LA15_PIN, 0);
	delay(300);
	displayAverage(LA5_PIN, MAX_LA5_PIN, 0);
	delay(300);
	displayAverage(LA15_PIN, MAX_LA1_PIN, 0);
	delay(300);
	displayPercent(MEM_PIN, 0);
	delay(300);
	displayPercent(CPU_PIN, 0);
}

void loop() {
	static unsigned long last_data_time = 0;

	if ( Serial.available() >= 5 ) {
		displayPercent(CPU_PIN, Serial.read());
		displayPercent(MEM_PIN, Serial.read());
		displayAverage(LA1_PIN, MAX_LA1_PIN, Serial.read());
		displayAverage(LA5_PIN, MAX_LA5_PIN, Serial.read());
		displayAverage(LA15_PIN, MAX_LA15_PIN, Serial.read());

		last_data_time = millis();
		digitalWrite(ACTIVE_PIN, HIGH);
	}

	unsigned long current_time = millis();
	if ( ( current_time >= last_data_time && current_time - last_data_time >= MAX_NO_DATA ) ||
		( current_time < last_data_time && ((unsigned long) -1) - last_data_time + current_time >= MAX_NO_DATA ) ) {
		digitalWrite(ACTIVE_PIN, LOW);
	}

	delay(10);
}