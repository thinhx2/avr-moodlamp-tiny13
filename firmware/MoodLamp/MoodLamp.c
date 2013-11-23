/*
 * MoonLamp.c
 *
 * Created: 29.04.2012 16:12:53
 *  Author: Trol
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/interrupt.h>

uint8_t currentColor[3];
uint8_t startColor[3];
uint8_t nextColor[3];

#define currentR currentColor[0]
#define currentG currentColor[1]
#define currentB currentColor[2]
#define startR startColor[0]
#define startG startColor[1]
#define startB startColor[2]
#define nextR nextColor[0]
#define nextG nextColor[1]
#define nextB nextColor[2]


uint16_t volatile tickBeforeChange;
volatile uint16_t cntColorChange = 0;		// counter for next color recalculation, 0 .. tickBeforeChange-1

volatile uint8_t rndTime = 50;
volatile uint8_t rndR = 0;
volatile uint8_t rndG = 0;
volatile uint8_t rndB = 150;
volatile uint8_t rndDarkIndex;	// pregenerated random values
volatile bool randomValuesReady = false;



#define TIMER_FREQUENCY	9600000
#define TIMER_OVERFLOW_FREQUENCY	(TIMER_FREQUENCY/0xff)


static uint8_t calculateColor(uint8_t from, uint8_t to);
static void calcNextPoint();

/*
uint32_t random() {
	/
	 * Compute x = (7^5 * x) mod (2^31 - 1)
	 * wihout overflowing 31 bits:
	 *      (2^31 - 1) = 127773 * (7^5) + 2836
	 * From "Random number generators: good ones are hard to find",
	 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
	 * October 1988, p. 1195.
	 /
	long hi, lo;

	static long x = 1;
	// Can't be initialized with 0, so use another value.
	if (x == 0)
		x = 123459876L;
	hi = x / 127773L;
	lo = x % 127773L;
	x = 16807L * lo - 2836L * hi;
	if (x < 0)
		x += 0x7fffffffL;
	return x;
}
*/
/*
uint16_t random() {
	static uint16_t next = 1;
	uint16_t hi, lo;
	
	if (next == 0) {
		next = 12345;
	}
	hi = next / 16390;
	lo = next % 16390;
	next = 1470 * lo - 3247 * hi;
	if (next < 0) {
		next = -next;
	}
	return next;// % max;
}
*/

	
/** 
 * Timer overflow interrupt. Used for PWM implementation
 * Called with TIMER_OVERFLOW_FREQUENCY frequency 
 */
ISR(TIM0_OVF_vect){
	static uint8_t cntPwm = 0;		// PWM counter	0..0xff
	
	if ( cntPwm == 0 ) {
		// recalculate bright
		currentR = calculateColor(startR, nextR);
		currentG = calculateColor(startG, nextG);
		currentB = calculateColor(startB, nextB);
		
		cntColorChange++;
		if ( cntColorChange >= tickBeforeChange ) {
			cntColorChange = 0;
			calcNextPoint();
		}
	}
	
	// update channels
	uint8_t output = 0;
	if ( currentR > cntPwm ) {
		output |= _BV(0);
	}
	if ( currentG > cntPwm ) {
		output |= _BV(1);
	}
	if ( currentB > cntPwm ) {
		output |= _BV(2);
	}
	PORTB = output; 
	
	cntPwm++;
}


__attribute__ ((noinline)) static uint8_t calculateColor(uint8_t from, uint8_t to) {
	uint32_t bright = 0xff;
	bright *= cntColorChange;
	bright /= tickBeforeChange;

	int32_t res = to - from;
	res = res * bright;
	res = res / 0xff;
	res += from;
	
	return res;
}

__attribute__ ((noinline)) static uint8_t rndColor() {
	return random() % 250;
}

	
static void calcNextPoint() {
	startR = nextR;
	startG = nextG;
	startB = nextB;

	nextR = rndR;
	nextG = rndG;
	nextB = rndB;
	
	nextColor[rndDarkIndex] = 0;

	tickBeforeChange = (TIMER_OVERFLOW_FREQUENCY/0xff)*rndTime/10;
	
	randomValuesReady = false;
}



int main(void) {
	nextR = 0;
	nextG = 0;
	nextB = 0;

	calcNextPoint();
	
	DDRB = 0xff;
	TCCR0B = _BV(CS00);		// disable prescaler for timer
	TIMSK0 = _BV(TOIE0);	// enable timer overflow interrupts
	sei();					// enable global interrupts

    while (1) {
		if (!randomValuesReady) {
			rndTime = random() % 50;
			rndTime += 30;					// time to change color, sec/10
			rndDarkIndex = random() % 3;
			rndR = rndColor();
			rndG = rndColor();
			rndB = rndColor();
			randomValuesReady = true;
		}
	}
}