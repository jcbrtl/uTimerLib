/**
 * Arduino tiny and cross-device compatible timer library.
 *
 * Timers used by microcontroller
 *	Atmel AVR:	Timer2 (3rd timer)
 *  STM32:		Timer3 (3rd timer)
 *  SAM (Due):  TC3 (Timer1, channel 0)
 *  ESP8266:	OS Timer, one slof of seven available (Software timer provided by Arduino because ESP8266 has only two hardware timers and one is needed by it normal operation)
 *  ESP32:		OS Timer, one slof of software timer.
 *  SAMD21:		Timer 4, CC0 (TC3). See http://ww1.microchip.com/downloads/en/DeviceDoc/40001882A.pdf
 *  SAMD51:		Timer 2 (TC1), 16 bits mode (See http://ww1.microchip.com/downloads/en/DeviceDoc/60001507C.pdf
 *
 * @copyright Naguissa
 * @author Naguissa
 * @email naguissa@foroelectro.net
 * @version 1.1.1
 * @created 2018-01-27
 */
#include "uTimerLib.h"

#ifdef _VARIANT_ARDUINO_STM32_
	uTimerLib *uTimerLib::_instance = NULL;
#endif



/**
 * Constructor
 *
 * Nothing to do here
 */
uTimerLib::uTimerLib() {
	#ifdef _VARIANT_ARDUINO_STM32_
		_instance = this;
		clearTimer();
	#endif
}

/**
 * Attaches a callback function to be executed each us microseconds
 *
 * @param	void*()				cb		Callback function to be called
 * @param	unsigned long int	us		Interval in microseconds
 */
void uTimerLib::setInterval_us(void (* cb)(), unsigned long int us) {
	clearTimer();
	_cb = cb;
	_type = UTIMERLIB_TYPE_INTERVAL;
	_attachInterrupt_us(us);
}


/**
 * Attaches a callback function to be executed once when us microseconds have passed
 *
 * @param	void*()				cb		Callback function to be called
 * @param	unsigned long int	us		Timeout in microseconds
 */
int uTimerLib::setTimeout_us(void (* cb)(), unsigned long int us) {
	clearTimer();
	_cb = cb;
	_type = UTIMERLIB_TYPE_TIMEOUT;
	_attachInterrupt_us(us);
}


/**
 * Attaches a callback function to be executed each s seconds
 *
 * @param	void*()				cb		Callback function to be called
 * @param	unsigned long int	s		Interval in seconds
 */
void uTimerLib::setInterval_s(void (* cb)(), unsigned long int s) {
	clearTimer();
	_cb = cb;
	_type = UTIMERLIB_TYPE_INTERVAL;
	_attachInterrupt_s(s);
}


/**
 * Attaches a callback function to be executed once when s seconds have passed
 *
 * @param	void*()				cb		Callback function to be called
 * @param	unsigned long int	s		Timeout in seconds
 */
int uTimerLib::setTimeout_s(void (* cb)(), unsigned long int s) {
	clearTimer();
	_cb = cb;
	_type = UTIMERLIB_TYPE_TIMEOUT;
	_attachInterrupt_s(s);
}









/**
 * Sets up the timer, calculation variables and interrupts for desired ms microseconds
 *
 * Note: This is device-dependant
 *
 * @param	unsigned long int	us		Desired timing in microseconds
 */
void uTimerLib::_attachInterrupt_us(unsigned long int us) {
	if (us == 0) { // Not valid
		return;
	}
	#ifdef ARDUINO_ARCH_AVR
		unsigned char CSMask = 0;
		// For this notes, we asume 16MHz CPU. We recalculate 'us' if not:
		if (F_CPU != 16000000) {
			us = F_CPU / 16000000 * us;
		}

		cli();

		// AVR, using Timer2. Counts at 8MHz
		/*
		Prescaler: TCCR2B; 3 last bits, CS20, CS21 and CS22

		CS22	CS21	CS20	Freq		Divisor		Base Delay	Overflow delay
		  0		  0		  0		stopped		   -		    -			    -
		  0		  0		  1		16MHz		   1		0.0625us			   16us
		  0		  1		  0		2MHz		   8		   0.5us			  128us
		  0		  1		  1		500KHz		  32		     2us			  512us
		  1		  0		  0		250KHz		  64		     4us			 1024us
		  1		  0		  1		125KHz		 128		     8us			 2048us
		  1		  1		  0		62.5KHz		 256		    16us			 4096us
		  1		  1		  1		15.625KHz	1024		    64us			16384us
		*/
		if (us >= 16384) {
			CSMask = (1<<CS22) | (1<<CS21) | (1<<CS20);
			_overflows = us / 16384;
			_remaining = 256 - ((us % 16384) / 64 + 0.5); // + 0.5 is round for positive numbers
		} else {
			if (us >= 4096) {
				CSMask = (1<<CS22) | (1<<CS21) | (1<<CS20);
				_remaining = 256 - (us / 64 + 0.5); // + 0.5 is round for positive numbers
			} else if (us >= 2048) {
				CSMask = (1<<CS22) | (1<<CS21);
				_remaining = 256 - (us / 16 + 0.5); // + 0.5 is round for positive numbers
			} else if (us >= 1024) {
				CSMask = (1<<CS22) | (1<<CS20);
				_remaining = 256 - (us / 8 + 0.5); // + 0.5 is round for positive numbers
			} else if (us >= 512) {
				CSMask = (1<<CS22);
				_remaining = 256 - (us / 4 + 0.5); // + 0.5 is round for positive numbers
			} else if (us >= 128) {
				CSMask = (1<<CS21) | (1<<CS20);
				_remaining = 256 - (us / 2 + 0.5); // + 0.5 is round for positive numbers
			} else if (us >= 16) {
				CSMask = (1<<CS21);
				_remaining = 256 - us * 2;
			} else {
				CSMask = (1<<CS20);
				_remaining = 256 - (us * 16);
			}
			_overflows = 0;
		}

		__overflows = _overflows;
		__remaining = _remaining;

		ASSR &= ~(1<<AS2); 		// Internal clock
		TCCR2A = (1<<COM2A1);	// Normal operation
		TCCR2B = TCCR2B & ~((1<<CS22) | (1<<CS21) | (1<<CS20)) | CSMask;	// Sets divisor

		// Clean counter in normal operation, load remaining when overflows == 0
		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			TCNT2 = 0;				// Clean timer count
		}
		TIMSK2 |= (1 << TOIE2);		// Enable overflow interruption when 0
    	TIMSK2 &= ~(1 << OCIE2A);	// Disable interrupt on compare match

		sei();
	#endif

	// STM32, all variants - Max us is: uint32 max / CYCLES_PER_MICROSECOND
	#ifdef _VARIANT_ARDUINO_STM32_
		Timer3.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
		__overflows = _overflows = __remaining = _remaining = 0;
		uint16_t timerOverflow = Timer3.setPeriod(us);
		Timer3.setCompare(TIMER_CH1, timerOverflow);
		if (_toInit) {
			_toInit = false;
			Timer3.attachInterrupt(TIMER_CH1, uTimerLib::interrupt);
		}
	    Timer3.refresh();
		Timer3.resume();
	#endif



	// SAM, Arduino Due
	#ifdef ARDUINO_ARCH_SAM
		/*
		Prescalers: MCK/2, MCK/8, MCK/32, MCK/128
		Base frequency: 84MHz

		Available Timers:
		ISR/IRQ	 TC	  Channel	Due pins
		  TC0	TC0		0		2, 13
		  TC1	TC0		1		60, 61
		  TC2	TC0		2		58
		  TC3	TC1		0		none
		  TC4	TC1		1		none
		  TC5	TC1		2		none
		  TC6	TC2		0		4, 5
		  TC7	TC2		1		3, 10
		  TC8	TC2		2		11, 12

		We will use TC1, as it has no associated pins. We will choose Channel 0, so ISR is TC3

		REMEMBER! 32 bit counter!!!


				Name				Prescaler	Freq	Base Delay		Overflow delay
		TC_CMR_TCCLKS_TIMER_CLOCK1	  2		    42MHz   0,023809524us	102261126,913327104us, 102,261126913327104s
		TC_CMR_TCCLKS_TIMER_CLOCK2	  8		  10.5MHz	0,095238095us	409044503,35834112us, 409,04450335834112s
		TC_CMR_TCCLKS_TIMER_CLOCK3	 32		 2.625MHz	0,380952381us	1636178017,523809524us, 1636,178017523809524s
		TC_CMR_TCCLKS_TIMER_CLOCK4	128		656.25KHz	1,523809524us	6544712070,913327104us, 6544,712070913327104s

		For simplify things, we'll use always TC_CMR_TCCLKS_TIMER_CLOCK32,as has enougth resolution for us.
		*/

		if (us > 1636178017) {
			__overflows = _overflows = us / 1636178017.523809524;
			__remaining = _remaining = (us - (1636178017.523809524 * _overflows)) / 0.380952381 + 0.5; // +0.5 is same as round
		} else {
			__overflows = _overflows = 0;
			__remaining = _remaining = (us / 0.380952381 + 0.5); // +0.5 is same as round
		}
		pmc_set_writeprotect(false); // Enable write
		pmc_enable_periph_clk(ID_TC3); // Enable TC1 - channel 0 peripheral
		TC_Configure(TC1, 0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK3); // Configure clock; prescaler = 32

		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			TC_SetRC(TC1, 0, 4294967295); // Int on last number
		}

		TC_Start(TC1, 0);
		TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;
		TC1->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS;
		NVIC_EnableIRQ(TC3_IRQn);
	#endif



	#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
		unsigned long int ms = (us / 1000) + 0.5;
		if (ms == 0) {
			ms = 1;
		}
		__overflows = _overflows = __remaining = _remaining = 0;
		_ticker.attach_ms(ms, uTimerLib::interrupt);
	#endif


	// SAMD21, Arduino Zero
	#ifdef _SAMD21_
		/*
		16 bit timer

		Prescaler:
		Prescalers: GCLK_TC, GCLK_TC/2, GCLK_TC/4, GCLK_TC/8, GCLK_TC/16, GCLK_TC/64, GCLK_TC/256, GCLK_TC/1024
		Base frequency: 84MHz

		We will use TCC2, as there're some models with only 3 timers (regular models have 5 TCs)

		REMEMBER! 16 bit counter!!!


		Name			Prescaler	Freq		Base Delay		Overflow delay
		GCLK_TC			   1		 48MHz		0,020833333us	   1365,333333333us;    1,365333333333ms
		GCLK_TC/2		   2		 24MHz		0,041666667us	   2730,666666667us;    2,730666666667ms
		GCLK_TC/4		   4		 12MHz		0,083333333us	   5461,333333333us;    5,461333333333ms
		GCLK_TC/8		   8		  6MHz		0,166666667us	  10922,666666667us;   10,922666666667ms
		GCLK_TC/16		  16		  3MHz		0,333333333us	  21845,333333333us;   21,845333333333ms
		GCLK_TC/64		  64		750KHz		1,333333333us	  87381,333311488us;   87,381333311488ms
		GCLK_TC/256		 256		187,5KHz	5,333333333us	 349525,333311488us;  349,525333311488ms
		GCLK_TC/1024	1024		46.875Hz	21,333333333us	1398101,333333333us; 1398,101333333333ms; 1,398101333333333s

		Will be using:
			GCLK_TC/16 for us
			GCLK_TC/1024 for s
		*/

		// Enable clock for TC
		REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC2_TC3)) ;
		while (GCLK->STATUS.bit.SYNCBUSY == 1); // sync

		// Disable TC
		_TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while (_TC->STATUS.bit.SYNCBUSY == 1); // sync

		// Set Timer counter Mode to 16 bits + Set TC as normal Normal Frq + Prescaler: GCLK_TC/16
		_TC->CTRLA.reg |= (TC_CTRLA_MODE_COUNT16 + TC_CTRLA_WAVEGEN_NFRQ + TC_CTRLA_PRESCALER_DIV16);
		while (_TC->STATUS.bit.SYNCBUSY == 1); // sync



		if (us > 21845) {
			__overflows = _overflows = us / 21845.333333333;
			__remaining = _remaining = ((us - (21845.333333333 * _overflows)) / 0.333333333) + 0.5; // +0.5 is same as round
		} else {
			__overflows = _overflows = 0;
			__remaining = _remaining = (us / 0.333333333) + 0.5; // +0.5 is same as round
		}

		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			_TC->CC[0].reg = 65535;
			_TC->INTENSET.reg = 0;              // disable all interrupts
			_TC->INTENSET.bit.OVF = 1;          // enable overfollow
			// Skip: while (_TC->STATUS.bit.SYNCBUSY == 1); // sync
		}

		_TC->COUNT.reg = 0;              // Reset to 0

		NVIC_EnableIRQ(TC3_IRQn);

		// Enable TC
		_TC->CTRLA.reg |= TC_CTRLA_ENABLE;
	#endif



	#ifdef __SAMD51__
		/*
		16 bit timer

		Prescaler:
		Prescalers: GCLK_TC, GCLK_TC/2, GCLK_TC/4, GCLK_TC/8, GCLK_TC/16, GCLK_TC/64, GCLK_TC/256, GCLK_TC/1024
		Base frequency: 84MHz

		We will use TC1

		REMEMBER! 16 bit counter!!!

		Name			Prescaler	Freq		Base Delay		Overflow delay
		GCLK_TC			   1		 120MHz		0,008333333us	    546,133333333us;    0,546133333333ms
		GCLK_TC/2		   2		  60MHz		0,016666667us	   1092,266666667us;    1,092266666667ms
		GCLK_TC/4		   4		  30MHz		0,033333333us	   2184,533333333us;    2,184533333333ms
		GCLK_TC/8		   8		  20MHz		0,066666667us	   4369,066666667us;    4,369066666667ms
		GCLK_TC/16		  16		  10MHz		0,133333333us	   8738,133333333us;    8,738133333333ms
		GCLK_TC/64		  64		1,875MHz	0,533333333us	  34952,533333333us;   34,952533333333ms
		GCLK_TC/256		 256		468,75KHz	2,133333333us	 139810,133333333us;  139,810133333333ms
		GCLK_TC/1024	1024		117,1875Hz	8,533333333us	 559240,533333333us;  559,240533333333ms

		Will be using:
			GCLK_TC/16 for us
			GCLK_TC/1024 for s
		*/

		MCLK->APBAMASK.bit.TC1_ = 1;
		GCLK->PCHCTRL[TC1_GCLK_ID].bit.GEN = 0;
		GCLK->PCHCTRL[TC1_GCLK_ID].bit.CHEN = 1;
		while(GCLK->PCHCTRL[TC1_GCLK_ID].bit.CHEN != 1); // sync

		TC1->COUNT16.CTRLA.bit.ENABLE = 0;
		while (TC1->COUNT16.SYNCBUSY.reg); // sync

		TC1->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
		TC1->COUNT16.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV16_Val;
		while (TC1->COUNT16.SYNCBUSY.reg); // sync


		if (us > 8738) {
			__overflows = _overflows = us / 8738.133333333;
			__remaining = _remaining = (us - (8738.133333333 * _overflows)) / 0.133333333 + 0.5; // +0.5 is same as round
		} else {
			__overflows = _overflows = 0;
			__remaining = _remaining = (us / 0.133333333 + 0.5); // +0.5 is same as round
		}

		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			TC1->COUNT16.CC[1].reg = 0xFFFF;
			// Skip: while (TC1->COUNT16.SYNCBUSY.reg); // sync
		}


		TC1->COUNT16.INTENSET.bit.OVF = 1;
		// Enable InterruptVector
		NVIC_EnableIRQ(TC1_IRQn);

		// Count on event
		TC1->COUNT16.EVCTRL.bit.EVACT = TC_EVCTRL_EVACT_COUNT_Val;
		// This works but should be after EVSYS setup according to the data sheet
		//TC1->COUNT16.EVCTRL.bit.TCEI = 1;
		//TC1->COUNT16.CTRLA.bit.ENABLE = 1;
	#endif


}


/**
 * Sets up the timer, calculation variables and interrupts for desired s seconds
 *
 * Note: This is device-dependant
 *
 * @param	unsigned long int	s		Desired timing in seconds
 */
void uTimerLib::_attachInterrupt_s(unsigned long int s) {
	if (s == 0) { // Not valid
		return;
	}
	// Arduino AVR
	#ifdef ARDUINO_ARCH_AVR
		unsigned char CSMask = 0;
		// For this notes, we asume 16MHz CPU. We recalculate 's' if not:
		if (F_CPU != 16000000) {
			s = F_CPU / 16000000 * s;
		}

		cli();

		/*
		Using longest mode from _ms function
		CS22	CS21	CS20	Freq		Divisor		Base Delay	Overflow delay
		  1		  1		  1		15.625KHz	1024		    64us			16384us
		*/
		CSMask = (1<<CS22) | (1<<CS21) | (1<<CS20);
		if (_overflows > 500000) {
			_overflows = s / 16384 * 1000000;
		} else {
			_overflows = s * 1000000 / 16384;
		}
		// Original: _remaining = 256 - round(((us * 1000000) % 16384) / 64);
		// Anti-Overflow trick:
		if (s > 16384) {
			unsigned long int temp = floor(s / 16384) * 16384;
			_remaining = 256 - ((((s - temp) * 1000000) % 16384) / 64 + 0.5); // + 0.5 is round for positive numbers
		} else {
			_remaining = 256 - (((s * 1000000) % 16384) / 64 + 0.5); // + 0.5 is round for positive numbers
		}

		__overflows = _overflows;
		__remaining = _remaining;

		ASSR &= ~(1<<AS2); 		// Internal clock
		TCCR2A = (1<<COM2A1);	// Normal operation
		TCCR2B = TCCR2B & ~((1<<CS22) | (1<<CS21) | (1<<CS20)) | CSMask;	// Sets divisor

		// Clean counter in normal operation, load remaining when overflows == 0
		if (__overflows == 0) {
			_loadRemaining();
		} else {
			TCNT2 = 0;				// Clean timer count
		}
		TIMSK2 |= (1 << TOIE2);		// Enable overflow interruption when 0
    	TIMSK2 &= ~(1 << OCIE2A);	// Disable interrupt on compare match

		sei();
	#endif


	// STM32, all variants
	#ifdef _VARIANT_ARDUINO_STM32_
		Timer3.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
		Timer3.setPeriod((unsigned long int) 1000000);			// 1s, in microseconds
		Timer3.setCompare(TIMER_CH1, 1000000);
		__overflows = _overflows = s;
		__remaining = _remaining = 0;
		if (_toInit) {
			_toInit = false;
			Timer3.attachInterrupt(TIMER_CH1, uTimerLib::interrupt);
		}
	    Timer3.refresh();
		Timer3.resume();
	#endif


	// SAM, Arduino Due
	#ifdef ARDUINO_ARCH_SAM
		/*

		See _ms functions for detailed info; here only selected points

				Name				Prescaler	Freq	Base Delay		Overflow delay
		TC_CMR_TCCLKS_TIMER_CLOCK4	128		656.25KHz	1,523809524us	6544712070,913327104us, 6544,712070913327104s

		For simplify things, we'll use always TC_CMR_TCCLKS_TIMER_CLOCK32,as has enougth resolution for us.
		*/
		if (s > 6544) {
			__overflows = _overflows = s / 6544.712070913327104;
			__remaining = _remaining = (s - (6544.712070913327104 * _overflows) / 0.000001523809524 + 0.5); // +0.5 is same as round
		} else {
			__overflows = _overflows = 0;
			__remaining = _remaining = (s / 0.000001523809524 + 0.5); // +0.5 is same as round
		}

		pmc_set_writeprotect(false); // Enable write
		//pmc_enable_periph_clk((uint32_t) TC3_IRQn); // Enable TC1 - channel 0 peripheral
		pmc_enable_periph_clk(ID_TC3); // Enable TC1 - channel 0 peripheral
		TC_Configure(TC1, 0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4); // Configure clock; prescaler = 128

		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			TC_SetRC(TC1, 0, 4294967295); // Int on last number
		}

		TC1->TC_CHANNEL[0].TC_IER=TC_IER_CPCS;
		TC1->TC_CHANNEL[0].TC_IDR=~TC_IER_CPCS;
		NVIC_EnableIRQ(TC3_IRQn);
		TC_Start(TC1, 0);
	#endif

	#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
		__overflows = _overflows = __remaining = _remaining = 0;
		_ticker.attach(s, uTimerLib::interrupt);
	#endif


	// SAMD21, Arduino Zero
	#ifdef _SAMD21_
		/*
		GCLK_TC/1024	1024		46.875Hz	21,333333333us	1398101,333333333us; 1398,101333333333ms; 1,398101333333333s
		*/

		// Enable clock for TC
		REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC2_TC3)) ;
		while (GCLK->STATUS.bit.SYNCBUSY == 1); // sync

		// Disable TC
		_TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while (_TC->STATUS.bit.SYNCBUSY == 1); // sync

		// Set Timer counter Mode to 16 bits + Set TC as normal Normal Frq + Prescaler: GCLK_TC/1024
		_TC->CTRLA.reg |= (TC_CTRLA_MODE_COUNT16 + TC_CTRLA_WAVEGEN_NFRQ + TC_CTRLA_PRESCALER_DIV1024);
		while (_TC->STATUS.bit.SYNCBUSY == 1); // sync

		if (s > 1) {
			__overflows = _overflows = s / 1.398101333333333;
			__remaining = _remaining = ((s - (1.398101333333333 * _overflows)) / 0.000021333333333) + 0.5; // +0.5 is same as round
		} else {
			__overflows = _overflows = 0;
			__remaining = _remaining = (s / 0.000021333333333) + 0.5; // +0.5 is same as round
		}

		if (__overflows == 0) {
			_loadRemaining();
			_remaining = 0;
		} else {
			_TC->CC[0].reg = 65535;
			_TC->INTENSET.reg = 0;              // disable all interrupts
			_TC->INTENSET.bit.OVF = 1;          // enable overfollow
			// Skip: while (_TC->STATUS.bit.SYNCBUSY == 1); // sync
		}

		_TC->COUNT.reg = 0;              // Reset to 0

		NVIC_EnableIRQ(TC3_IRQn);

		// Enable TC
		_TC->CTRLA.reg |= TC_CTRLA_ENABLE;
	#endif


	#ifdef __SAMD51__
		/*
		GCLK_TC/1024	1024		117,1875Hz	8,533333333us	 559240,533333333us;  559,240533333333ms
		*/

		MCLK->APBAMASK.bit.TC1_ = 1;
		GCLK->PCHCTRL[TC1_GCLK_ID].bit.GEN = 0;
		GCLK->PCHCTRL[TC1_GCLK_ID].bit.CHEN = 1;
		while(GCLK->PCHCTRL[TC1_GCLK_ID].bit.CHEN != 1); // sync

		TC1->COUNT16.CTRLA.bit.ENABLE = 0;
		while (TC1->COUNT16.SYNCBUSY.reg); // sync

		TC1->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
		TC1->COUNT16.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV16_Val;
		while (TC1->COUNT16.SYNCBUSY.reg); // sync


		__overflows = _overflows = s / 0.559240533333333;
		__remaining = _remaining = (s - (0.559240533333333 * _overflows)) / 0.000008533333333 + 0.5; // +0.5 is same as round

		TC1->COUNT16.CC[1].reg = 0xFFFF;
		// Skip: while (TC1->COUNT16.SYNCBUSY.reg); // sync

		TC1->COUNT16.INTENSET.bit.OVF = 1;
		// Enable InterruptVector
		NVIC_EnableIRQ(TC1_IRQn);

		// Count on event
		TC1->COUNT16.EVCTRL.bit.EVACT = TC_EVCTRL_EVACT_COUNT_Val;
		// This works but should be after EVSYS setup according to the data sheet
		//TC1->COUNT16.EVCTRL.bit.TCEI = 1;
		//TC1->COUNT16.CTRLA.bit.ENABLE = 1;
	#endif


}



/**
 * Loads last bit of time needed to precisely count until desired time (non complete loop)
 *
 * Note: This is device-dependant
 */
void uTimerLib::_loadRemaining() {
	#ifdef ARDUINO_ARCH_AVR
		TCNT2 = _remaining;
	#endif

	// STM32: Not needed

	// SAM, Arduino Due
	#ifdef ARDUINO_ARCH_SAM
		TC_SetRC(TC1, 0, _remaining);
	#endif

	// ESP8266
	// ESP32

	// SAMD21, Arduino Zero
	#ifdef _SAMD21_
		_TC->COUNT.reg = 0;              // Reset to 0
		_TC->CC[0].reg = _remaining;
		_TC->INTENSET.reg = 0;              // disable all interrupts
		_TC->INTENSET.bit.MC0 = 1;          // enable compare match to CC0
		while (_TC->STATUS.bit.SYNCBUSY == 1); // sync
	#endif

	#ifdef __SAMD51__
		TC1->COUNT16.CC[1].reg = _remaining;
		while (TC1->COUNT16.SYNCBUSY.reg); // sync
	#endif

}

/**
 * Clear timer interrupts
 *
 * Note: This is device-dependant
 */
void uTimerLib::clearTimer() {
	_type = UTIMERLIB_TYPE_OFF;

	#ifdef ARDUINO_ARCH_AVR
		TIMSK2 &= ~(1 << TOIE2);		// Disable overflow interruption when 0
		SREG = (SREG & 0b01111111); // Disable interrupts without modifiying other interrupts
	#endif

	#ifdef _VARIANT_ARDUINO_STM32_
		Timer3.pause();
	#endif

	// SAM, Arduino Due
	#ifdef ARDUINO_ARCH_SAM
        NVIC_DisableIRQ(TC3_IRQn);
	#endif

	#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
		_ticker.detach();
	#endif

	// SAMD21, Arduino Zero
	#ifdef _SAMD21_
		// Disable TC
		_TC->INTENSET.reg = 0;              // disable all interrupts
		_TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
	#endif

	// SAMD51
	#ifdef __SAMD51__
		TC1->COUNT16.INTENSET.bit.OVF = 1;
		// Disable InterruptVector
		NVIC_DisableIRQ(TC1_IRQn);
	#endif

}

/**
 * Internal intermediate function to control timer interrupts
 *
 * As timers doesn't give us enougth flexibility for large timings,
 * this function implements oferflow control to offer user desired timings.
 */
void uTimerLib::_interrupt() {
	if (_type == UTIMERLIB_TYPE_OFF) { // Should not happen
		return;
	}
	#ifdef _VARIANT_ARDUINO_STM32_
		_cb();
	#else
		if (_overflows > 0) {
			_overflows--;
		}
		if (_overflows == 0 && _remaining > 0) {
				// Load remaining count to counter
				_loadRemaining();
				// And clear remaining count
				_remaining = 0;
		} else if (_overflows == 0 && _remaining == 0) {
			if (_type == UTIMERLIB_TYPE_TIMEOUT) {
				clearTimer();
			} else if (_type == UTIMERLIB_TYPE_INTERVAL) {
				if (__overflows == 0) {
					_remaining = __remaining;
					_loadRemaining();
					_remaining = 0;
				} else {
					_overflows = __overflows;
					_remaining = __remaining;

					// SAM, Arduino Due
					#ifdef ARDUINO_ARCH_SAM
						TC_SetRC(TC1, 0, 4294967295);
					#endif

					// SAMD21, Arduino Zero
					#ifdef _SAMD21_
						_TC->COUNT.reg = 0;              // Reset to 0
						_TC->INTENSET.reg = 0;              // disable all interrupts
						_TC->INTENSET.bit.OVF = 0;          // enable overfollow
						_TC->INTENSET.bit.MC0 = 1;          // disable compare match to CC0
						_TC->CC[0].reg = 65535;
					#endif


					// SAMD51
					#ifdef __SAMD51__
						TC1->COUNT16.CC[1].reg = 0xFFFF;
					#endif
				}
			}
			_cb();
		}
		// SAM, Arduino Due
		#ifdef ARDUINO_ARCH_SAM
			// Reload for SAM
			else if (_overflows > 0) {
				TC_SetRC(TC1, 0, 4294967295);
			}
		#endif

		// SAMD21, Arduino Zero
		#ifdef _SAMD21_
			// Reload for SAMD21
			else if (_overflows > 0) {
				_TC->COUNT.reg = 0;              // Reset to 0
				_TC->INTENSET.reg = 0;              // disable all interrupts
				_TC->INTENSET.bit.OVF = 0;          // enable overfollow
				_TC->INTENSET.bit.MC0 = 1;          // disable compare match to CC0
				_TC->CC[0].reg = 65535;
			}
		#endif

		// SAMD51
		#ifdef __SAMD51__
			else if (_overflows > 0) {
				TC1->COUNT16.CC[1].reg = 0xFFFF;
			}
		#endif
	#endif

}

/**
 * Static envelope for Internal intermediate function to control timer interrupts
 */
#ifdef _VARIANT_ARDUINO_STM32_
	void uTimerLib::interrupt() {
		_instance->_interrupt();
	}
#endif


// Preinstantiate Object
uTimerLib TimerLib = uTimerLib();





/**
 * Attach Interrupts using internal functionality
 *
 * Note: This is device-dependant
 */
#ifdef ARDUINO_ARCH_AVR
	// Arduino AVR
	ISR(TIMER2_OVF_vect) {
		TimerLib._interrupt();
	}
#endif


#ifdef ARDUINO_ARCH_SAM
	void TC3_Handler() {
		TC_GetStatus(TC1, 0); // reset interrupt
		TimerLib._interrupt();
	}
#endif


#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	void uTimerLib::interrupt() {
		TimerLib._interrupt();
	}
#endif


#ifdef _SAMD21_
	void TC3_Handler() {
		// Overflow - Nothing, we will use compare to max value instead to unify behaviour
		if (TimerLib._TC->INTFLAG.bit.OVF == 1) {
			TimerLib._TC->INTFLAG.bit.OVF = 1;  // Clear flag
			TimerLib._interrupt();
		}
		// Compare
		if (TimerLib._TC->INTFLAG.bit.MC0 == 1) {
			TimerLib._TC->INTFLAG.bit.MC0 = 1;  // Clear flag
			TimerLib._interrupt();
		}
	}
#endif

#ifdef __SAMD51__
	void TC2_Handler() {
		// Overflow - Nothing, we will use compare to max value instead to unify behaviour
		if (TC1->COUNT16.INTFLAG.bit.OVF == 1) {
			TC1->COUNT16.INTENSET.bit.OVF = 1;  // Clear flag
		}
		// Compare
		if (TC1->COUNT16.INTFLAG.bit.MC0 == 1) {
			TC1->COUNT16.INTENSET.bit.MC0 = 1;  // Clear flag
			TimerLib._interrupt();
		}
	}
#endif
