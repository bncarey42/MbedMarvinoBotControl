//This program has been adapted by Ben Carey for CSci 1410-60 at Saint Paul College, Saint Paul Minnesota December of 2015
//  to see this program in action view the following link: https://www.youtube.com/watch?v=sy8xRlEwfgg

#include <mbed.h>
#include <stdlib.h>

Timer t1;
Timer t2;
Timer t3;

DigitalOut myled(LED1);
I2C i2c(p9, p10);
AnalogIn light(p18);
AnalogIn linel(p19);
AnalogIn liner(p20);
PwmOut eyes(p21);
PwmOut motlspd(p22);
PwmOut motrspd(p23);
DigitalOut motldir(p24);
DigitalOut motrdir(p25);
PwmOut speaker(p26);
DigitalIn armr(p27);
DigitalIn armrf(p28);
DigitalIn arml(p29);
DigitalIn armlf(p30);

//srand(time(NULL));                        //seed random number

int r = rand() % 16;                        //set r = random number <= 15
int HatLightAddr = 0x46;                    // PCA8575 16 bit IO
char HatLightData[2];                       // 16 bit data
float PWMFreq = 5000;                       // PWM Frequency in Hz
float PWMPeriod = 1 / PWMFreq;              // On the mbed this is global for all PWM output pins
float MotLeftAdj = 1;                       // Left motor speed adjust
float MotRightAdj = 1;                      // Right motor speed adjust
float vcc = 3.3;                            // mbed supply voltage
float ForwardFast = 1;                      // motor forward full speed
float ForwardNorm = ForwardFast * 0.5;      // forward normal speed
float ForwardSlow = ForwardNorm * 0.5;      // forward half normal speed
float ReverseNorm = 0-ForwardNorm;          // reverse normal speed
float ReverseFast = 0-ForwardFast;          // motor reverse full speed
float ReverseSlow = 0-ForwardSlow;          // reverse half normal speed
float Stop = 0.0;                           // stop
float SensorTimeOut1 = 10;                  // Nothing happening to Arms or Line Following in seconds (change to line following)
float SensorTimeOut2 = 10;                  // Nothing happening to Light Following in seconds (stuck? so reverse turn)
float T3TimeOut = 25;
int LightChange = 0;                        // used to determine change in light following
int LineFollow = 0;                         // used to determine continous run of line following
int LineBright = 1;                         // voltage that determines line brightness

// MotorLeft expects -1 to 1 float for speed and direction
void MotorLeft(float sp) {
    if (sp > 0) {
        motldir = 1;
    }
    else {
        motldir = 0;
        sp = 0 - sp;
    }
    motlspd.pulsewidth(sp * PWMPeriod * MotLeftAdj);   // left motor a bit fast so slow it down
}

// MotorRight expects -1 to 1 float for speed and direction
void MotorRight(float sp) {
    if (sp > 0) {
        motrdir = 1;
    }
    else {
        motrdir = 0;
        sp = 0 - sp;
    }
    motrspd.pulsewidth(sp * PWMPeriod * MotRightAdj);
}

// EyesBright expects 0 to 1 float for brrightness
void EyesBright(float sp) {
    eyes.pulsewidth(sp * PWMPeriod);
}

/*/buzzer out
void SpeakerOut() {
    speaker.period(PWMPeriod);
    speaker = 0.5;
    speaker.period(PWMPeriod * .3);
    speaker.period(PWMPeriod * 3);
}
*/
void SpeakerOut(float f, float p) {
    float t = 0.5/f;
    for (int i=0; i<(f*p); i++) {
        speaker = 1;                        // speaker high
        wait(t);
        speaker = 0;                        // speaker low
        wait(t);
    } 
}
// FlashEyes flashes eyes n times at period p in seconds
void FlashEyes(int n, float p) {
    for (int i=0; i<n; i++) {
        EyesBright(1);                      // eyes full on
        wait(p/2);
        EyesBright(0);                      // eyes off
        wait(p/2);
    }
    EyesBright(0.5);                        // back to half bright
}

void spin(){                   
    MotorLeft(ForwardFast);
    MotorRight(ReverseFast);
    wait(0.5);
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm); 
}


void RandSpin(){                   
    MotorLeft(ForwardFast);
    MotorRight(ReverseFast);
    wait(1.5);
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm);
    t3.reset();
}


void AvoidLeft() {
    MotorLeft(ReverseSlow + .05);
    MotorRight(ReverseFast);                // reverse turn back
    FlashEyes(3, 0.1);
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm);                // forward
}

void AvoidRight() {
    MotorLeft(ReverseFast);
    MotorRight(ReverseSlow + .05);                // reverse turn back
    FlashEyes(3, 0.1);
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm);                // forward
}   

void AvoidLeftFront() {
    MotorLeft(ReverseFast);
    MotorRight(ReverseNorm);                // reverse
    FlashEyes(6, 0.1);                      // flash eyes
    MotorRight(Stop);
    FlashEyes(6, 0.1);                      // rotate
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm);                // forward
}     

void AvoidRightFront() {
    FlashEyes(6, 0.1);                      // flash eyes
    FlashEyes(6, 0.1);                      // flash eyes
    MotorLeft(ForwardNorm);
    MotorRight(ForwardNorm);                // forward
}         

void AvoidHeadOn() {
    SpeakerOut(1000, 0.01);
    MotorLeft(ReverseFast);
    MotorRight(ReverseFast);                // reverse
    FlashEyes(8, 0.1);                      // flash eyes
    MotorLeft(ForwardFast);
    MotorRight(ReverseFast);
    spin();                                //spin
    FlashEyes(8, 0.1);                     
    MotorRight(ForwardNorm);
    MotorLeft(ForwardNorm);                // carry on straight
}

void NudgeLeft() {
    MotorLeft(Stop);                        // stop left motor
    wait(0.01);                             // wait a bit
    MotorLeft(ForwardNorm);                 // carry on
}     

void NudgeRight() {
    MotorRight(Stop);                       // stop right motor
    wait(0.01);                             // wait a bit
    MotorRight(ForwardNorm);                // carry on
}
void T3Oreset() {
    spin();
    t3.reset();
}   

int main() {
    motlspd.period(PWMPeriod);
    motrspd.period(PWMPeriod);
    eyes.period(PWMPeriod);
    EyesBright(0.5);                        // eyes half brightness
    MotorLeft(ForwardNorm);                 // motor left forward
    MotorRight(ForwardNorm);                // motor right forward
    t1.start();
    t3.start();
   
    spin();                                 //spin on start
    
    while(1) { 
        if (t3.read() >= T3TimeOut){
            T3Oreset();
        } 
        if (arml == 0 && armr != 0) {
            t1.reset();
            AvoidLeft();
        }
        if (arml != 0 && armr == 0) {
            t1.reset();
            AvoidRight();
        }
        if (arml == 0 && armr == 0) {
            t1.reset();
            AvoidHeadOn();    
        }
        if (armlf == 0 && armrf == 0) {
            t1.reset();
            AvoidHeadOn();
        }
        if (armlf == 0 && armrf != 0) {
            t1.reset();
            AvoidLeftFront();
        }
        if (armlf != 0 && armrf == 0) {
            t1.reset();
            AvoidRightFront();
        }
        if ((linel > (LineBright/vcc)) && (liner < (LineBright/vcc))) {
            NudgeLeft();
        }
        if ((linel < (LineBright/vcc)) && (liner > (LineBright/vcc))) {
            NudgeRight();
            }
        if (((linel < (LineBright/vcc)) && (liner < (LineBright/vcc))) || ((linel > (LineBright/vcc)) && (liner > (LineBright/vcc)))) {
            LineFollow = 0;
            }
        else {
            LineFollow++;
            if (LineFollow > 10) {
                t1.reset();                 // only reset activity timer if consistant line following
            }
        }
        if (t1.read() > SensorTimeOut1) {   // if nothing else is happening then follow light
              t2.start();
            if (light > 0.7) {              // more light to the left
            if (LightChange == 0) {
                LightChange = 1;
                t2.reset();
                SpeakerOut(1000, 0.01);
                NudgeLeft();
            }
            if (light < 0.3) {              // more light to the right
                if (LightChange == 1) {
                    LightChange = 0;
                    t2.reset();
                    SpeakerOut(1000, 0.01);
                }
                NudgeRight();
            }
        }
        else {
            t2.reset();
            t2.stop();
        }
        if (t2.read() > SensorTimeOut2) {   // probably stuck
            AvoidHeadOn();
            t1.reset();
            t2.reset();
            t2.stop();
        }
    }
}
}
