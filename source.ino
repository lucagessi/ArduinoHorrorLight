
char S = 0;
volatile int int_times = 0;
char last_pin_state;
char pin_state;
int period_times = 100;
int duty_cycle = 30;
unsigned long start_alarm = 0;
unsigned long start_pl = 0;
unsigned long start_mod = 0;
unsigned long start_pulse = 0;

int seconds = 0;  //conteggio secondi...
const unsigned long alarm_period = 5*1000000U; 
const unsigned long micro_pl_period = 10000; //periodo power line 10ms in microsecondi
const int mod_period = 40; //step in cui è diviso l'intero periodo della onda. utile per modulazione
const unsigned long debug_period = 10000U;
const int mod_step = mod_period/40; // step utile per debug, incremento dutycycle
const unsigned long micro_pl_tick = micro_pl_period/mod_period; 
unsigned long duty_tick = micro_pl_period - micro_pl_tick * duty_cycle;

int pout_state = LOW; //livello uscita comandoi gate: HIGH o LOW

typedef struct flickFuns_t {
    void (*flickPt)(void);
    void (*flickInitPt)(void);
} flickFuns_t;

typedef struct flickUtils_t {
    const short flickerProbability = 3;//Probabilità di flick in parti percentuali
    flickFuns_t flickFuns;
    void (*periodManagePt)(void);
    unsigned const long meanMainPeriod = 5*1000000U; //valore medio di main_period è 5 secondi (5*10^6 microsecondi)
    unsigned long mainPeriod;
    unsigned long startMainPeriod;
    bool isFlickering;
    
} flickUtils_t;

typedef struct defaultFlick_t {
  unsigned long  defaultFlickPeriod = 4000000U;
  unsigned long  flickDCUpdate = 100000U; //10 update al secondo
  short flickTimes;
  short flickIndex;
  short flickStep;
  int flickMax = 10;
  
} defaultFlick_t;

flickUtils_t flickUV;
defaultFlick_t defaultFlickV;

#define PIN 2
#define POUT 3

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  //Serial.begin(115200,SERIAL_8N1);
 
  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(POUT, OUTPUT);
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  pinMode(PIN, INPUT_PULLUP);     //set the pin to input
  flickUV.startMainPeriod = micros();
  duty_tick = 0;
  duty_cycle = mod_period;
  //duty_tick = micro_pl_period;
  start_mod = micros();
  flickUV.periodManagePt = randomPeriodManage;
  flickUV.flickFuns.flickPt = defaultFlick;
  flickUV.flickFuns.flickInitPt = defaultFlickInit;
  flickUV.mainPeriod = flickUV.meanMainPeriod;
  randomSeed(analogRead(0));
}

// the loop function runs over and over again forever
void loop() {
  //Controllo periodo linea elettrica tramite ingresso zero crossing detector
  if(digitalRead(PIN))
  {
    //modulate() è bloccante. Ritorna dopo che è stato triggerato il gate del triac
    modulate();
  }
   
  if( flickUV.isFlickering )
  {
    flickUV.flickFuns.flickPt();
  }else 
  {
    if ( ( micros()-flickUV.startMainPeriod ) > flickUV.mainPeriod )
    {
      flickUV.periodManagePt();
    }
  }
}

void randomPeriodManage(void)
{
  flickUV.startMainPeriod = micros();
  //duty_cycle = (mod_period/4 + random(-mod_period/2,mod_period/2));
  //duty_tick = micro_pl_period - (micro_pl_tick * duty_cycle);
  if (random(0,100) < flickUV.flickerProbability)
  {
    flickUV.flickFuns.flickInitPt();
    flickUV.isFlickering = true;
  }
}

void dummyFun()
{
  return;
}

void constPeriodManage(void)
{
 flickUV.startMainPeriod = micros();
 flickUV.flickFuns.flickInitPt();
 flickUV.isFlickering = true;
}

void defaultFlick(void)
{
  short temp;
  if ( ( micros()-flickUV.startMainPeriod ) > defaultFlickV.defaultFlickPeriod )
    {
      duty_cycle = duty_cycle + defaultFlickV.flickStep;
      if(duty_cycle <= 0)
      {
        duty_cycle = 0;
        defaultFlickV.defaultFlickPeriod = 5000*random(1,10);
        defaultFlickV.flickStep = -defaultFlickV.flickStep;
      }else
      {
        if(duty_cycle >= mod_period)
        {
          duty_cycle = mod_period;
          defaultFlickV.flickIndex++;
          defaultFlickV.flickStep = -defaultFlickV.flickStep;
          if(defaultFlickV.flickTimes == defaultFlickV.flickIndex)
          {
            flickUV.isFlickering = false;
            temp = duty_cycle;
          }
        }
      }
      //Sovrappongo rumore
      if(flickUV.isFlickering)
      {
        temp = duty_cycle + random(-5,5);
      }else
      {
        temp = duty_cycle;
      }
      duty_tick = micro_pl_period - (micro_pl_tick * temp);
      flickUV.startMainPeriod = micros();
    }else
    {
        //duty_tick = micro_pl_period;
    }
}

void defaultFlickInit(void)
{
  defaultFlickV.flickTimes = random(4,9);
  defaultFlickV.flickIndex = 0;
  defaultFlickV.defaultFlickPeriod = 5000*random(1,3);
  defaultFlickV.flickStep = -2;
}

/*
 * Funzione che gestisce la modulazione di fase del carico
 * Serve per, appunto, comandare il triac
 */
void modulate()
{
  start_pl = micros();
   //Aspetto che sia trascorso sufficiente tempo per attivare triac
  while( ( micros()-start_pl ) < ( 8600 )) //penso che questo valore sia per limitare problemi con il crossing successivo
  { 
    if(( micros()-start_pl ) > ( duty_tick )) digitalWrite(POUT, HIGH);
    digitalWrite(POUT, LOW);    
  }
  
 
}
