/**
 * uTimerLib example
 *
 * @author Naguissa
 * @url https://www.github.com/Naguissa/uTimerLib
 * @url https://www.foroelectro.net
 */

#include "Arduino.h"
#include "uTimerLib.h"

#ifndef LED_BUILTIN
	// change to fit your needs
	#define LED_BUILTIN 13
#endif

volatile bool status = 0;

void timed_function() {
	status = !status;
	if (status) {
		TimerLib.setTimeout_us(timed_function, 1000000);
	}
}


void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	TimerLib.setTimeout_us(timed_function, 1000000);
}


void loop() {
	digitalWrite(LED_BUILTIN, status);
}

