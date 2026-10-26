#include "USB.h"
#include "USBMIDI.h"

#include <Arduino.h>

USBMIDI MIDI("Talking drum"); //creates a midi device with the name talking drum

const int PIEZO_PIN = 4;
const int FSR_PIN = 5;
const int PIEZO_THRESHOLD = 10; //Values from this and above are treated as a strike. N.B: The current value is a placeholder and will be changed upon testing 
const uint32_t PEAK_WINDOWMS = 10; //A deadline for how long we want to keep checking to find the strongest reading from the current strike
const uint32_t NOTE_LENGTHMS = 100; //How long a MIDI note stays on
const uint32_t FSR_UPDATEMS = 10;


uint8_t drumNote = 60; //60 is a placeholder value for now
uint8_t midiChannel = 1; //MIDI Channel runs from 1-16 
uint32_t prevStrike = 0; //This is in Milliseconds
uint32_t strikePeriod = 40; //To avoid multiple inputs in one strike, cause of vibrations
uint32_t peakWindowEnd = 0;
uint32_t noteOffAt = 0;
uint32_t lastFSRUpdate = 0;
int highestPiezoValue = 0; //This stores the highest voltage received during the current run
int lastPitchBend = -1; //to avoid sending the same pitch value repeatedly


bool strikePeak = false; // This is to know if a strike peak has been reached within a given time frame
bool noteIsOn = false;

uint8_t GetVelocity(int peak)
{
  int velocity = map(peak, PIEZO_THRESHOLD,4095,1,127); //(highestPiezoValue,PIEZO_THRESHOLD, ESP32-S3 maxium ADC reading, Minimum Midi Velocity, Maximum MIDI Velocity)
  return (uint8_t)constrain(velocity,1,127); //To ensure it doesn't go beyond 1-127 
}

void ProcessPiezo()
{
  int piezoValue = analogRead(PIEZO_PIN);
  const uint32_t now = millis();

  if ((!strikePeak) && piezoValue >= PIEZO_THRESHOLD && now - prevStrike >= strikePeriod)
  {
    strikePeak = true;
    highestPiezoValue = piezoValue;
    prevStrike = now;
    peakWindowEnd = now + PEAK_WINDOWMS; 
  }
  
  if(strikePeak)
  {
    if (piezoValue > highestPiezoValue)
    {
      highestPiezoValue = piezoValue;
    } 

    if((int32_t)(now - peakWindowEnd) >= 0) //check whether the peak window has ended. To avoid negativity
    {
      MIDI.noteOn(drumNote,GetVelocity(highestPiezoValue), midiChannel);
      strikePeak = false;
      noteIsOn = true;
      noteOffAt = now + NOTE_LENGTHMS;
    }
  }
  
  if (noteIsOn  && (int32_t)(now - noteOffAt) >= 0)
  {
    MIDI.noteOff(drumNote,0,midiChannel);
    noteIsOn = false;
  }
}


void ProcessFSR()
{
  const uint32_t now = millis();
  if (now - lastFSRUpdate < FSR_UPDATEMS)
  {
    return;
  }

  lastFSRUpdate = now;
  

  int pitchBendValue = map(fsrValue,0,4095, -8192, 8191);

  if (pitchBendValue != lastPitchBend)
  {
    MIDI.pitchBend((int16_t)pitchBendValue, midiChannel);
    lastPitchBend = pitchBendValue;
  }
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("XIAO Connected");
  MIDI.begin(); //starts the MIDI interface
  USB.begin(); //starts the USB device

  pinMode(PIEZO_PIN, INPUT); //The XIAO receives voltage signal at both pins
  pinMode(FSR_PIN,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  ProcessPiezo();
  ProcessFSR();
  //fsrValue = analogRead(FSR_PIN);
  //Serial.println(fsrValue);
}
