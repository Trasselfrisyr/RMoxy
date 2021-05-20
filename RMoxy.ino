/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Minipops drummer firmware for Music Thing Modular Radio Music     //
//   by Johan Berglund, May 2021                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// WAV files converted to code by wav2sketch
// Minipops 7 samples downloaded from http://samples.kb6.de 

#include "AudioSampleBd808w.h"
#include "AudioSampleBongo2mp7w.h"
#include "AudioSampleClavemp7w.h"
#include "AudioSampleCowmp7w.h"
#include "AudioSampleCymbal1mp7w.h"
#include "AudioSampleGuiramp7w.h"
#include "AudioSampleMaracasmp7w.h"
#include "AudioSampleQuijadamp7w.h"

#include "patterns.h"        // Patterns for rhythm section


/*
 RADIO MUSIC is the intended hardware for this firmware
 https://github.com/TomWhitwell/RadioMusic
 
 Audio out: Onboard DAC, teensy3.1 pin A14/DAC
 
 Bank Button: 2
 Bank LEDs 3,4,5,6
 Reset Button: 8  
 Reset LED 11 
 Reset CV input: 9 
 Channel Pot: A9 
 Channel CV: A8 // check 
 Time Pot: A7 
 Time CV: A6 // check 
 SD Card Connections: 
 SCLK 14
 MISO 12
 MOSI 7 
 SS   10 
 */
#define LED0 6
#define LED1 5
#define LED2 4
#define LED3 3
#define RESET_LED 11      // Reset LED indicator
#define CHAN_POT_PIN A9   // pin for Channel pot -- RMoxy Pattern
#define CHAN_CV_PIN 20    // pin for Channel CV -- RMoxy Reset CLK (20/A6)
#define TIME_POT_PIN A7   // pin for Time pot -- RMoxy Tempo (full CCW for external CLK)
#define TIME_CV_PIN A8    // pin for Time CV -- RMoxy binary muting, 0V on input or unconnected = nothing muted
#define RESET_BUTTON 8    // Reset button -- RMoxy RUN/SET
#define RESET_CV 9        // Reset pulse input -- RMoxy CLK

#define ADC_BITS 13
#define ADC_MAX_VALUE (1 << ADC_BITS)

#define TEMPO_THR 200
#define MUTING_MARGIN 100

// GUItool: begin automatically generated code
AudioPlayMemory          playMem1;       //xy=247,53
AudioPlayMemory          playMem2;       //xy=248,90
AudioPlayMemory          playMem3;       //xy=249,125
AudioPlayMemory          playMem4;       //xy=251,161
AudioPlayMemory          playMem5;       //xy=252,194
AudioPlayMemory          playMem6;       //xy=254,227
AudioPlayMemory          playMem7;       //xy=257,260
AudioPlayMemory          playMem8;       //xy=259,293
AudioMixer4              mixer1;         //xy=517,133
AudioMixer4              mixer2;         //xy=557,235
AudioMixer4              mixer3;         //xy=707,178
AudioOutputAnalog        dac1;           //xy=883,177
AudioConnection          patchCord1(playMem1, 0, mixer1, 0);
AudioConnection          patchCord2(playMem2, 0, mixer1, 1);
AudioConnection          patchCord3(playMem3, 0, mixer1, 2);
AudioConnection          patchCord4(playMem4, 0, mixer1, 3);
AudioConnection          patchCord5(playMem5, 0, mixer2, 0);
AudioConnection          patchCord6(playMem6, 0, mixer2, 1);
AudioConnection          patchCord7(playMem7, 0, mixer2, 2);
AudioConnection          patchCord8(playMem8, 0, mixer2, 3);
AudioConnection          patchCord9(mixer1, 0, mixer3, 0);
AudioConnection          patchCord10(mixer2, 0, mixer3, 1);
AudioConnection          patchCord11(mixer3, dac1);
// GUItool: end automatically generated code

// Pointers
AudioPlayMemory*            rtm[8]    {&playMem1,&playMem2,&playMem3,&playMem4,&playMem5,&playMem6,&playMem7,&playMem8};

int currentStep = 0;        // pattern step
int patNum = 0;             // selected rhythm pattern
int tempoRead = 0;
int muteRead = 0;
bool runPress = 0;
bool runPressRead = 0;
bool runPressDebounce = 0;
bool runPressLast = 0;
bool runStatus = 0;
bool clkNow = 0;
bool clkLast = 0;
bool externalClk = 0;
bool resetRead = 0;
bool resetLast = 0;

unsigned long currentMillis = 0L;
unsigned long statusPreviousMillis = 0L;
unsigned long stepTimerMillis = 0L;
unsigned long stepInterval = 150;   // step interval in ms, one step is a 1/16 note http://www.dvfugit.com/beats-per-minute-millisecond-delay-calculator.php
unsigned long debounceTimerMillis = 0L;
unsigned long debounceTime = 10;

void setup() {
  // put your setup code here, to run once:
  AudioMemory(50);
  dac1.analogReference(INTERNAL);   // normal volume
  //dac1.analogReference(EXTERNAL); // louder
  mixer1.gain(0, 0.27);
  mixer1.gain(1, 0.27);
  mixer1.gain(2, 0.27);
  mixer1.gain(3, 0.27);
  mixer2.gain(0, 0.27);
  mixer2.gain(1, 0.27);
  mixer2.gain(2, 0.27);
  mixer2.gain(3, 0.27);
  mixer3.gain(0, 0.5);
  mixer3.gain(1, 0.5);
  
  analogReadRes(ADC_BITS);
  
  pinMode(RESET_BUTTON, INPUT);
  pinMode(RESET_CV, INPUT);
  pinMode(CHAN_CV_PIN, INPUT);
  pinMode(RESET_LED, OUTPUT);
  pinMode(LED0,OUTPUT);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  
}

void loop() {
  currentMillis = millis();
  patNum = map(analogRead(CHAN_POT_PIN), 0, ADC_MAX_VALUE, 0, 15);
  tempoRead = analogRead(TIME_POT_PIN);
  runPressRead = digitalReadFast(RESET_BUTTON);
  if (runPressRead != runPressDebounce) debounceTimerMillis = currentMillis;
  if ((currentMillis - debounceTimerMillis) > debounceTime) {
    runPress = runPressRead;
  }
  runPressDebounce = runPressRead;
  muteRead = constrain(map(analogRead(TIME_CV_PIN), MUTING_MARGIN, ADC_MAX_VALUE - MUTING_MARGIN, 255, 0), 0, 255);
  if (tempoRead < TEMPO_THR) {
    externalClk = 1;
  } else {
    externalClk = 0;
    stepInterval = map(tempoRead, ADC_MAX_VALUE, TEMPO_THR, 50, 300);
  }
  resetRead = digitalReadFast(CHAN_CV_PIN);
  if (resetRead && !resetLast) { // if reset is going high, go to step 0
    currentStep = 0;
    digitalWrite(LED0, bitRead(currentStep, 0));
    digitalWrite(LED1, bitRead(currentStep, 1));
    digitalWrite(LED2, bitRead(currentStep, 2));
    digitalWrite(LED3, bitRead(currentStep, 3));
  }
  resetLast = resetRead;
  clkNow = digitalReadFast(RESET_CV);
  if (runPress && !runPressLast) { // start/stop
    if (runStatus){
      runStatus = 0;
      currentStep = 0;
      digitalWrite(LED0, 0);
      digitalWrite(LED1, 0);
      digitalWrite(LED2, 0);
      digitalWrite(LED3, 0);
    } else {
        runStatus = 1;    
    }
    digitalWrite(RESET_LED, runStatus); // LED by button lit when in RUN mode
    statusPreviousMillis = currentMillis; // reset interval timing for internal clock
  }
  if (runStatus) {
    if ((externalClk && clkNow && !clkLast) || (!externalClk && ((unsigned long)(currentMillis - stepTimerMillis) >= stepInterval))){ // ext CLK rising or internal clock timer reach
      for (int i = 0; i < 8; i++){
        if (bitRead(pattern[patNum][currentStep],7-i) && bitRead(muteRead, i)) playRtm(i);
      }
      digitalWrite(LED0, bitRead(currentStep, 0));
      digitalWrite(LED1, bitRead(currentStep, 1));
      digitalWrite(LED2, bitRead(currentStep, 2));
      digitalWrite(LED3, bitRead(currentStep, 3));
      currentStep++;
      stepTimerMillis = currentMillis; // reset interval timing for internal clock
    }
    clkLast = clkNow;
    resetRead = digitalReadFast(CHAN_CV_PIN);
    if ((resetRead && !resetLast) || (currentStep == 16) || pattern[patNum][currentStep] == 255) currentStep = 0; // start over if we are at step 0 if we passed 15 or next step pattern value is 255 (reset)
    resetLast = resetRead;
  }
  runPressLast = runPress;
}

// play rhythm samples
void playRtm(int i){
  switch(i){
    case 0:
      rtm[i]->play(AudioSampleGuiramp7w); //GU
      break;
    case 1:
      rtm[i]->play(AudioSampleBongo2mp7w); //BG2
      break;
    case 2:
      rtm[i]->play(AudioSampleBd808w); // BD
      break;
    case 3:
      rtm[i]->play(AudioSampleClavemp7w); // CL
      break;
    case 4:
      rtm[i]->play(AudioSampleCowmp7w); // CW
      break;     
    case 5:
      rtm[i]->play(AudioSampleMaracasmp7w); // MA
      break;  
    case 6:
      rtm[i]->play(AudioSampleCymbal1mp7w); // CY
      break;    
    case 7:
      rtm[i]->play(AudioSampleQuijadamp7w); // QU
      break;           
  }
}
