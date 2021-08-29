
#define BATTERYpin 22

#define TOF1pin 2
#define TOF2pin 3
#define TOF3pin 4
#define TOF4pin 5

#define VIBR1pin 6
#define VIBR2pin 7

//motor drivers enable pins
#define DRIVE_EN1_pin 29
#define DRIVE_EN2_pin 30
#define DRIVE_EN3_pin 31
#define DRIVE_EN4_pin 32


void Pinout(){
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN,LOW);

    pinMode(BATTERYpin,INPUT);

    pinMode(TOF1pin,INPUT);
    pinMode(TOF2pin,INPUT);
    pinMode(TOF3pin,INPUT);
    pinMode(TOF4pin,INPUT);

    pinMode(VIBR1pin,INPUT);
    pinMode(VIBR2pin,INPUT);

    pinMode(DRIVE_EN1_pin,OUTPUT);
    pinMode(DRIVE_EN2_pin,OUTPUT);
    pinMode(DRIVE_EN3_pin,OUTPUT);
    pinMode(DRIVE_EN4_pin,OUTPUT);
    digitalWrite(DRIVE_EN1_pin,LOW);
    digitalWrite(DRIVE_EN2_pin,LOW);
    digitalWrite(DRIVE_EN3_pin,LOW);
    digitalWrite(DRIVE_EN4_pin,LOW);

}