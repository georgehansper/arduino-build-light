/*
	This code has been written for the Arduino Nano V3, to control two RGB LEDs,
	based on commands given via the USB port

	It contains additional code for activating an additional LED, and
	pulsing an electronic switch (EFFECT)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	see <http://www.gnu.org/licenses/>.

	Author: George Hansper george@hansper.id.au
 */
 
// For storing the HELP message without wasting RAM
#include <avr/pgmspace.h>
#define COMMON_ANODE 1

#if COMMON_ANODE == 1
	#define INVERT_LED(x) (~ x)
#else
	#define INVERT_LED(x) x
#endif

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
#define SMD_LED_pin 13	// Built-in Surface Mount Device LED (white)
#define RED_L_pin 3
#define GRN_L_pin 5
#define BLU_L_pin 6
#define RED_R_pin 11
#define GRN_R_pin 10
#define BLU_R_pin 9
#define EFFECT_pin	7
#define SWITCH_pin	2
#define SKIRT_pin	12

// LED Modes
#define LAMP_TEST 0	// Sequence of LED tests
#define STEADY	1	// LED(s) on always
#define BLINK	2	// Blink alternate LEDs
#define STROBE	3	// Blink both LEDs together
#define PULSE	4	// Fade-in / Fade-out both LEDs together
// #define FLASH	5
#define POLICE	6	// Alternate RED/BLUE
#define RAINBOW	7
#define TIMER	8
// Mask for 'which led(s)'
#define LEFT_MASK	0x01
#define RIGHT_MASK	0x02
#define FLASH_MASK	0x04
#define SKIRT_MASK	0x08

// Start with the LAMP_TEST mode, which switches to STEADY when complete
int left_mode=LAMP_TEST;
int right_mode=LAMP_TEST;
int skirt_mode=LAMP_TEST;
int which_leds=(LEFT_MASK|RIGHT_MASK);
unsigned int flash_counter = 0;
unsigned int flash_counter_default=10;	// 100 ms
unsigned long skirt_counter = 0;
unsigned long left_counter = 0;
unsigned long right_counter = 0;
int loop_count=0;
int period = 200;	// default period = 2s, period=1=10ms


// Current state of the LED colors

byte RED_L=0;
byte GRN_L=0;
byte BLU_L=0;
byte RED_R=0;
byte GRN_R=0;
byte BLU_R=0;
byte SKIRT_LED=0;

// Flash color (one-shot transient
byte RED_FLASH_L=0;
byte GRN_FLASH_L=0;
byte BLU_FLASH_L=0;
byte RED_FLASH_R=0;
byte GRN_FLASH_R=0;
byte BLU_FLASH_R=0;

// the setup routine runs once when you press reset:
void setup() {                
	// initialize the digital pin as an output.
	pinMode(SMD_LED_pin, OUTPUT);     
	pinMode(RED_L_pin, OUTPUT);
	pinMode(GRN_L_pin, OUTPUT);
	pinMode(BLU_L_pin, OUTPUT);
	pinMode(RED_R_pin, OUTPUT);
	pinMode(GRN_R_pin, OUTPUT);
	pinMode(BLU_R_pin, OUTPUT);
	// Set the sound/light EFFECT_pin trigger to INPUT until needed, so that the Demo button still works
	pinMode(EFFECT_pin, INPUT);
	pinMode(SKIRT_pin, OUTPUT);
	pinMode(SWITCH_pin, INPUT);
	digitalWrite(SWITCH_pin, HIGH);       // turn on pullup resistors

	digitalWrite(SKIRT_pin, HIGH);

	Serial.begin(9600);
	Serial.setTimeout(100);
}

void serialEvent() {
	// really only need about 20 chars, but we have more than enough - let's splurge
	char cmd_str[81];
	int      cmd_length;

	digitalWrite(SMD_LED_pin, HIGH);    // turn the LED on by making the voltage HIGH
	cmd_length = Serial.readBytesUntil('\n', cmd_str, 80);
	cmd_str[cmd_length] = '\n';
	cmd_str[cmd_length+1] = '\0';
	to_upper(cmd_str);
/*
	if ( cmd_length > 0 ) {
		Serial.print("cmd_str[");
		Serial.print(sizeof(cmd_str));
		Serial.print("] = ");
		Serial.println(cmd_str);
	}
*/
	if ( cmd_length > 0 ) {
		parse_cmd(cmd_str);
	}
} /* serialEvent() */


// the loop routine runs over and over again forever:
void loop() {

	if ( left_mode == right_mode ) {
		update_leds((LEFT_MASK|RIGHT_MASK),left_mode);
	} else {
		update_leds(LEFT_MASK,left_mode);
		update_leds(RIGHT_MASK,right_mode);
	}
	update_leds(SKIRT_MASK,skirt_mode);

	delay(10);
	digitalWrite(SMD_LED_pin, LOW);    // turn the LED off by making the voltage LOW

	loop_count++;
	loop_count %= (period * 8);
	if ( flash_counter > 0 ) {
		flash_counter--;
	}
	if ( left_counter > 0 ) {
		left_counter--;
		if ( left_counter == 0 ) {
		    RED_L=0; GRN_L=0; BLU_L=0;
		}
	}
	if ( right_counter > 0 ) {
		right_counter--;
		if ( right_counter == 0 ) {
		    RED_R=0; GRN_R=0; BLU_R=0;
		}
	}
	if ( skirt_counter > 0 ) {
		skirt_counter--;
		if ( skirt_counter == 0 ) {
		    SKIRT_LED=0;
		}
	}
	if ( digitalRead(SWITCH_pin) == LOW ) {
		launch_effect();
		loop_count+=20;
		loop_count %= (period * 8);
	}
} /* loop() */

void parse_cmd (char *cmd_str) {
	int R, G, B;
	long T;
	char *str;
	while(*cmd_str != '\0' ) {
		if ( strcmp(cmd_str, (char *) "RED") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=255; GRN_L=0; BLU_L=0;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=255; GRN_R=0; BLU_R=0;
			}
		} else if ( strcmp(cmd_str, (char *) "GREEN") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=0; GRN_L=255; BLU_L=0;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=0; GRN_R=255; BLU_R=0;
			}
		} else if ( strcmp(cmd_str, (char *) "BLUE") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=0; GRN_L=0; BLU_L=255;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=0; GRN_R=0; BLU_R=255;
			}
		} else if ( strcmp(cmd_str, (char *) "YELLOW") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=255; GRN_L=255; BLU_L=0;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=255; GRN_R=255; BLU_R=0;
			}
		} else if ( strcmp(cmd_str, (char *) "AMBER") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=255; GRN_L=128; BLU_L=0;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=255; GRN_R=128; BLU_R=0;
			}
		} else if ( strcmp(cmd_str, (char *) "WHITE" ) == 0 || strcmp(cmd_str, (char *) "ON" ) == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=255; GRN_L=255; BLU_L=255;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=255; GRN_R=255; BLU_R=255;
			}
			if ( which_leds & SKIRT_MASK ) {
				SKIRT_LED=1;
			}
		} else if ( strcmp(cmd_str, (char *) "OFF") == 0 ) {
			set_mode(which_leds,STEADY);
			if ( which_leds & LEFT_MASK ) {
				RED_L=0; GRN_L=0; BLU_L=0;
			}
			if ( which_leds & RIGHT_MASK ) {
				RED_R=0; GRN_R=0; BLU_R=0;
			}
			if ( which_leds & SKIRT_MASK ) {
				SKIRT_LED=0;
			}
		} else if ( strcmp(cmd_str, (char *) "LEFT") == 0 ) {
			which_leds=LEFT_MASK;
		} else if ( strcmp(cmd_str, (char *) "RIGHT") == 0 ) {
			which_leds=RIGHT_MASK;
		} else if ( strcmp(cmd_str, (char *) "SKIRT") == 0 ) {
			which_leds=SKIRT_MASK;
		} else if ( *cmd_str >= '0' && *cmd_str <= '9' ) {
			str=cmd_str;
			G=-1;
			T=-1;
			R=strtol(str,&str);
			if ( R >= 0 && *str++ == ',' ) {
				G=strtol(str,&str);
				if ( G >= 0 && *str++ == ',' ) {
					B=strtol(str,&str);
					if ( B >= 0 ) {
						if ( *str++ == ',' ) {
							T = strtol(str,&str);	// T is in milliseconds
							if ( T >= 0 ) {
								T = ( T + 5) / 10;
								if ( T == 0 ) {
									T = 1;
								}
							}
						}
						if ( which_leds & LEFT_MASK ) {
							RED_L=R; GRN_L=G, BLU_L=B;
							if ( T > 0 ) {
								left_counter = T;
							}
						} 
						if ( which_leds & RIGHT_MASK ) {
							RED_R=R; GRN_R=G, BLU_R=B;
							if ( T > 0 ) {
								right_counter = T;
							}
						}
						if ( which_leds & FLASH_MASK ) {
							RED_FLASH_R=R; GRN_FLASH_R=G, BLU_FLASH_R=B;
							RED_FLASH_L=R; GRN_FLASH_L=G, BLU_FLASH_L=B;
							if ( T > 0 && T <= 30000 ) {
								flash_counter_default = T;
								flash_counter = flash_counter_default;
							}
						}
						set_mode(which_leds,STEADY);
					}
				}
			}
			if ( which_leds & SKIRT_MASK ) {
				// any non-zero value is ON
				SKIRT_LED=R;
				set_mode(SKIRT_MASK,STEADY);
				if ( T == -1 && G > 0 ) {
					T = G;
				}
				if ( T > 0 ) {
					T = ( T + 5) / 10;
					if ( T == 0 ) {
						T = 1;
					}
					skirt_counter = T;
				}
			} 
		} else if ( strcmp(cmd_str, (char *) "PERIOD") == 0 ) {
			// Set loop_count limits
			str = cmd_str+6;
			if ( *str == '=' ) {
				T=strtol(++str,&str);	// T is in milliseconds
				if ( T >= 20 && T <= 10000 ) {
					// 10 seconds is a reasonable limit
					period = T/10;
				}
			}
		} else if ( strcmp(cmd_str, (char *) "STEADY") == 0 ) {
			set_mode(which_leds,STEADY);
		} else if ( strcmp(cmd_str, (char *) "FLASH") == 0 ) {
			flash_counter = flash_counter_default;
			which_leds = FLASH_MASK;
		} else if ( strcmp(cmd_str, (char *) "BLINK") == 0 ) {
			set_mode(which_leds,BLINK);
		} else if ( strcmp(cmd_str, (char *) "STROBE") == 0 ) {
			set_mode(which_leds,STROBE);
		} else if ( strcmp(cmd_str, (char *) "PULSE") == 0 ) {
			set_mode(which_leds,PULSE);
		} else if ( strcmp(cmd_str, (char *) "000") == 0 || strcmp(cmd_str,(char *) "POLICE") == 0 ) {
			set_mode(which_leds,POLICE);
		} else if ( strcmp(cmd_str, (char *) "RAINBOW") == 0 ) {
			set_mode(which_leds,RAINBOW);
		} else if ( strcmp(cmd_str, (char *) "EFFECT") == 0 ) {
			launch_effect();
		} else if ( strcmp(cmd_str, (char *) "TEST") == 0 ) {
			if ( which_leds == (LEFT_MASK|RIGHT_MASK) ) {
				set_mode((LEFT_MASK|RIGHT_MASK|SKIRT_MASK),LAMP_TEST);
			} else {
				set_mode(which_leds,LAMP_TEST);
			}
			if ( period < 350 ) {
				/* Otherwise we get stuck in test mode - still accepting commands, but a little annoying */
				period=500;
			}
			loop_count=0;
		} else if ( strcmp(cmd_str,(char *) "HELP") == 0 || strcmp(cmd_str,(char *) "?") == 0 ) {
			usage();
		} else if ( *cmd_str == '\n' ) {
			which_leds = (LEFT_MASK|RIGHT_MASK);
			cmd_str++;
		} else if ( *cmd_str == ' ' || *cmd_str == '=' || *cmd_str == '\t' ) {
			cmd_str++;
			continue;
		} else {
			/* Command line not valid, or finished */
			break;
		}
		/* Move cmd_str pointer to next item - skip space, tab = */
		while ( *cmd_str != '\0' && *cmd_str != ' ' && *cmd_str != '\n' && *cmd_str != '=' && *cmd_str != '\t' ) {
			cmd_str++;
		}
	} /* while ( *cmd_str != '\0' ) */
	if ( left_mode == right_mode ) {
		update_leds((LEFT_MASK|RIGHT_MASK),left_mode);
	} else {
		update_leds(LEFT_MASK,left_mode);
		update_leds(RIGHT_MASK,right_mode);
	}
	update_leds(SKIRT_MASK,skirt_mode);
}

void launch_effect () {
	pinMode(EFFECT_pin, OUTPUT);
	digitalWrite(EFFECT_pin, HIGH);		// simulate pressing the demo button the by making the voltage HIGH
	delay(200);
	digitalWrite(EFFECT_pin, LOW);		// Make the pin an input again (floating)
	pinMode(EFFECT_pin, INPUT);
}

void update_leds(int which,int mode) {
	byte R,G,B;
	long fade;
	// FLASH overrides the current mode for LEFT and RIGHT leds
	if ( flash_counter > 0 && ( which & (LEFT_MASK|RIGHT_MASK) != 0 ) ) {
		set_leds((LEFT_MASK|RIGHT_MASK),RED_FLASH_L,GRN_FLASH_L,BLU_FLASH_L);			
		return;
	}
	switch(mode) {
	case STEADY:		// steady
		set_leds((which & LEFT_MASK),RED_L,GRN_L,BLU_L);			
		set_leds((which & RIGHT_MASK),RED_R,GRN_R,BLU_R);	
		set_leds((which & SKIRT_MASK),SKIRT_LED,0,0);
		break;
	case BLINK:		// blink left/right
		if( (loop_count/(period/2)) % 2 ) {
			set_leds((which & LEFT_MASK),RED_L,GRN_L,BLU_L);			
 			set_leds((which & RIGHT_MASK),0,0,0);			
			set_leds((which & SKIRT_MASK),1,0,0);
		} else {
			set_leds((which & RIGHT_MASK),RED_R,GRN_R,BLU_R);	
 			set_leds((which & LEFT_MASK),0,0,0);			
			set_leds((which & SKIRT_MASK),0,0,0);
		}
		break;
	case STROBE:		// blink together (Strobe)
		if( (loop_count/(period/2)) % 2 ) {
			set_leds((which & LEFT_MASK),RED_L,GRN_L,BLU_L);			
			set_leds((which & RIGHT_MASK),RED_R,GRN_R,BLU_R);	
			set_leds((which & SKIRT_MASK),1,0,0);
		} else {
 			set_leds(which,0,0,0);			
		}
		break;
	case PULSE:		// fade pulse
		fade = abs(loop_count % period - period/2);
		// fade is an integer in the range 0 ... period/2, counting up, then down
		if ( which & LEFT_MASK ) {
			R=RED_L * fade / (period/2);
			G=GRN_L * fade / (period/2);
			B=BLU_L * fade / (period/2);
			set_leds(LEFT_MASK,R,G,B);			
		}
		if ( which & RIGHT_MASK ) {
			R=RED_R * fade / (period/2);
			G=GRN_R * fade / (period/2);
			B=BLU_R * fade / (period/2);
			set_leds(RIGHT_MASK,R,G,B);	
		}
		if ( which & SKIRT_MASK ) {
			if ( fade > period/4 ) {
				set_leds((which & SKIRT_MASK),1,0,0);
			} else {
				set_leds((which & SKIRT_MASK),0,0,0);
			}
		}
		break;
	case POLICE: 	// Police
		if( (loop_count/(period/2)) % 2 ) {
				set_leds((which & LEFT_MASK),255,0,0);			
				set_leds((which & RIGHT_MASK),0,0,255);
		} else {
 				set_leds((which & LEFT_MASK),0,0,255);			
				set_leds((which & RIGHT_MASK),255,0,0);	
		}
		break;
	case RAINBOW:		// rainbow
		long R2,G2,B2;
		R2 = abs(loop_count % (period*2) - period);
		// R is an integer in the range 0..period, counting up, then down over period*2
		if ( R2 >= period/3 ) {
			R2 = (R2 - period/3) * 512 / (period*3);
		} else {
			R2=0;
		}
		G2 = abs((loop_count+(4*period/3)) % (period*2) - period);
		if ( G2 >= period/3 ) {
			G2 = (G2- period/3) * 512 / (period*3);
		} else {
			G2=0;
		}
		
		B2 = abs((loop_count+(2*period/3)) % (period*2) - period);
		if ( B2 >= period/3 ) {
			B2 = (B2- period/3) * 512 / (period*3);
		} else {
			B2=0;
		}

		set_leds((which & LEFT_MASK),R2,G2,B2);
		set_leds((which & RIGHT_MASK),B2,R2,G2);
		break;
	
	case LAMP_TEST:		// Lamp test - cycle colors on startup
		// Run a lamp-test here (excluding the sound-effect_pin)
		switch (loop_count/50) {
			case 0:
				set_leds(which,255,0,0);
				break;
			case 1:
				set_leds(which,0,255,0);
				break;
			case 2:
				set_leds(which,0,0,255);
				break;
			case 3:
				set_leds(which,255,255,255);
				break;
			case 4:
				set_leds(which,0,0,0);
				break;
			default:
				set_leds(which,0,0,0);
				if ( which & LEFT_MASK) {
					left_mode=RAINBOW;
				}
				if ( which & RIGHT_MASK) {
					right_mode=RAINBOW;
				}
				if ( which & SKIRT_MASK) {
					skirt_mode=STEADY;
					SKIRT_LED=1;
				}
				break;
		}	/* switch (loop_count/50) */
		break;
	} /* switch mode */
} /* update_leds() */

void set_mode(int which, int mode) {
	if ( which & LEFT_MASK ) {
		left_mode = mode;
	}
	if ( which & RIGHT_MASK ) {
		right_mode = mode;
	}
	if ( which & SKIRT_MASK ) {
		skirt_mode = mode;
	}
} /* set_mode() */

void set_leds(int which, byte R, byte G, byte B) {
	if ( which & LEFT_MASK ) {
		analogWrite(RED_L_pin,INVERT_LED(R));
		analogWrite(GRN_L_pin,INVERT_LED(G));
		analogWrite(BLU_L_pin,INVERT_LED(B));
	}
	if ( which & RIGHT_MASK ) {
		analogWrite(RED_R_pin,INVERT_LED(R));
		analogWrite(GRN_R_pin,INVERT_LED(G));
		analogWrite(BLU_R_pin,INVERT_LED(B));
	}
	if ( which & SKIRT_MASK ) {
		if ( R ) {
			digitalWrite(SKIRT_pin, HIGH);
		} else {
			digitalWrite(SKIRT_pin, LOW);
		}
	}
} /* set_leds() */

int strcmp( char a[], char b[]) {
	unsigned int i=0;
	while ( a[i] == b[i] ) {
		if ( a[i] == '\0' ) {
			return(0);	//equal
		}
		i++;
	}
	if ( b[i] == '\0' && ( a[i] == ' ' || a[i] == ',' || a[i] == '=' || a[i] == '\n' || a[i] == '\t' )) {
		return(0);	// equal up to separator
	}
	return(1);
} /* strcmp() */

int to_upper( char *s ) {
	//Serial.println(s);
	while( *s ) {
		if ( *s >= 'a' && *s <= 'z' ) {
			*s = *s - ( 'a' - 'A' );
		}
		s++;
	}
	// Serial.println(s);
} /* to_upper */

// like strtol, but only do decimal
long strtol(char *s,char **endptr) {
	long result=0;
	*endptr = s;
	while( *s >= '0' && *s <= '9') {
		result = result * 10;
		result = result + int( *s - '0' );
		s++;
	}
	if ( *endptr == s ) {
		/* No digits matched */
		return -1;
	} else {
		*endptr=s;
		return result;
	}
} /* strtol() */

void prg_print(char *str) {
	char line[80];
	char cc;
	int  ndx;

	ndx = 0;
	while( cc = pgm_read_byte(str++)) {
		line[ndx++]=cc;
		if ( cc == '\n' || ndx == 79 ) {
			line[ndx]='\0';
			Serial.print(line);
			ndx=0;
		}
	}
	if ( ndx > 0 ) {
		line[ndx]='\0';
		Serial.print(line);
	}
	return;
} /* prg_print() */

/*
   Storing the HELP text as a simple string constant chews up the available RAM,
   because this static data is loaded into RAM by the compiler
   The PROGMEM directive keeps the HELP text in program store (flash) and we read it out using pgm_read_byte()
*/
PROGMEM prog_uchar usage_txt[] = {
"This unit accepts commands to the serial port, terminated by a new-line, eg:\n"
"\n"
"	echo RED > /dev/ttyUSB0\n"
"	echo STROBE > /dev/ttyUSB0\n"
"	echo PERIOD=500 > /dev/ttyUSB0\n"
"\n"
"To set the color of both LEDs (full brightness), use one of the following commands:\n"
"	RED\n"
"	GREEN\n"
"	BLUE\n"
"	YELLOW\n"
"	AMBER\n"
"	WHITE\n"
"	R,G,B\n"
"\n"
"To set the color and brightness of the LEDs individually, use one of the following:\n"
"\n"
"	LEFT=R,G,B\n"
"	RIGHT=R,G,B\n"
"	LEFT=R,G,B,T\n"
"	RIGHT=R,G,B,T\n"
"\n"
"where R G and B are integers in the range 0-255. T is in milliseconds (32bit signed) eg:\n"
"	LEFT=128,0,255\n"
"	RIGHT=0,64,64\n"
"\n"
"To specify a timeout on the LED(s) after which it turns off, use\n"
"	255,255,255,1000\n"
"	LEFT=255,0,0,1000\n"
"	RIGHT=0,255,0,90000\n"
"\n"
"To blink/pulse the LED indepenantly, use:\n"
"	LEFT=128,0,255 PULSE\n"
"	RIGHT GREEN PULSE\n"
"\n"
"To flash both LEDs breifly a different color use the following:\n"
"	FLASH=R,G,B,T\n"
"\n"
"where R G and B are integers in the range 0-255, and T is in milliseconds eg:\n"
"	FLASH=255,255,255,50\n"
"	FLASH=255,0,0,200\n"
"values are saved, so the same flash can be repeated using simply\n"
"	FLASH\n"
"\n"
"Other commands:\n"
"	OFF	...turn both LEDs off\n"
"		   (you can turn on again using a color command)\n"
"	BLINK	...flash the LEDs alternately (one LED on at a time)\n"
"	STROBE	...flash the LEDs together (both LEDs on at the same time)\n"
"	PULSE	...dim and brighten the LEDs together\n"
"		   note that BLINK, STROBE, and PULSE have no effect if the LEDs are OFF\n"
"	STEADY	...stop blinking/flashing/pulsing and leave the LEDs on steadily\n"
"	POLICE	...alternate RED and BLUE\n"
"	000	...alternate RED and BLUE\n"
"	TEST	...run the lamp test sequence (as on power-up)\n"
"	RAINBOW	...just try it\n"
"	EFFECT	...start or cancel the sound+motion EFFECT (equivalent to pressing demo)\n"
"\n"
"	SKIRT=0 ...Turn off skirt LEDs\n"
"	SKIRT=1,1000 ...Turn on skirt LEDs for 1000 ms\n"
"\n"
"To set the rate of the blink/flash/pulse use\n"
"\n"
"	PERIOD=T\n"
"\n"
"where T is an integer from 20 to 10000\n"
"\n"
"To print this help, use:\n"
"	bash -c 'exec 3<>/dev/ttyUSB0; stty min 1 sane -hupcl  ; echo help >&3; sleep 2; cat <&3'\n"
"\n"
"To prevent the arduino from being reset on every open/close, use\n"
"	stty -F /dev/ttyUSB0 -hupcl\n"
"\n"
"Recommended udev configuration - create file /etc/udev/rules.d/99-arduino.tty.rules with the following line\n"
"\n"
"SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"0403\", ATTRS{idProduct}==\"6001\", RUN+=\"/bin/stty -F /dev/$name 9600 min 1 -hupcl -opost -cstopb cs8 -istrip -parenb inpck -ignpar -parodd cread clocal -crtscts\"\n"
"\n"
"	Version $Id$\n"
"\n"
"Powered by Arudino Nano v3\n"
"\n"
"George Hansper - george@hansper.id.au\n"
};

void usage() {
	prg_print((char *) usage_txt);
}

/*
  Serial.print("cmd_str[");
  Serial.print(sizeof(cmd_str));
  Serial.print("] = ");
  Serial.println(cmd_str);

  Serial.print("a[");
  Serial.print(sizeof((*a)));
  Serial.print("] = ");
  Serial.print(a);
  Serial.print("   b[");
  Serial.print(sizeof((*b)));
  Serial.print("] = ");
  Serial.println(b);
*/
//  delayMicroseconds(500); //Wait for 500us to go into reset

//	vim: syntax=c
