/***************************************************************************

    CPU/Memory/LA monitor for UNIX-based OS (firmware)
    Copyright (C) 2016  Devaev Maxim <mdevaev@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/


#include <Arduino.h>
#include <EEPROM.h>

#ifndef DEVICE_NAME
# define DEVICE_NAME "org.liksys.md.hardware.quantum-core"
#endif

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


void commandPrintSerialNumber() {
	char buf[9];
	for (int address = 0; address < 8; address++) {
		buf[address] = EEPROM.read(address);
	}
	buf[8] = '\0';
	Serial.println(buf);
}

void commandSetSerialNumber(const unsigned char *cmd) {
	for (int address = 0; address < 8; address++) {
		EEPROM.write(address, cmd[address + 1]);
	}
}

inline float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void displayPercent(int pin, unsigned char percent) {
	analogWrite(pin, map(percent, 0, 100, 0, 255));
}

void displayAverage(int pin, int max_pin, unsigned char avg) {
	if ( avg > 100 ) {
		digitalWrite(max_pin, HIGH);
		avg = 100;
	} else {
		digitalWrite(max_pin, LOW);
	}
	analogWrite(pin, (int) fmap(((float) avg) / 10.0, 0.0, 10.0, 0.0, 255.0));
}

bool processCommand(const unsigned char *cmd) {
	switch(cmd[0]) {
		case 0:  Serial.println(DEVICE_NAME); return false;
		case 1:  commandPrintSerialNumber(); return false;
		case 2:  commandSetSerialNumber(cmd); return false;
		case 10: displayPercent(CPU_PIN, cmd[1]); return true;
		case 11: displayPercent(MEM_PIN, cmd[1]); return true;
		case 12: displayAverage(LA1_PIN, MAX_LA1_PIN, cmd[1]); return true;
		case 13: displayAverage(LA5_PIN, MAX_LA5_PIN, cmd[1]); return true;
		case 14: displayAverage(LA15_PIN, MAX_LA15_PIN, cmd[1]); return true;
		default: break;
	}
	return false;
}

void demo() {
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

void setup() {
	Serial.begin(SERIAL_SPEED);

	pinMode(ACTIVE_PIN, OUTPUT);
	pinMode(MAX_LA1_PIN, OUTPUT);
	pinMode(MAX_LA5_PIN, OUTPUT);
	pinMode(MAX_LA15_PIN, OUTPUT);

	demo();
}

void loop() {
	static unsigned long last_data_time = 0;
	unsigned char cmd[9];

	if ( Serial.available() >= 9 ) {
		for (int count = 0; count < 9; ++count) {
			cmd[count] = Serial.read();
		}
		if (processCommand(cmd)) {
			last_data_time = millis();
			digitalWrite(ACTIVE_PIN, HIGH);
		}
	}

	unsigned long current_time = millis();
	if ( ( current_time >= last_data_time && current_time - last_data_time >= MAX_NO_DATA ) ||
		( current_time < last_data_time && ((unsigned long) -1) - last_data_time + current_time >= MAX_NO_DATA ) ) {
		digitalWrite(ACTIVE_PIN, LOW);
	}

	delay(10);
}
