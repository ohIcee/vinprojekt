
#define Pot 3

#include <LiquidCrystal.h>
#include <stepperUNO.h>
#include <EEPROM.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

keypad Keypad;
stepMOTOR motor;

long EEPROMReadlong(long address)
{
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

volatile byte meniStatus = 0;
volatile byte tipka = 0;
volatile byte posCursor3 = 1;
volatile byte posCursor1 = 0;
volatile byte spd = 0;
volatile byte rset = 5; // Spremeni za spremembo št. klikov za reset števcev
volatile byte rst = rset;
volatile byte aIzmetGlava = EEPROM.read(6); // IzmetGlava
volatile byte aIzmetPn = EEPROM.read(7);    // IzmetPN

volatile int stObrL = EEPROM.read(0);
volatile int stObrD = EEPROM.read(1);
volatile int fakt = 0;
volatile int faktCasa = 30;
volatile int korak = 20;

// & 0xFF izvede AND opeacijo nad biti
volatile int zakasnitevPn = EEPROM.read(10) & 0xFF) + ((EEPROM.read(11) << 8) & 0xFFFF);
volatile int zakasnitevGlava = (EEPROM.read(12) & 0xFF) + ((EEPROM.read(13) << 8) & 0xFFFF);

// Število ciklov (shranjenih v EEPROM)
volatile long stCiklov = EEPROMReadlong(2);

// uporabljeno za prikaz na zaslonu
bool isWorking = false;

bool stiskPn = false;
bool stiskGlava = false;
bool stiskStart = false;
bool izklGlava = false;
bool izklPn = false;

// 400 je en obrat
long levo = stObrL * 400;
long desno = stObrD * 400;
long l;
long d;
long pos = 0;
long tim = 0;
long hitrost = 0;

const int pn = 10;       // IZHOD za pnevmatski cilinder
const int glava = 11;    // VHOD za pnevmatski cilinder v glavi (PULLUP)
const int drzi = 12;     // VHOD za pnevmatski cilinder (PULLUP)
const int start = 13;    // VHOD za start (PULLUP)
const int cilinder = A2; // IZHOD za cilinder v glavi

unsigned long now, timeGlava, timePn;

void setup()
{
  pinMode(Pot, INPUT);
  pinMode(start, INPUT_PULLUP); // Start
  pinMode(drzi, INPUT_PULLUP);  // Stikalo za pn cilinder
  pinMode(glava, INPUT_PULLUP); // Tipka za ročno odpiranje glave

  pinMode(pn, OUTPUT);       // PN cilinder protikonektor
  pinMode(cilinder, OUTPUT); // Cilinder glava

  motor.begin(0x4); // Motor 1 is at address 0x5. Motor 2 on 0x4
  motor.disable();

  lcd.begin(16, 2); // LCD INITIALISATION

  lcd.clear();
  writeToDisplay(true);
}

void loop()
{

  if (meniStatus == 0)
  {
    while (digitalRead(drzi) == LOW)
    {
      digitalWrite(pn, HIGH);
      delay(50);
      stiskPn = true;
    }
    while (digitalRead(glava) == LOW)
    {
      digitalWrite(cilinder, HIGH);
      delay(50);
      stiskGlava = true;
    }
    if (digitalRead(start) == LOW && !stiskStart)
    {
      // stiskStart = true;
      startWorking();
    }
    else if (digitalRead(start) != LOW)
    {
      stiskStart = false;
    }

    if (stiskPn)
    {
      digitalWrite(pn, LOW);
      stiskPn = false;
    }

    if (stiskGlava)
    {
      digitalWrite(cilinder, LOW);
      stiskGlava = false;
    }
  }
  byte button = Keypad.ReadKey();

  if (Keypad.buttonJustPressed)
  {
    switch (button)
    {
    case 1:
    { // 1=RIGHT
      Keypad.buttonJustPressed = true;
      delay(200);
      if (meniStatus > 0)
      {
        tipka = 1;
        meni(tipka);
      }
      break;
    }
    case 4:
    { // 4=LEFT
      Keypad.buttonJustPressed = true;
      delay(200);
      if (meniStatus > 0)
      {
        tipka = 4;
        meni(tipka);
      }
      break;
    }
    case 2:
    { // 2=UP
      Keypad.buttonJustPressed = true;
      delay(200);
      tipka = 2;
      meni(tipka);
      break;
    }
    case 3:
    { // 3=DOWN
      Keypad.buttonJustPressed = true;
      delay(200);
      tipka = 3;
      meni(tipka);
      break;
    }
    case 5:
    { // 5=SELECT
      Keypad.buttonJustPressed = true;
      delay(200);
      tipka = 5;
      meni(tipka);
      break;
    }
    } // end switch

    Keypad.buttonJustPressed = false;
  } // end if

  if (Keypad.buttonJustReleased)
  {
    if (button == BUTTON_NONE)
    {
    }
    Keypad.buttonJustReleased = false;
  }
} // end loop

void startWorking()
{
  isWorking = true;
  writeToDisplay(false);

  korak = 20;
  faktCasa = 40;
  fakt = 1;
  int kor = korak;
  int POTval = 0; // analogRead(Pot);
  int cas = 1;

  levo = stObrL * 400;
  desno = stObrD * 400;

  // zapri luknjo
  digitalWrite(pn, LOW);
  digitalWrite(cilinder, LOW);
  delay(50);

  // vključi motor in nastavi stran (levo/desno)
  motor.enable();
  motor.setDirection(true);
  pos = 0;
  POTval = 6;

  // Obračaj dokler ne pridemo do obrata
  while (pos != levo)
  {
    if (pos < 1050 || levo - pos < 1050)
    {
      if (pos < 20)
      {
        cas = POTval * 50;
      }
      else if (pos < 40)
      {
        cas = POTval * 49;
      }
      else if (pos < 60)
      {
        cas = POTval * 48;
      }
      else if (pos < 80)
      {
        cas = POTval * 47;
      }
      else if (pos < 100)
      {
        cas = POTval * 46;
      }
      else if (pos < 120)
      {
        cas = POTval * 45;
      }
      else if (pos < 140)
      {
        cas = POTval * 44;
      }
      else if (pos < 160)
      {
        cas = POTval * 43;
      }
      else if (pos < 180)
      {
        cas = POTval * 42;
      }
      else if (pos < 200)
      {
        cas = POTval * 41;
      }
      else if (pos < 220)
      {
        cas = POTval * 40;
      }
      else if (pos < 240)
      {
        cas = POTval * 39;
      }
      else if (pos < 260)
      {
        cas = POTval * 38;
      }
      else if (pos < 280)
      {
        cas = POTval * 37;
      }
      else if (pos < 300)
      {
        cas = POTval * 36;
      }
      else if (pos < 320)
      {
        cas = POTval * 35;
      }
      else if (pos < 340)
      {
        cas = POTval * 34;
      }
      else if (pos < 360)
      {
        cas = POTval * 33;
      }
      else if (pos < 380)
      {
        cas = POTval * 32;
      }
      else if (pos < 400)
      {
        cas = POTval * 31;
      }
      else if (pos < 420)
      {
        cas = POTval * 30;
      }
      else if (pos < 440)
      {
        cas = POTval * 29;
      }
      else if (pos < 460)
      {
        cas = POTval * 28;
      }
      else if (pos < 480)
      {
        cas = POTval * 27;
      }
      else if (pos < 500)
      {
        cas = POTval * 26;
      }
      else if (pos < 520)
      {
        cas = POTval * 25;
      }
      else if (pos < 540)
      {
        cas = POTval * 24;
      }
      else if (pos < 560)
      {
        cas = POTval * 23;
      }
      else if (pos < 580)
      {
        cas = POTval * 22;
      }
      else if (pos < 600)
      {
        cas = POTval * 21;
      }
      else if (pos < 620)
      {
        cas = POTval * 20;
      }
      else if (pos < 640)
      {
        cas = POTval * 19;
      }
      else if (pos < 660)
      {
        cas = POTval * 18;
      }
      else if (pos < 680)
      {
        cas = POTval * 17;
      }
      else if (pos < 700)
      {
        cas = POTval * 16;
      }
      else if (pos < 720)
      {
        cas = POTval * 15;
      }
      else if (pos < 740)
      {
        cas = POTval * 14;
      }
      else if (pos < 760)
      {
        cas = POTval * 13;
      }
      else if (pos < 780)
      {
        cas = POTval * 12;
      }
      else if (pos < 800)
      {
        cas = POTval * 11;
      }
      else if (pos < 820)
      {
        cas = POTval * 10;
      }
      else if (pos < 840)
      {
        cas = POTval * 9;
      }
      else if (pos < 860)
      {
        cas = POTval * 8;
      }
      else if (pos < 880)
      {
        cas = POTval * 7;
      }
      else if (pos < 900)
      {
        cas = POTval * 6;
      }
      else if (pos < 920)
      {
        cas = POTval * 5;
      }
      else if (pos < 940)
      {
        cas = POTval * 4;
      }
      else if (pos < 960)
      {
        cas = POTval * 3;
      }
      else if (pos < 980)
      {
        cas = POTval * 2;
      }
      else if (pos < 1000)
      {
        cas = POTval;
      }
      else if (pos < 1020)
      {
        cas = 2;
      }
      else if (levo - pos < 20)
      {
        cas = POTval * 50;
      }
      else if (levo - pos < 40)
      {
        cas = POTval * 49;
      }
      else if (levo - pos < 60)
      {
        cas = POTval * 48;
      }
      else if (levo - pos < 80)
      {
        cas = POTval * 47;
      }
      else if (levo - pos < 100)
      {
        cas = POTval * 46;
      }
      else if (levo - pos < 120)
      {
        cas = POTval * 45;
      }
      else if (levo - pos < 140)
      {
        cas = POTval * 44;
      }
      else if (levo - pos < 160)
      {
        cas = POTval * 43;
      }
      else if (levo - pos < 180)
      {
        cas = POTval * 42;
      }
      else if (levo - pos < 200)
      {
        cas = POTval * 41;
      }
      else if (levo - pos < 220)
      {
        cas = POTval * 40;
      }
      else if (levo - pos < 240)
      {
        cas = POTval * 39;
      }
      else if (levo - pos < 260)
      {
        cas = POTval * 38;
      }
      else if (levo - pos < 280)
      {
        cas = POTval * 37;
      }
      else if (levo - pos < 300)
      {
        cas = POTval * 36;
      }
      else if (levo - pos < 320)
      {
        cas = POTval * 35;
      }
      else if (levo - pos < 340)
      {
        cas = POTval * 34;
      }
      else if (levo - pos < 360)
      {
        cas = POTval * 33;
      }
      else if (levo - pos < 380)
      {
        cas = POTval * 32;
      }
      else if (levo - pos < 400)
      {
        cas = POTval * 31;
      }
      else if (levo - pos < 420)
      {
        cas = POTval * 30;
      }
      else if (levo - pos < 440)
      {
        cas = POTval * 29;
      }
      else if (levo - pos < 460)
      {
        cas = POTval * 28;
      }
      else if (levo - pos < 480)
      {
        cas = POTval * 27;
      }
      else if (levo - pos < 500)
      {
        cas = POTval * 26;
      }
      else if (levo - pos < 520)
      {
        cas = POTval * 25;
      }
      else if (levo - pos < 540)
      {
        cas = POTval * 24;
      }
      else if (levo - pos < 560)
      {
        cas = POTval * 23;
      }
      else if (levo - pos < 580)
      {
        cas = POTval * 22;
      }
      else if (levo - pos < 600)
      {
        cas = POTval * 21;
      }
      else if (levo - pos < 620)
      {
        cas = POTval * 20;
      }
      else if (levo - pos < 640)
      {
        cas = POTval * 19;
      }
      else if (levo - pos < 660)
      {
        cas = POTval * 18;
      }
      else if (levo - pos < 680)
      {
        cas = POTval * 17;
      }
      else if (levo - pos < 700)
      {
        cas = POTval * 16;
      }
      else if (levo - pos < 720)
      {
        cas = POTval * 15;
      }
      else if (levo - pos < 740)
      {
        cas = POTval * 14;
      }
      else if (levo - pos < 760)
      {
        cas = POTval * 13;
      }
      else if (levo - pos < 780)
      {
        cas = POTval * 12;
      }
      else if (levo - pos < 800)
      {
        cas = POTval * 11;
      }
      else if (levo - pos < 820)
      {
        cas = POTval * 10;
      }
      else if (levo - pos < 840)
      {
        cas = POTval * 9;
      }
      else if (levo - pos < 860)
      {
        cas = POTval * 8;
      }
      else if (levo - pos < 880)
      {
        cas = POTval * 7;
      }
      else if (levo - pos < 900)
      {
        cas = POTval * 6;
      }
      else if (levo - pos < 920)
      {
        cas = POTval * 5;
      }
      else if (levo - pos < 940)
      {
        cas = POTval * 4;
      }
      else if (levo - pos < 960)
      {
        cas = POTval * 3;
      }
      else if (levo - pos < 980)
      {
        cas = POTval * 2;
      }
      else if (levo - pos < 1000)
      {
        cas = POTval;
      }
    }
    delayMicroseconds(cas);
    pos += 1;

    // Naredi en step na motorju
    motor.step();
  }

  delay(200);

  motor.setDirection(false);
  cas = POTval;
  pos = 0;

  while (pos != desno)
  {
    if (pos < 1050 || desno - pos < 1050)
    {
      if (pos < 20)
      {
        cas = POTval * 50;
      }
      else if (pos < 40)
      {
        cas = POTval * 49;
      }
      else if (pos < 60)
      {
        cas = POTval * 48;
      }
      else if (pos < 80)
      {
        cas = POTval * 47;
      }
      else if (pos < 100)
      {
        cas = POTval * 46;
      }
      else if (pos < 120)
      {
        cas = POTval * 45;
      }
      else if (pos < 140)
      {
        cas = POTval * 44;
      }
      else if (pos < 160)
      {
        cas = POTval * 43;
      }
      else if (pos < 180)
      {
        cas = POTval * 42;
      }
      else if (pos < 200)
      {
        cas = POTval * 41;
      }
      else if (pos < 220)
      {
        cas = POTval * 40;
      }
      else if (pos < 240)
      {
        cas = POTval * 39;
      }
      else if (pos < 260)
      {
        cas = POTval * 38;
      }
      else if (pos < 280)
      {
        cas = POTval * 37;
      }
      else if (pos < 300)
      {
        cas = POTval * 36;
      }
      else if (pos < 320)
      {
        cas = POTval * 35;
      }
      else if (pos < 340)
      {
        cas = POTval * 34;
      }
      else if (pos < 360)
      {
        cas = POTval * 33;
      }
      else if (pos < 380)
      {
        cas = POTval * 32;
      }
      else if (pos < 400)
      {
        cas = POTval * 31;
      }
      else if (pos < 420)
      {
        cas = POTval * 30;
      }
      else if (pos < 440)
      {
        cas = POTval * 29;
      }
      else if (pos < 460)
      {
        cas = POTval * 28;
      }
      else if (pos < 480)
      {
        cas = POTval * 27;
      }
      else if (pos < 500)
      {
        cas = POTval * 26;
      }
      else if (pos < 520)
      {
        cas = POTval * 25;
      }
      else if (pos < 540)
      {
        cas = POTval * 24;
      }
      else if (pos < 560)
      {
        cas = POTval * 23;
      }
      else if (pos < 580)
      {
        cas = POTval * 22;
      }
      else if (pos < 600)
      {
        cas = POTval * 21;
      }
      else if (pos < 620)
      {
        cas = POTval * 20;
      }
      else if (pos < 640)
      {
        cas = POTval * 19;
      }
      else if (pos < 660)
      {
        cas = POTval * 18;
      }
      else if (pos < 680)
      {
        cas = POTval * 17;
      }
      else if (pos < 700)
      {
        cas = POTval * 16;
      }
      else if (pos < 720)
      {
        cas = POTval * 15;
      }
      else if (pos < 740)
      {
        cas = POTval * 14;
      }
      else if (pos < 760)
      {
        cas = POTval * 13;
      }
      else if (pos < 780)
      {
        cas = POTval * 12;
      }
      else if (pos < 800)
      {
        cas = POTval * 11;
      }
      else if (pos < 820)
      {
        cas = POTval * 10;
      }
      else if (pos < 840)
      {
        cas = POTval * 9;
      }
      else if (pos < 860)
      {
        cas = POTval * 8;
      }
      else if (pos < 880)
      {
        cas = POTval * 7;
      }
      else if (pos < 900)
      {
        cas = POTval * 6;
      }
      else if (pos < 920)
      {
        cas = POTval * 5;
      }
      else if (pos < 940)
      {
        cas = POTval * 4;
      }
      else if (pos < 960)
      {
        cas = POTval * 3;
      }
      else if (pos < 980)
      {
        cas = POTval * 2;
      }
      else if (pos < 1000)
      {
        cas = POTval;
      }
      else if (pos < 1020)
      {
        cas = 2;
      }
      else if (desno - pos < 20)
      {
        cas = POTval * 50;
      }
      else if (desno - pos < 40)
      {
        cas = POTval * 49;
      }
      else if (desno - pos < 60)
      {
        cas = POTval * 48;
      }
      else if (desno - pos < 80)
      {
        cas = POTval * 47;
      }
      else if (desno - pos < 100)
      {
        cas = POTval * 46;
      }
      else if (desno - pos < 120)
      {
        cas = POTval * 45;
      }
      else if (desno - pos < 140)
      {
        cas = POTval * 44;
      }
      else if (desno - pos < 160)
      {
        cas = POTval * 43;
      }
      else if (desno - pos < 180)
      {
        cas = POTval * 42;
      }
      else if (desno - pos < 200)
      {
        cas = POTval * 41;
      }
      else if (desno - pos < 220)
      {
        cas = POTval * 40;
      }
      else if (desno - pos < 240)
      {
        cas = POTval * 39;
      }
      else if (desno - pos < 260)
      {
        cas = POTval * 38;
      }
      else if (desno - pos < 280)
      {
        cas = POTval * 37;
      }
      else if (desno - pos < 300)
      {
        cas = POTval * 36;
      }
      else if (desno - pos < 320)
      {
        cas = POTval * 35;
      }
      else if (desno - pos < 340)
      {
        cas = POTval * 34;
      }
      else if (desno - pos < 360)
      {
        cas = POTval * 33;
      }
      else if (desno - pos < 380)
      {
        cas = POTval * 32;
      }
      else if (desno - pos < 400)
      {
        cas = POTval * 31;
      }
      else if (desno - pos < 420)
      {
        cas = POTval * 30;
      }
      else if (desno - pos < 440)
      {
        cas = POTval * 29;
      }
      else if (desno - pos < 460)
      {
        cas = POTval * 28;
      }
      else if (desno - pos < 480)
      {
        cas = POTval * 27;
      }
      else if (desno - pos < 500)
      {
        cas = POTval * 26;
      }
      else if (desno - pos < 520)
      {
        cas = POTval * 25;
      }
      else if (desno - pos < 540)
      {
        cas = POTval * 24;
      }
      else if (desno - pos < 560)
      {
        cas = POTval * 23;
      }
      else if (desno - pos < 580)
      {
        cas = POTval * 22;
      }
      else if (desno - pos < 600)
      {
        cas = POTval * 21;
      }
      else if (desno - pos < 620)
      {
        cas = POTval * 20;
      }
      else if (desno - pos < 640)
      {
        cas = POTval * 19;
      }
      else if (desno - pos < 660)
      {
        cas = POTval * 18;
      }
      else if (desno - pos < 680)
      {
        cas = POTval * 17;
      }
      else if (desno - pos < 700)
      {
        cas = POTval * 16;
      }
      else if (desno - pos < 720)
      {
        cas = POTval * 15;
      }
      else if (desno - pos < 740)
      {
        cas = POTval * 14;
      }
      else if (desno - pos < 760)
      {
        cas = POTval * 13;
      }
      else if (desno - pos < 780)
      {
        cas = POTval * 12;
      }
      else if (desno - pos < 800)
      {
        cas = POTval * 11;
      }
      else if (desno - pos < 820)
      {
        cas = POTval * 10;
      }
      else if (desno - pos < 840)
      {
        cas = POTval * 9;
      }
      else if (desno - pos < 860)
      {
        cas = POTval * 8;
      }
      else if (desno - pos < 880)
      {
        cas = POTval * 7;
      }
      else if (desno - pos < 900)
      {
        cas = POTval * 6;
      }
      else if (desno - pos < 920)
      {
        cas = POTval * 5;
      }
      else if (desno - pos < 940)
      {
        cas = POTval * 4;
      }
      else if (desno - pos < 960)
      {
        cas = POTval * 3;
      }
      else if (desno - pos < 980)
      {
        cas = POTval * 2;
      }
      else if (desno - pos < 1000)
      {
        cas = POTval;
      }
    }
    delayMicroseconds(cas);
    pos += 1;
    motor.step();
  }
  now = millis();
  timeGlava = now + (zakasnitevGlava * 10);
  timePn = now + (zakasnitevPn * 10);

  if (aIzmetGlava == 1)
  {
    izklGlava = true;
  }
  if (aIzmetPn == 1)
  {
    izklPn = true;
  }

  // odpri luknje in spusti žice
  // (je opcija, ker lahko tudi na roko pritisnemo gumb)
  if (aIzmetGlava == 1 || aIzmetPn == 1)
  { //Če je katerikoli izmet vključen
    while (millis() < timeGlava + 250 || millis() < timePn + 250)
    {
      if (millis() > timeGlava && izklGlava)
      {
        digitalWrite(cilinder, HIGH);
        izklGlava = false;
      }
      if (millis() > timePn && izklPn)
      {
        digitalWrite(pn, HIGH);
        izklPn = false;
      }
      if (millis() > timeGlava + 200)
      {
        digitalWrite(cilinder, LOW);
      }
      if (millis() > timePn + 200)
      {
        digitalWrite(pn, LOW);
      }
    }
  }

  motor.disable();
  stCiklov += 1;
  EEPROM.write(2, (stCiklov & 0xFF));
  EEPROM.write(3, ((stCiklov >> 8) & 0xFF));
  EEPROM.write(4, ((stCiklov >> 16) & 0xFF));
  EEPROM.write(5, ((stCiklov >> 24) & 0xFF));
  isWorking = false;
  writeToDisplay(false);
}

/*
  Pojdi skozi menije in za vsak meni
  določi, kaj naredi določen gumb.
*/
void meni(byte gumb)
{ // 1 = RIGHT ; 2 = UP ; 3 = DOWN ; 4 = LEFT ; 5 = SELECT
  switch (meniStatus)
  {
  case 0:
  { // ko je meniStatus 0
    switch (gumb)
    { //
    case 2:
    { // UP
      meniStatus = 1;
      writeToDisplay(true);
      break;
    } // end case 2 gumb
    case 3:
    { // DOWN
      rst = rset;
      meniStatus = 5;
      writeToDisplay(true);
      break;
    } // end case 3 gumb
    case 5:
    { // SELECT
      startWorking();
      break;
    } // end case 5 gumb
    } // end switch gumb
    break;
  } // end case 0 meniStatus

  case 1:
  { // ko je meniStatus 1
    switch (gumb)
    { //
    case 2:
    { // UP
      meniStatus = 2;
      rst = rset;
      writeToDisplay(true);
      break;
    } // end case 2 gumb
    case 3:
    { // DOWN
      meniStatus = 0;
      writeToDisplay(true);
      break;
    } // end case 3 gumb
    case 5:
    { // SELECT
      meniStatus = 3;
      writeToDisplay(true);
      break;
    } // end case 5 gumb
    } // end switch gumb
    break;
  } // end case 1 meniStatus

  case 2:
  { // ko je meniStatus 2
    switch (gumb)
    { //
    case 2:
    { // UP
      meniStatus = 5;
      writeToDisplay(true);
      break;
    } // end case 2 gumb
    case 3:
    { // DOWN
      meniStatus = 1;
      writeToDisplay(true);
      break;
    } // end case 3 gumb
    case 5:
    { // SELECT
      rst -= 1;
      if (rst < 1)
      {
        rst = rset;
        stCiklov = 0;
        EEPROM.write(2, (stCiklov & 0xFF));
        EEPROM.write(3, ((stCiklov >> 8) & 0xFF));
        EEPROM.write(4, ((stCiklov >> 16) & 0xFF));
        spd = 0;
        meniStatus = 0;
      }
      writeToDisplay(true);
      break;
    } // end case 5 gumb
    } // end switch gumb
    break;
  } // end case 2 meniStatus

  case 3:
  { // meniStatus = 3
    switch (gumb)
    {

    case 1:
    { // case gumb = 1 = RIGHT
      switch (posCursor3)
      {
      case 0:
      { // case posCursor3 = 0
        posCursor3 = 1;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 0
      case 1:
      { // case posCursor3 = 1
        posCursor3 = 2;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 1
      case 2:
      { // case posCursor3 = 2
        posCursor3 = 3;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 2
      case 3:
      { // case posCursor3 = 3
        posCursor3 = 0;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 3
      } // end switch posCursor3
      break;
    } // end of case gumb = 1

    case 2:
    { // case gumb = 2 = UP
      switch (posCursor3)
      { // switch posCursor3

      case 0:
      {                                    // case posCursor3 = 0
        stObrL = calculate(stObrL, 10, 1); // povecaj obrate levo za 10
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 0

      case 1:
      {                                   // case posCursor3 = 1
        stObrL = calculate(stObrL, 1, 1); // povecaj obrate levo za 1
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 1

      case 2:
      {                                    // case posCursor3 = 2
        stObrD = calculate(stObrD, 10, 1); // povecaj obrate desno za 10
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 2

      case 3:
      {                                   // case posCursor3 = 3
        stObrD = calculate(stObrD, 1, 1); // povecaj obrate desno za 1
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 3
      } // end of switch posCursor3
      break;
    } // end of case gumb = 2

    case 3:
    { // case gumb = 3 = DOWN
      switch (posCursor3)
      { // switch posCursor3

      case 0:
      {                                    // case posCursor3 = 0
        stObrL = calculate(stObrL, 10, 0); // pomanjsaj obrate levo za 10
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 0

      case 1:
      {                                   // case posCursor3 = 1
        stObrL = calculate(stObrL, 1, 0); // pomanjsaj obrate levo za 1
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 1

      case 2:
      {                                    // case posCursor3 = 2
        stObrD = calculate(stObrD, 10, 0); // pomanjsaj obrate desno za 10
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 2

      case 3:
      {                                   // case posCursor3 = 3
        stObrD = calculate(stObrD, 1, 0); // pomanjsaj obrate desno za 1
        writeToDisplay(true);
        break;
      } // end of case posCursor3 = 3
      } // end of switch posCursor3
      break;
    } // end of case gumb = 3

    case 4:
    { // case gumb = 4 = LEFT
      switch (posCursor3)
      {

      case 0:
      { // case posCursor3 = 0
        posCursor3 = 3;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 0

      case 1:
      { // case posCursor3 = 1
        posCursor3 = 0;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 1

      case 2:
      { // case posCursor3 = 2
        posCursor3 = 1;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 2

      case 3:
      { // case posCursor3 = 3
        posCursor3 = 2;
        writeToDisplay(true);
        break;
      } // end case posCursor3 = 3
      } // end switch posCursor3
      break;
    } // end of case gumb = 4

    case 5:
    { // case gumb = 5 = SELECT
      EEPROM.write(1, stObrD);
      EEPROM.write(0, stObrL);
      meniStatus = 1;
      writeToDisplay(true);
      break;
    } // end of case gumb = 5
    } // end of switch gumb
    break;
  } // end of case meniStatus = 3

  case 5:
  { // ko je meniStatus 5

    switch (gumb)
    { // switch gumb

    case 2:
    { // UP
      meniStatus = 0;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      rst = rset;
      meniStatus = 2;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT
      meniStatus = 6;
      writeToDisplay(true);
      break;
    } // end case 5 gumb
    } // end switch gumb
    break;
  } // end case 5 meniStatus

  case 6:
  { // ko je meniStatus 6

    switch (gumb)
    { // switch gumb

    case 2:
    { // UP
      meniStatus = 8;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      meniStatus = 7;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT
      meniStatus = 9;
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 6 meniStatus

  case 7:
  { // ko je meniStatus 7
    switch (gumb)
    { //

    case 2:
    { // UP
      meniStatus = 6;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      meniStatus = 12;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT
      meniStatus = 5;
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 7 meniStatus

  case 8:
  { // ko je meniStatus 8

    switch (gumb)
    { // switch gumb

    case 2:
    { // UP BTN
      meniStatus = 11;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN BTN
      meniStatus = 6;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT BTN
      meniStatus = 10;
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 8 meniStatus

  case 9:
  { // meniStatus 9
    switch (gumb)
    {

    case 1:
    { // RIGHT BUTTON
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        posCursor1 = 2;
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        posCursor1 = 0;
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        posCursor1 = 1;
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      writeToDisplay(true);
      break;
    } // end of case 1 gumb

    case 2:
    { // UP BUTTON
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        zakasnitevPn = calculate(zakasnitevPn, 1, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        zakasnitevPn = calculate(zakasnitevPn, 10, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        zakasnitevPn = calculate(zakasnitevPn, 100, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN BUTTON
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        zakasnitevPn = calculate(zakasnitevPn, 1, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        zakasnitevPn = calculate(zakasnitevPn, 10, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        zakasnitevPn = calculate(zakasnitevPn, 100, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 2
      }
      break;
    } // end case 3 gumb

    case 4:
    { // LEFT BUTTON
      switch (posCursor1)
      { // switch posCursor1
      case 0:
      { // case posCursor1 = 0
        posCursor1 = 1;
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        posCursor1 = 2;
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        posCursor1 = 0;
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      writeToDisplay(true);
      break;
    } // end of case 4 gumb

    case 5:
    {                                                 // SELECT
      EEPROM.write(10, (zakasnitevPn & 0xFF));        // sharni vrednost 16ih bitov(int)
      EEPROM.write(11, ((zakasnitevPn >> 8) & 0xFF)); //(EEPROM.read(10) << 0) & 0xFF) + ((EEPROM.read(11) << 8) & 0xFFFF)
      meniStatus = 6;
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 9 meniStatus

  case 10:
  { // meniStatus 10
    switch (gumb)
    { //

    case 1:
    { // RIGHT
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        posCursor1 = 2;
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        posCursor1 = 0;
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        posCursor1 = 1;
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      writeToDisplay(true);
      break;
    } // end of case 1 gumb

    case 2:
    { // UP BUTTON
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        zakasnitevGlava = calculate(zakasnitevGlava, 1, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        zakasnitevGlava = calculate(zakasnitevGlava, 10, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        zakasnitevGlava = calculate(zakasnitevGlava, 100, 1);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      switch (posCursor1)
      { // switch posCursor1

      case 0:
      { // case posCursor1 = 0
        zakasnitevGlava = calculate(zakasnitevGlava, 1, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        zakasnitevGlava = calculate(zakasnitevGlava, 10, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        zakasnitevGlava = calculate(zakasnitevGlava, 100, 0);
        writeToDisplay(true);
        break;
      } // end of case posCursor1 = 2
      }
      break;
    } // end case 3 gumb

    case 4:
    { // LEFT
      switch (posCursor1)
      { // switch posCursor1
      case 0:
      { // case posCursor1 = 0
        posCursor1 = 1;
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        posCursor1 = 2;
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        posCursor1 = 0;
        break;
      } // end of case posCursor1 = 2
      } // end of switch posCursor1
      writeToDisplay(true);
      break;
    } // end of case 4 gumb

    case 5:
    {                                                    // SELECT
      EEPROM.write(12, (zakasnitevGlava & 0xFF));        // sharni vrednost 16ih bitov(int)
      EEPROM.write(13, ((zakasnitevGlava >> 8) & 0xFF)); //(EEPROM.read(10) << 0) & 0xFF) + ((EEPROM.read(11) << 8) & 0xFFFF)
      meniStatus = 8;
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 10 meniStatus

  case 11:
  { // ko je meniStatus 11

    switch (gumb)
    { // switch gumb

    case 2:
    { // UP
      meniStatus = 12;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      meniStatus = 8;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT
      if (aIzmetPn == 1)
      { //Če je aIzmetPn vključen
        aIzmetPn = 0;
      }
      else if (aIzmetPn == 0)
      { //Če je aIzmetPn izključen
        aIzmetPn = 1;
      }
      EEPROM.write(7, aIzmetPn);
      writeToDisplay(true);
      break;
    } // end case 5 gumb
    } // end switch gumb
    break;
  } // end case 11 meniStatus

  case 12:
  { // ko je meniStatus 12

    switch (gumb)
    { // switch gumb

    case 2:
    { // UP
      meniStatus = 7;
      writeToDisplay(true);
      break;
    } // end case 2 gumb

    case 3:
    { // DOWN
      meniStatus = 11;
      writeToDisplay(true);
      break;
    } // end case 3 gumb

    case 5:
    { // SELECT
      if (aIzmetGlava == 1)
      { //Če je aIzmet za Glavo vključen
        aIzmetGlava = 0;
      }
      else if (aIzmetGlava == 0)
      { //Če je aIzmet za Glavo izključen
        aIzmetGlava = 1;
      }
      EEPROM.write(6, aIzmetGlava);
      writeToDisplay(true);
      break;
    } // end case 5 gumb

    } // end switch gumb
    break;
  } // end case 12 meniStatus
  } // end switch meniStatus
}

// piši na zaslon
void writeToDisplay(bool changeDisplay)
{
  if (!changeDisplay)
  {
    if (isWorking)
    {
      lcd.setCursor(0, 0);
      lcd.write("Delovanje   ");
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.write("Pripravljen");
      lcd.setCursor(11, 1);
      zapolni(String(stCiklov), 5);
    }
  }
  else
  {
    lcd.noBlink();
    switch (meniStatus)
    {

    case 0:
    { // meniStatus = 0; default stanje
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Pripravljen");
      lcd.setCursor(0, 1);
      lcd.write("St. ciklov:");
      zapolni(String(stCiklov), 5);
      break;
    } // end case meniStatus 0

    case 1:
    { // meniStatus = 1; nastavitve obratov
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("St.obr.:levo:   ");
      lcd.setCursor(0, 1);
      lcd.write("St.obr.desno:   ");
      lcd.setCursor(14, 0);
      zapolni(String(stObrL), 2);
      lcd.setCursor(14, 1);
      zapolni(String(stObrD), 2);
      break;
    } // end of case meniStatus = 1

    case 2:
    { // meniStatus = 2; reset števcev
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("RESET stevcev?   ");
      lcd.setCursor(0, 1);
      lcd.write("SELECT ");
      lcd.write(String(rst)[0]);
      lcd.write("x");
      break;
    } // end of case meniStatus = 2

    case 3:
    { // case meniStatus = 3; stevilo obratov levo in desno
      lcd.setCursor(14, 0);
      zapolni(String(stObrL), 2);
      lcd.setCursor(14, 1);
      zapolni(String(stObrD), 2);

      // Pravilno pozicioniranje cursorja
      switch (posCursor3)
      {

      case 0:
      { // case poCursor3 = 0
        lcd.setCursor(14, 0);
        break;
      } // end case poCursor3 = 0

      case 1:
      { // case poCursor3 = 1
        lcd.setCursor(15, 0);
        break;
      } // end case poCursor3 = 1

      case 2:
      { // case poCursor3 = 2
        lcd.setCursor(14, 1);
        break;
      } // end case poCursor3 = 2

      case 3:
      { // case poCursor3 = 3
        lcd.setCursor(15, 1);
        break;
      } // end case poCursor3 = 3
      } // end switch posCursor3
      lcd.blink();
      break;
    } // end of case meniStatus = 3

    case 5:
    { // meniStatus = 5
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("NASTAVITVE");
      break;
    } // end case meniStatus 5

    case 6:
    { // meniStatus = 6
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Cas zakasnitve");
      lcd.setCursor(0, 1);
      lcd.write("PN");
      lcd.setCursor(8, 1);
      if (String(zakasnitevPn).length() > 2)
      {
        lcd.write(String(zakasnitevPn)[0]);
      }
      else
      {
        lcd.write("0");
      }
      lcd.write(".");
      zapolni(String(zakasnitevPn % 100), 2);
      lcd.write("sek");
      break;
    } // end case meniStatus 6

    case 7:
    { // meniStatus = 7
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Izhod iz");
      lcd.setCursor(0, 1);
      lcd.write("nastavitev?");
      break;
    } // end case meniStatus 7

    case 8:
    { // meniStatus = 8
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Cas zakasnitve");
      lcd.setCursor(0, 1);
      lcd.write("Glava");
      lcd.setCursor(8, 1);
      if (String(zakasnitevGlava).length() > 2)
      {
        lcd.write(String(zakasnitevGlava)[0]);
      }
      else
      {
        lcd.write("0");
      }
      lcd.write(".");
      zapolni(String(zakasnitevGlava % 100), 2);
      lcd.write("sek");
      break;
    } // end case meniStatus 8

    case 9:
    { // meniStatus = 9

      lcd.setCursor(8, 1);
      if (String(zakasnitevPn).length() > 2)
      {
        lcd.write(String(zakasnitevPn)[0]);
      }
      else
      {
        lcd.write("0");
      }
      lcd.write(".");
      zapolni(String(zakasnitevPn % 100), 2);

      switch (posCursor1)
      { // switch posCursor1
      case 0:
      { // case posCursor1 = 0
        lcd.setCursor(11, 1);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        lcd.setCursor(10, 1);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        lcd.setCursor(8, 1);
        break;
      } // end of case posCursor1 = 2
      }
      lcd.blink();
      break;
    } // end case meniStatus 9

    case 10:
    { // meniStatus = 10

      lcd.setCursor(8, 1);
      if (String(zakasnitevGlava).length() > 2)
      {
        lcd.write(String(zakasnitevGlava)[0]);
      }
      else
      {
        lcd.write("0");
      }
      lcd.write(".");
      zapolni(String(zakasnitevGlava % 100), 2);

      switch (posCursor1)
      { // switch posCursor1
      case 0:
      { // case posCursor1 = 0
        lcd.setCursor(11, 1);
        break;
      } // end of case posCursor1 = 0

      case 1:
      { // case posCursor1 = 1
        lcd.setCursor(10, 1);
        break;
      } // end of case posCursor1 = 1

      case 2:
      { // case posCursor1 = 2
        lcd.setCursor(8, 1);
        break;
      } // end of case posCursor1 = 2
      }
      lcd.blink();
      break;
    } // end case meniStatus 10

    case 11:
    { // meniStatus = 11

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Avt. zakasnitev");
      lcd.setCursor(0, 1);
      lcd.write("PN");
      lcd.setCursor(11, 1);

      if (aIzmetPn == 1)
      {
        lcd.write("Vkl.");
      }
      else if (aIzmetPn == 0)
      {
        lcd.write("Izkl.");
      }
      break;
    } // end case meniStatus 11

    case 12:
    { // meniStatus = 12

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Avt. zakasnitev");
      lcd.setCursor(0, 1);
      lcd.write("Glava");
      lcd.setCursor(11, 1);

      if (aIzmetGlava == 1)
      {
        lcd.write("Vkl.");
      }
      else if (aIzmetGlava == 0)
      {
        lcd.write("Izkl.");
      }
      break;
    } // end case meniStatus 12
    } // end of switch meniStatus
  }
}

int calculate(int stevilo, int stevilo2, byte operacija)
{
  int obd;

  switch (operacija)
  { // switch operacija

  case 0:
  { // case of operacija 0 = ODSTEVAJ
    switch (stevilo2)
    { // switch of stevilo 2

    case 1:
    { // case of stevilo2 = 1
      obd = stevilo % 10;
      stevilo -= obd;
      if (obd == 0)
      {
        obd = 9;
      }
      else
      {
        obd -= 1;
      }
      stevilo += obd;
      break;
    } // end of case of stevilo2 = 1

    case 10:
    { // case of stevilo2 = 10
      obd = (stevilo % 100) / 10;
      stevilo -= obd * 10;
      if (obd == 0)
      {
        obd = 9;
      }
      else
      {
        obd -= 1;
      }
      stevilo += obd * 10;
      break;
    } // end of case of stevilo2 = 10

    case 100:
    { // case of stevilo2 = 100
      obd = (stevilo % 1000) / 100;
      stevilo -= obd * 100;
      if (obd == 0)
      {
        obd = 9;
      }
      else
      {
        obd -= 1;
      }
      stevilo += obd * 100;
      break;
    } // end of case of stevilo2 = 100
    } // end of switch of stevilo 2
    break;
  } // end of case operacija = 0

  case 1:
  { // case of operacija 1 = SESTEVAJ
    switch (stevilo2)
    { // switch of stevilo 2

    case 1:
    { // case of stevilo2 = 1
      obd = stevilo % 10;
      stevilo -= obd;
      if (obd == 9)
      {
        obd = 0;
      }
      else
      {
        obd += 1;
      }
      stevilo += obd;
      break;
    } // end of case of stevilo2 = 1

    case 10:
    { // case of stevilo2 = 10
      obd = (stevilo % 100) / 10;
      stevilo -= obd * 10;
      if (obd == 9)
      {
        obd = 0;
      }
      else
      {
        obd += 1;
      }
      stevilo += obd * 10;
      break;
    } // end of case of stevilo2 = 10

    case 100:
    { // case of stevilo2 = 100
      obd = (stevilo % 1000) / 100;
      stevilo -= obd * 100;
      if (obd == 9)
      {
        obd = 0;
      }
      else
      {
        obd += 1;
      }
      stevilo += obd * 100;
      break;
    } // end of case of stevilo2 = 100
    } // end of switch of stevilo 2
    break;
  } // end of case operacija = 1 SESTEVAJ
  } // end of switch operacija
  return stevilo;
}

// dolzina - max length, ki je lahko
void zapolni(String a, int dolzina)
{
  int len = a.length();
  int razlika = dolzina - len;
  char b = ' ';
  // če je trenutni meni eden izmed spodaj naštetih, dodaj 0, drugače space
  if (meniStatus == 4 || meniStatus == 3 || meniStatus == 9 || meniStatus == 10 || meniStatus == 6 || meniStatus == 8)
  {
    b = '0';
  }
  if (razlika > 0)
  {
    for (int i = 0; i < razlika; i++)
    {
      lcd.write(b);
    }
  }
  for (int i = 0; i < String(a).length(); i++)
  {
    lcd.write(String(a)[i]);
  }
}
