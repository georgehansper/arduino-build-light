/*
	This code has been written for the Arduino Nano V3, to control one or more LEDs,
	based on commands given via the USB port

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	see <http://www.gnu.org/licenses/>.

	Author: George Hansper george@hansper.id.au
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
#define SMD_LED_pin 13	// Built-in Surface Mount Device LED (white)
#define RED_L_pin 6
#define GRN_L_pin 5
#define BLU_L_pin 3
#define RED_R_pin 11
#define GRN_R_pin 10
#define BLU_R_pin 9
#define EFFECT_pin	7

// LED Modes
#define LAMP_TEST 0	// Sequence of LED tests
#define STEADY 1	// LED(s) on always
#define BLINK  2	// Blink alternate LEDs
#define STROBE 3	// Blink both LEDs together
#define PULSE  4	// Fade-in / Fade-out both LEDs together
// #define FLASH  5
#define POLICE 6	// Alternate RED/BLUE
#define RAINBOW 7

// Start with the LAMP_TEST mode, which switches to STEADY when complete
int mode=LAMP_TEST;
long loop_count=0;
int period = 100;	// default period = 1s, period=1=10ms


// Current state of the LED colors

int RED_L=0;
int GRN_L=0;
int BLU_L=0;
int RED_R=0;
int GRN_R=0;
int BLU_R=0;


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

  Serial.begin(9600);
  Serial.setTimeout(100);
}

void serialEvent() {
  // really only need about 20 chars, but we have more than enough - let's splurge
  char cmd_str[80];
  int      cmd_length;

  cmd_length = Serial.readBytesUntil('\n', cmd_str, 80);
  cmd_str[cmd_length] = '\0';
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
  digitalWrite(SMD_LED_pin, HIGH);    // turn the LED on by making the voltage HIGH
}


// the loop routine runs over and over again forever:
void loop() {

  update_leds(); 

  delay(10);
  digitalWrite(SMD_LED_pin, LOW);    // turn the LED off by making the voltage LOW

  loop_count++;
  loop_count %= (period * 8);
}


void parse_cmd (char *cmd_str) {
    int R, G, B;
    int T;
    char *str;
    if ( strcmp(cmd_str, (char *) "RED") == 0 ) {
    	  mode=STEADY;
	  RED_L=255; GRN_L=0; BLU_L=0;
	  RED_R=255; GRN_R=0; BLU_R=0;
    } else if ( strcmp(cmd_str, (char *) "GREEN") == 0 ) {
    	  mode=STEADY;
	  RED_L=0; GRN_L=255; BLU_L=0;
	  RED_R=0; GRN_R=255; BLU_R=0;
    } else if ( strcmp(cmd_str, (char *) "BLUE") == 0 ) {
    	  mode=STEADY;
	  RED_L=0; GRN_L=0; BLU_L=255;
	  RED_R=0; GRN_R=0; BLU_R=255;
    } else if ( strcmp(cmd_str, (char *) "YELLOW") == 0 ) {
    	  mode=STEADY;
	  RED_L=255; GRN_L=255; BLU_L=0;
	  RED_R=255; GRN_R=255; BLU_R=0;
    } else if ( strcmp(cmd_str, (char *) "AMBER") == 0 ) {
    	  mode=STEADY;
	  RED_L=255; GRN_L=128; BLU_L=0;
	  RED_R=255; GRN_R=128; BLU_R=0;
    } else if ( strcmp(cmd_str, (char *) "WHITE") == 0 ) {
    	  mode=STEADY;
	  RED_L=255; GRN_L=255; BLU_L=255;
	  RED_R=255; GRN_R=255; BLU_R=255;
    } else if ( strcmp(cmd_str, (char *) "OFF") == 0 ) {
    	  mode=STEADY;
	  RED_L=0; GRN_L=0; BLU_L=0;
	  RED_R=0; GRN_R=0; BLU_R=0;
    } else if ( strcmp(cmd_str, (char *) "LEFT") == 0 ) {
          str = cmd_str+4;
	  if ( *str == '=' ) {
	  	R=strtol(++str,&str);
	  	if ( *str == ',' ) {
	  		G=strtol(++str,&str);
			if ( *str == ',' ) {
	  			B=strtol(++str,&str);
				RED_L=R; GRN_L=G, BLU_L=B;
				mode=STEADY;
			}
		}
	  }
    } else if ( strcmp(cmd_str, (char *) "RIGHT") == 0 ) {
          str = cmd_str+5;
	  if ( *str == '=' ) {
	  	R=strtol(++str,&str);
	  	if ( *str == ',' ) {
	  		G=strtol(++str,&str);
			if ( *str == ',' ) {
	  			B=strtol(++str,&str);
				RED_R=R; GRN_R=G, BLU_R=B;
				mode=STEADY;
			}
		}
	  }
   } else if ( strcmp(cmd_str, (char *) "PERIOD") == 0 ) {
          // Set loop_count limits
          str = cmd_str+6;
	  if ( *str == '=' ) {
	  	  T=strtol(++str,&str);    // T is in milliseconds
                  if ( T >= 10 && T <= 10000 ) {
		  	// 10 seconds is a reasonable limit
		  	period = T/10;
		  }
          }
   } else if ( strcmp(cmd_str, (char *) "STEADY") == 0 ) {
    	mode=STEADY;
//    } else if ( strcmp(cmd_str, (char *) "FLASH") == 0 ) {
//    	mode=FLASH;
    } else if ( strcmp(cmd_str, (char *) "BLINK") == 0 ) {
    	mode=BLINK;
    } else if ( strcmp(cmd_str, (char *) "STROBE") == 0 ) {
    	mode=STROBE;
    } else if ( strcmp(cmd_str, (char *) "PULSE") == 0 ) {
    	mode=PULSE;
    } else if ( strcmp(cmd_str,(char *) "000") == 0 ||  strcmp(cmd_str,(char *) "POLICE") == 0 ) {
 	mode=POLICE;
    } else if ( strcmp(cmd_str,(char *) "RAINBOW") == 0 ) {
 	mode=RAINBOW;
    } else if ( strcmp(cmd_str, (char *) "EFFECT") == 0 ) {
	  pinMode(EFFECT_pin, OUTPUT);
	  digitalWrite(EFFECT_pin, HIGH);    // simulate pressing the demo button the by making the voltage HIGH
	  delay(200);
	  digitalWrite(EFFECT_pin, LOW);    // Make the pin an input again (floating)
	  pinMode(EFFECT_pin, INPUT);
    } else if ( strcmp(cmd_str, (char *) "TEST") == 0 ) {
    	mode=LAMP_TEST;
	loop_count=0;
    } else if ( strcmp(cmd_str,(char *) "HELP") == 0 ||  strcmp(cmd_str,(char *) "?") == 0 ) {
 	usage();
    }
    update_leds();
}

void update_leds() {
   int R,G,B;
   long fade;
   switch(mode) {
   	case STEADY:		// steady
	        led_left(RED_L,GRN_L,BLU_L);			
	        led_right(RED_R,GRN_R,BLU_R);	
		break;
	case BLINK:		// blink left/right
		if( (loop_count/(period/2)) % 2 ) {
			   led_left(RED_L,GRN_L,BLU_L);			
			   led_right(0,0,0);
		} else {
			   led_right(RED_R,GRN_R,BLU_R);	
 			   led_left(0,0,0);			
		}
		break;
	case STROBE:		// blink together (Strobe)
		if( (loop_count/(period/2)) % 2 ) {
			   led_left(RED_L,GRN_L,BLU_L);			
			   led_right(RED_R,GRN_R,BLU_R);	
		} else {
 			   led_left(0,0,0);			
			   led_right(0,0,0);
		}
		break;
	case PULSE:		// fade pulse
		fade = abs(loop_count % period - period/2);
		// fade is an integer in the range 0 ... period/2, counting up, then down
		R=RED_L * fade / (period/2);
		G=GRN_L * fade / (period/2);
		B=BLU_L * fade / (period/2);
		led_left(R,G,B);
		R=RED_R * fade / (period/2);
		G=GRN_R * fade / (period/2);
		B=BLU_R * fade / (period/2);
		led_right(R,G,B);
		break;
	case POLICE: 	// Police
		if( (loop_count/(period/2)) % 2 ) {
			   led_left(255,0,0);			
			   led_right(0,0,255);
		} else {
 			   led_left(0,0,255);			
			   led_right(255,0,0);	
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

		led_left(R2,G2,B2);
		led_right(B2,R2,G2);
		break;
	
	case LAMP_TEST:		// Lamp test - cycle colors on startup
		// Run a lamp-test here (excluding the sound-effect_pin)
		switch (loop_count/50) {
			case 0:
			  led_left(255,0,0);
			  led_right(255,0,0);
			  break;
			case 1:
			  led_left(0,255,0);
			  led_right(0,255,0);
			  break;
			case 2:
			  led_left(0,0,255);
			  led_right(0,0,255);
			  break;
			case 3:
			  led_left(255,255,255);
			  led_right(0,0,0);
			  break;
			case 4:
			  led_left(0,0,0);
			  led_right(255,255,255);
			  break;
		  	default:
			  led_left(0,0,0);
			  led_right(0,0,0);
			  if ( mode == LAMP_TEST ) {
			  	mode=STEADY;
			  }
			  break;
		}	// switch (loop_count/50);
		break;
   }
}

void led_left(int R, int G, int B) {
  analogWrite(RED_L_pin,R);
  analogWrite(GRN_L_pin,G);
  analogWrite(BLU_L_pin,B);
}

void led_right(int R, int G, int B) {
  analogWrite(RED_R_pin,R);
  analogWrite(GRN_R_pin,G);
  analogWrite(BLU_R_pin,B);
}

int strcmp( char a[], char b[]) {
  unsigned int i=0;
  while ( a[i] == b[i] ) {
	if ( a[i] == '\0' ) {
		return(0);	//equal
	}
	i++;
  }
  if ( b[i] == '\0' && ( a[i] == ' ' || a[i] == ',' || a[i] == '=' || a[i] == '\n' )) {
  	return(0);	// equal up to separator
  }
  return(1);
}

int to_upper( char *s ) {
   //Serial.println(s);
   while( *s ) {
      if ( *s >= 'a' && *s <= 'z' ) {
      	*s = *s - ( 'a' - 'A' );
      }
      s++;
   }
   // Serial.println(s);
}

// like strtol, but only do decimal
int strtol(char *s,char **endptr) {
	int result=0;
	while(*s) {
		if ( *s >= '0' && *s <= '9' ) {
			result = result * 10;
			result = result + int( *s - '0' );
		} else {
			break;
		}
		s++;
	}
	*endptr=s;
	return result;
}

char *usage_txt =
"This unit accepts commands to the serial port, terminated by a new-line, eg:\n"
"\n"
"	echo RED > /dev/ttyUSB0\n"
"	echo FLASH > /dev/ttyUSB0\n"
"	echo PERIOD=500 > /dev/ttyUSB0\n"
"\n"
"To set the color of both LEDs (full brightness), use one of the following commands:\n"
"	RED\n"
"	GREEN\n"
"	BLUE\n"
"	YELLOW\n"
"	AMBER\n"
"	WHITE\n"
"\n"
"To set the color and brightness of the LEDs individually, use one of the following:\n"
"\n"
"	LEFT=R,G,B\n"
"	RIGHT=R,G,B\n"
"\n"
"where R G and B are integers in the range 0-255. eg:\n"
"	LEFT=128,0,255\n"
"	RIGHT=0,64,64\n"
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
"To set the rate of the blink/flash/pulse use\n"
"\n"
"	PERIOD=T\n"
"\n"
"where T is an integer from 10 to 10000\n"
"\n"
"To print this help, use:\n"
"	bash -c 'exec 3<>/dev/ttyUSB0; stty min 1 sane -hupcl  ; echo help >&3; sleep 2; cat <&3'\n"
"\n"
"\n"
"Powered by Arudino Nano v3\n"
"\n"
"George Hansper - george@hansper.id.au\n"
;

void usage() {
	Serial.println(usage_txt);
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


