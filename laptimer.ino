/**************************************************
 * The idea is to display sector- and running laptimes
 * in a rental go kart running on a track with alfono
 * magnetic strips. ie  https://www.alfanosystem.com/
 *
 * Attched a LCD Keypad shield
 * using D4,D5,D6,D7,D8,D9 and D10 for the display
 * using A0 for the buttons.
 * https://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)
 *
 * Attached a reed switch
 * using D2
 *
 * TODO:
 * Displaysetup made for 3 sectors, make a better where
 *   1 sector = 1 lap makes sence
 * Save trackname and records? 
 * Add a buzzer to make a sound instead
 * Add a compass to detect direction when passing a
 *   magnet strip, and thus be able to auto-configure
 * Make a list of tracks with known strips
 * Add a microphone - guess frequency = rpm = speed
 ***************************************************/

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#include <EEPROM.h>

#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   4
#define btnLEFT   8
#define btnSELECT 16
#define btnNONE   0
#define btnANY    B11111111
#define btnINC    btnUP | btnRIGHT
#define btnDEC    btnDOWN | btnLEFT


#define REEDPIN 2
#define LEDPIN 13

#define LONGLAP  120000
#define MAX_SECTORS 3
#define LAP 0

//Magic value signaling not valid
//has to fit a signed int
#define NOTIME 0x7FED

#define DEBUG

#ifdef DEBUG
#define PRINT(x)    Serial.print (x)
#define PRINTDEC(x) Serial.print (x, DEC)
#define PRINTLN(x)  Serial.println (x)
#define BEGIN(x)    Serial.begin(x)
#define DEBOUNCE 500
#else
#define PRINT(x)
#define PRINTDEC(x)
#define PRINTLN(x) 
#define BEGIN(x)
#define DEBOUNCE 5000
#endif

// where we store our config
#define CONFADDR 20
#define CONFVER  1

volatile int newlap;
volatile long reedready;
long lasttime;
int  sectors;
int  cursector;
int  lastsector;
int  curlap;
int  state;

struct timing_struct {
  unsigned int best;    
  unsigned int last;
  int diff; //last - previous best
} 
timing_struct;

struct timing_struct timing[MAX_SECTORS+1];

void setup() {
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0,0);
  lcd.print(F("CHR.HANSEN HOLD6")); 
  lcd.setCursor(0,1);
  lcd.print(F("laptimer ver 0.4")); 
  BEGIN(9600);
  //Activate internal pullup
  pinMode(REEDPIN , INPUT);
  digitalWrite(REEDPIN , HIGH);
  // turn off led
  pinMode(LEDPIN , OUTPUT);
  digitalWrite(LEDPIN , LOW);

  attachInterrupt(0, trigger, FALLING);
  cleardata();
  //read from flash?
  sectors   = 3;
  getconf();

  delay(5000);
  lcd.clear();
  show();
}

void cleardata() {
  newlap    = 0;
  reedready = 0;
  lasttime  = 0;
  //sectors   = 3; config setting
  cursector = 0;
  lastsector= 0;
  curlap    = 0;
  state     = 0; //waiting to start
  timing[0] = { 
    NOTIME, NOTIME, NOTIME  }; //lap
  timing[1] = { 
    NOTIME, NOTIME, NOTIME  }; //sec1
  timing[2] = { 
    NOTIME, NOTIME, NOTIME  }; //sec2
  timing[3] = { 
    NOTIME, NOTIME, NOTIME  }; //sec3  
}

void getconf() {
  if ( EEPROM.read(CONFADDR) == 'L' and
    EEPROM.read(CONFADDR+1) == 'T' ) {
    int confver = EEPROM.read(CONFADDR+2);
    if ( confver == 1 ) {
      sectors = EEPROM.read(CONFADDR+3);
    }
  }
}

void saveconf() {
  EEPROM.write(CONFADDR, 'L');
  EEPROM.write(CONFADDR+1,'T' );
  EEPROM.write(CONFADDR+2, 1);
  EEPROM.write(CONFADDR+3, sectors);
}


void loop() {
  //Just wait for an interrupt
  static int key = btnNONE;
  static int lastkey = btnNONE;
  static int keycount = 0;

  if (newlap > 0) {
    //calculate
    //Don't process interrupts during calculations
    detachInterrupt(0);

    if (state == 0) {
      //first hit
      lastsector = sectors;
      cursector  = 1;
      curlap     = 1;
    } 
    else {
      savetime(cursector, reedready - lasttime);

      lastsector = cursector;
      if (cursector == sectors) {
        //start/finish passing
        cursector = 1;
        curlap++;
      } 
      else {
        cursector++;
      }
    }

    state      = 1; //running

    show();
    lasttime = reedready;     
    newlap = 0;

    attachInterrupt(0, trigger, FALLING);
  }
  delay(100);
  if ( state < 2 and millis() - reedready > LONGLAP) {
    state=2;
    show();
  }
  key = getkey();
  if ( key != btnNONE ) {
    if ( key == lastkey ) {
      keycount++;
      PRINTLN(key);
      PRINTLN(keycount);
    } 
    else {
      keycount = 1;
      PRINTLN("newkey");
      PRINTLN(key);
    }

    if ( key == btnSELECT and keycount > 20 ) {
      // 2 seconds holding
      PRINTLN("Select hold");
      int confsectors = sectors;
      config();
      keycount=0;
      if ( confsectors != sectors ) {
        cleardata();
        saveconf();
      }
      show();
    }
  }
  lastkey = key;

}

void config() {
  int key = btnNONE;
  int done = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("# sectors:");
  lcd.print(sectors);
  //wait for release of select button
  waitkeynone();
  do {
    key = waitkey(btnANY);
    if (key != btnNONE ) {
      if (key == btnUP or key == btnRIGHT) {
        sectors++;
      }  
      if (key == btnDOWN or key == btnLEFT) {
        sectors--;
      }
      if ( sectors > MAX_SECTORS ) {
        sectors = 1;
      } 
      else if ( sectors < 1 ) {
        sectors = MAX_SECTORS;
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("# sectors:");
      lcd.print(sectors);
    }
    if ( key == btnSELECT ) {
      //save and exit
      done = 1;
      break;
    }
    waitkeynone();
  } 
  while (done == 0);
  PRINTLN("out config");
}

void savetime(int sectoridx, long time) {
  PRINT("Sector,time:");
  PRINT(sectoridx);
  PRINT(",");
  PRINTLN(time);
  //avoid overflow
  int sectortime = int((time+5)/10);
  if ( time > LONGLAP ) {
    sectortime = NOTIME + 1;
  }

  if (timing[sectoridx].last == NOTIME) {
    //first time
    timing[sectoridx].best = sectortime;
    timing[sectoridx].last = sectortime;
    timing[sectoridx].diff = 0;
  } 
  else {
    timing[sectoridx].diff = sectortime - timing[sectoridx].best;
    timing[sectoridx].last = sectortime;

    if ( sectortime < timing[sectoridx].best ) {
      timing[sectoridx].best = sectortime;
    }
  }

  PRINT("best, last, diff:");
  PRINT(timing[sectoridx].best);
  PRINT(",");
  PRINT(timing[sectoridx].last);
  PRINT(",");
  PRINTLN(timing[sectoridx].diff);

  if (sectoridx != LAP) {   
    //calculate running lap
    long runlap = 0;
    //sum of all sectors
    for (int i=1; i <= sectors; i++) {
      if (timing[i].last == NOTIME) {
        runlap = NOTIME;
        break;
      } 
      runlap += timing[i].last;
    }
    //timing is in millis
    if (runlap != NOTIME) {
      savetime(LAP,(long)(runlap*10));
    }
  }
}

void show() {
  // xx.xx +x.xx aaaa  - sectortime sectordiff  status
  // xx.xx +x.xx ll:s  - laptime    lapdiff     curlap|cursec 

  char buff[8];
  lcd.clear();

  PRINT("sec: best, last, diff:");
  PRINT(timing[lastsector].best);
  PRINT(",");
  PRINT(timing[lastsector].last);
  PRINT(",");
  PRINTLN(timing[lastsector].diff);
  PRINT("lap: best, last, diff:");
  PRINT(timing[LAP].best);
  PRINT(",");
  PRINT(timing[LAP].last);
  PRINT(",");
  PRINTLN(timing[LAP].diff);


  lcd.setCursor(0,0);
  lcd.print( ftime(timing[lastsector].last,buff));

  lcd.setCursor(0,1);
  lcd.print( ftime(timing[LAP].last,buff));

  lcd.setCursor(12,0);
  if (state == 0) {
    lcd.print("WAIT");
  } 
  else if (state == 1) {
    lcd.print("RACE");
  } 
  else {
    lcd.print("STOP");
  }


  lcd.setCursor(6,0);

  if (curlap < 2) {
    lcd.print(" *.**");
  } 
  else {
    lcd.print( fdiff(timing[lastsector].diff,buff));
  }


  //sector and laptime is identical 
  if ( sectors > 1 ) {
    lcd.setCursor(6,1);

    if (curlap < 2) {
      lcd.print(" *.**");
    } 
    else {
      lcd.print( fdiff(timing[LAP].diff,buff));
    }


    lcd.setCursor(12,1);
    if (curlap > 0) {
      if (curlap < 10) {
        lcd.print("0");
      }
      lcd.print(curlap);
      lcd.print("|");
      lcd.print(cursector);
    } 
    else {
      lcd.print("**|*");
    }
  } 
  else {
    //last line in one-sector-mode
    lcd.setCursor(0,1);
    lcd.print("BEST: ");
    lcd.print( ftime(timing[LAP].best,buff) );

    lcd.setCursor(12,1);
    lcd.print("L:");
    if (curlap > 0) {
      if (curlap < 10) {
        lcd.print("0");
      }
      lcd.print(curlap);
    } 
    else {
      lcd.print("**");
    }
  }

  if ( (timing[lastsector].diff != NOTIME and timing[lastsector].diff < 0) or
    (timing[LAP].diff != NOTIME and timing[LAP].diff < 0)) {
    lcd.setCursor(12,0);
    lcd.print("BEST");
    for (int i=0; i<5; i++) {
      delay(500);
      lcd.noDisplay();
      delay(100);
      lcd.display();
    }
  }    

}

char* fdiff(int diff, char* buf) {
  int num = fabs(diff);
  if (diff >= 1000) {
    num = 999;
  }
  int secs = int(num/100);
  int hund = num % 100;
  sprintf(buf, "%c%01d.%02d", diff>0?'+':'-', secs, hund); 
  return(buf);   
}

char* ftime(int time, char* buf) {
  int num = fabs(time);
  if ( time == NOTIME ) {
    strcpy(buf,"**.**");
    return(buf);
  } 
  else if (time >= 10000) {
    num = 9999;     
  } 
  else if (time < 0) {
    num = 0;
  }
  int secs = int(num/100);
  int hund = num % 100;
  sprintf(buf, "%2d.%02d", secs, hund); 
  return(buf);   
}

int getkey()
{
  int adc_key_in = analogRead(0); 
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 250)  return btnUP; 
  if (adc_key_in < 450)  return btnDOWN; 
  if (adc_key_in < 650)  return btnLEFT; 
  if (adc_key_in < 850)  return btnSELECT;  

  return btnNONE;  // when all others fail, return this...
}


void waitkeynone() {
  do {
    delay(5);
  } 
  while(getkey() != btnNONE);
  // debounce
  delay(20);
}

uint8_t waitkey(uint8_t keymask) {
  uint8_t sKey = btnNONE;
  do  {
    sKey = getkey() & keymask;    
  }  
  while(sKey == btnNONE);

  delay(20);
  return sKey;
}

void trigger() {
  if( millis() > reedready + DEBOUNCE){
    newlap++;
    reedready = millis();
  }
}


