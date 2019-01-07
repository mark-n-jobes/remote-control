///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pins
boolean BurstPortB = true; // Makes DigPin8-11 all burst
#define IRledPin 8
#define IRDet_PINS PIND
#define IRDetDataPin 2
#define IRDetVccPin 3
#define IRDetGndPin 4
// Detection
#define ONs 1
#define OFFs 0
#define PulseTimeout 65000 // 16bit
#define SampleDelay_us 20
#define MaxPulses  192
uint16_t pulses[MaxPulses][2];
uint8_t currentSampleIdx = 0;
// FSM
#define numBuff 10 //2hexDig*4num+2_'s;
char FSM[numBuff] = {0};
// Flags
boolean HitMax      = false;
boolean Xmit_Code   = false;
boolean CaptureEnb  = false;
boolean DidDummyCap = false;
// Code to send
#define numCode 4 // 32bits/8bits = 4 (compressed)
uint8_t CodeToSend[numCode] = {0};
uint8_t CodeCaptured[numCode*2] = {0}; // 32bits/4bits = 8 (ASCII rep)
// Pulse Timming
uint16_t HeaderBlinkOn      = 4500;
uint16_t HeaderBlinkOff     = 4300;
uint16_t TimeBlinkOn        = 600;
uint16_t TimeBlinkOffTrue   = 1600;
uint16_t TimeBlinkOffFalse  = 500;
uint16_t FooterBlinkOn      = 600;
uint16_t FooterBlinkOff_ms  = 45; // Note this is ms, not us
uint16_t DetectionFuzzyness = 10 * SampleDelay_us;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
    // Leds
    if(BurstPortB) DDRB |= B1111;
    else           pinMode(IRledPin,OUTPUT);
    pinMode(IRledPin-1,OUTPUT); digitalWrite(IRledPin-1,LOW); // Used as a gnd for LED
    // Detector
    pinMode(IRDetVccPin, OUTPUT); digitalWrite(IRDetVccPin,HIGH);
    pinMode(IRDetGndPin, OUTPUT); digitalWrite(IRDetGndPin,LOW);
    // Serial
    Serial.begin(115200);
    Serial.println("Remote Control Turned On");
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
    pollSerial(); // By polling the Serial, we can avoid all that ISR dissabling stuff
    if(CaptureEnb) CapturePulses();
    if(Xmit_Code) Xmit_TVCode(CodeToSend,numCode,HeaderBlinkOn,HeaderBlinkOff,TimeBlinkOn,
                              TimeBlinkOffTrue,TimeBlinkOffFalse,FooterBlinkOn,FooterBlinkOff_ms);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pollSerial() {
    if(Serial.available() > 0) {
        char tempc = Serial.read();
        if((tempc == '_')||(tempc == '.')||(tempc == 't')||((tempc >= 'A')&&(tempc <= 'F'))||((tempc >= '0')&&(tempc <= '9'))) CrunchFSM(tempc);
        else if(tempc == '+') {
            CaptureEnb = !CaptureEnb;
            Serial.print("Capture is ");
            Serial.println((CaptureEnb)? "ON " : "OFF");
            DidDummyCap = !CaptureEnb; // So if starting new, DidDummyCap is initialized
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CrunchFSM(char newChar) {
    if(newChar != 0) {
        for(int i=numBuff-1;i>=0;i--) {
            if(i==0) FSM[i] = newChar;
            else     FSM[i] = FSM[i-1];
        }
        if((FSM[0]==FSM[numBuff-1])&&(FSM[numBuff-1]=='_')) {
            for(int i=0;i<numCode;i++) CodeToSend[i] = concatHex(FSM[numBuff-1-(2*i+1)],FSM[numBuff-1-(2*i+1+1)]);
            Xmit_Code = true;
            FSM[0] = {0};
        }
        else if((FSM[0]==FSM[numBuff-1])&&(FSM[numBuff-1]=='.')) {
            HeaderBlinkOn      = concatHex(FSM[numBuff-1-(2*0+1)],FSM[numBuff-1-(2*0+1+1)])*100;
            HeaderBlinkOff     = concatHex(FSM[numBuff-1-(2*1+1)],FSM[numBuff-1-(2*1+1+1)])*100;
            FooterBlinkOn      = concatHex(FSM[numBuff-1-(2*2+1)],FSM[numBuff-1-(2*2+1+1)])*10;
            FooterBlinkOff_ms  = concatHex(FSM[numBuff-1-(2*3+1)],FSM[numBuff-1-(2*3+1+1)]);
            Serial.println("Overwrote Header and Footer info to: ");
            Serial.print("HeaderBlinkOn     = ");Serial.println(HeaderBlinkOn    );
            Serial.print("HeaderBlinkOff    = ");Serial.println(HeaderBlinkOff   );
            Serial.print("FooterBlinkOn     = ");Serial.println(FooterBlinkOn    );
            Serial.print("FooterBlinkOff_ms = ");Serial.println(FooterBlinkOff_ms);
            FSM[0] = {0};
        }
        else if((FSM[0]==FSM[numBuff-1])&&(FSM[numBuff-1]=='t')) {
            TimeBlinkOn       = concatHex(FSM[numBuff-1-(2*0+1)],FSM[numBuff-1-(2*0+1+1)])*10;
            TimeBlinkOffTrue  = concatHex(FSM[numBuff-1-(2*1+1)],FSM[numBuff-1-(2*1+1+1)])*100;
            TimeBlinkOffFalse = concatHex(FSM[numBuff-1-(2*2+1)],FSM[numBuff-1-(2*2+1+1)])*10;
            Serial.println("Overwrote Timing info to: ");
            Serial.print("TimeBlinkOn       = ");Serial.println(TimeBlinkOn      );
            Serial.print("TimeBlinkOffTrue  = ");Serial.println(TimeBlinkOffTrue );
            Serial.print("TimeBlinkOffFalse = ");Serial.println(TimeBlinkOffFalse);
            FSM[0] = {0};
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int concatHex(char MSD, char LSD) {return ((charToDec(MSD)<<4)+charToDec(LSD));}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int charToDec(char In) { // Assumes [0-9A-F]
    if(In >= 'A') return ((In-'A')+10);
    else          return  (In-'0');
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char decToChar(int In) { // Assumes [0-9A-F]
    if(In >= 10) return ((In+'A')-10);
    else         return  (In+'0');
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Xmit_TVCode(uint8_t CodeIn[], uint8_t CodeLen, uint16_t HeaderOn, uint16_t HeaderOff, uint16_t TimeOn, 
                 uint16_t TimeOffTrue, uint16_t TimeOffFalse, uint16_t FooterOn, uint16_t FooterOff_ms) {
    // Header
    pulseIR_38KHz(HeaderOn);
    delayMicroseconds(HeaderOff);
    // Body
    sendCharArray(CodeIn,CodeLen,TimeOn,TimeOffTrue,TimeOffFalse);
    // Footer
    pulseIR_38KHz(FooterOn);
    delay(FooterOff_ms);
    // Handshake
    Xmit_Code = false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendCharArray(uint8_t CharArray[], uint8_t Cnt, uint16_t PulseOn, uint16_t DelayTrue, uint16_t DelayFalse) {
    uint8_t tempc = 0;
    for(int i=0;i < Cnt;i++) {
        tempc = CharArray[i];
        for(int j=7;j>=0;j--) {
            pulseIR_38KHz(PulseOn);
            if (((tempc>>j)%2) == 1) delayMicroseconds(DelayTrue);
            else                     delayMicroseconds(DelayFalse);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pulseIR_38KHz(long microsecs) {
    if(BurstPortB) {
        while (microsecs > 0) {
            // 38KHz = 26.316us -> 26us, so cut in 1/2 for IR pulse -> 13us high, 13us low
            PORTB = B1111;  // Takes ~1/2us
            delayMicroseconds(13);
            PORTB = B0000;  // Takes ~1/2us
            delayMicroseconds(12);
            microsecs -= 26;
        }
    } else {
        uint16_t pinID = IRledPin;
        while (microsecs > 0) {
            // 38KHz = 26.316us -> 26us, so cut in 1/2 for IR pulse -> 13us high, 13us low
            digitalWrite(pinID, HIGH);  // Takes ~3us
            delayMicroseconds(10);
            digitalWrite(pinID, LOW);   // Takes ~3us
            delayMicroseconds(10);
            microsecs -= 26;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CapturePulses(void) {
    uint16_t pulseTimeCnt;
    // Count 38KHz time durration (so pin is LOW (active LOW))
    pulseTimeCnt = 0;
    while (! (IRDet_PINS & (1 << IRDetDataPin))) { // pin is still LOW
        delayMicroseconds(SampleDelay_us);
        // Timeout test for print
        pulseTimeCnt++;
        if (pulseTimeCnt >= PulseTimeout) {
            if(currentSampleIdx != 0) EchoPulseTimes();
            currentSampleIdx=0;
            // break;
            return;
        }
    }
    pulses[currentSampleIdx][ONs] = pulseTimeCnt; // Note the pin is active LOW
    // Count time w/o 38KHz (so pin is HIGH (active LOW))
    pulseTimeCnt = 0;
    while (IRDet_PINS & (1 << IRDetDataPin)) { // vout pin is still HIGH
        delayMicroseconds(SampleDelay_us);
        // Timeout test for print
        pulseTimeCnt++;
        if (pulseTimeCnt >= PulseTimeout) {
            if(currentSampleIdx != 0) EchoPulseTimes();
            currentSampleIdx=0;
            // break;
            return;
        }
    }
    pulses[currentSampleIdx][OFFs] = pulseTimeCnt; // Note the pin is active LOW
    // Increment Sample index
    currentSampleIdx++;
    if(currentSampleIdx >= MaxPulses) {
        HitMax = true;
        Serial.println("Error: Buffer Overflow Occured, here is what was captured");
        EchoPulseTimes();
        currentSampleIdx = 0;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean areFuzzyEqual(uint16_t ValA, uint16_t ValB, uint16_t Fuzzy) {
    if(ValA>ValB) { return ((ValA-ValB) <= Fuzzy); }
    else          { return ((ValB-ValA) <= Fuzzy); }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean ProcessPulseTimes(void) {
    int PulseCnt = MaxPulses;//(HitMax)? MaxPulses : currentSampleIdx;
    uint8_t TmpCode[numCode*2] = {0};
    boolean Valid = true; // Only over-write if works perfectly
    // Assumes a 32bit word wrapped with header and footer (timming is the same as global vars)
    boolean SearchENB = true;
    uint8_t i=0;
    while(SearchENB && (i < PulseCnt-34)) {
        if(areFuzzyEqual(pulses[i     ][ONs ] * SampleDelay_us,HeaderBlinkOn ,DetectionFuzzyness) && 
           areFuzzyEqual(pulses[i     ][OFFs] * SampleDelay_us,HeaderBlinkOff,DetectionFuzzyness) &&
           ((areFuzzyEqual(pulses[i + 33][ONs ] * SampleDelay_us,FooterBlinkOn ,DetectionFuzzyness) &&
             // areFuzzyEqual(pulses[i + 33][OFFs] * SampleDelay_us,FooterBlinkOff_ms*1000,DetectionFuzzyness*1000)) )) { // Then Framed  ||
             (pulses[i + 33][OFFs] * SampleDelay_us > FooterBlinkOff_ms*1000)) )) { // Then Framed  ||
            // (i!=0)&&areFuzzyEqual(pulses[i - 1][OFFs] * SampleDelay_us,FooterBlinkOff_ms*1000,DetectionFuzzyness*1000))) { // Then Framed
            Serial.print("Searcher framed at index:");
            Serial.println(i);
            i = i + 1; // increment to first data
            for(uint8_t hexChar=0; hexChar < numCode*2; hexChar++) { // 8 Hex digits in a 32bit word, doing this because we're 16bit
                uint8_t tmpVal = 0;
                for(uint8_t bit=0; bit < 4; bit++) { // over each of the bits in the nibble
                    if(areFuzzyEqual(pulses[i+hexChar*4+3-bit][ONs ] * SampleDelay_us,TimeBlinkOn     ,DetectionFuzzyness) &&
                       areFuzzyEqual(pulses[i+hexChar*4+3-bit][OFFs] * SampleDelay_us,TimeBlinkOffTrue,DetectionFuzzyness) ) {
                        tmpVal += 1<<bit; // If it's a 1, add it in
                    } else if(!areFuzzyEqual(pulses[i+hexChar*4+3-bit][ONs ] * SampleDelay_us,TimeBlinkOn      ,DetectionFuzzyness) ||
                              !areFuzzyEqual(pulses[i+hexChar*4+3-bit][OFFs] * SampleDelay_us,TimeBlinkOffFalse,DetectionFuzzyness) ) {
                        Valid = false; // Since not true, not matching a false pattern -> invalid
                        Serial.println("ERROR: Invalid Data, Pulses are: ");
                        Serial.println(pulses[i][ONs]  * SampleDelay_us);
                        Serial.println(pulses[i][OFFs] * SampleDelay_us);
                    }
                }
                if(!Valid) {
                    hexChar = numCode*2;
                    SearchENB = false;
                } else {
                    TmpCode[hexChar] = decToChar(tmpVal);
                }
            }
            if(Valid) {
                SearchENB = false;
                i = PulseCnt-1;
            }
        } else {
            i = i + 1;
        }
    }
    if(!SearchENB && Valid) {
        for(i=0; i < 8; i++) {
            CodeCaptured[i] = TmpCode[i];
        }
        return true;
    } else {
        Serial.println("Searcher Failed to frame!");
        return false;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EchoPulseTimes(void) {
    int PulseCnt = (HitMax)? MaxPulses : currentSampleIdx;
    if(DidDummyCap) {
        Serial.println("\n\nRaw Received: (TimeON \tTimeOFF)usec");
        for (uint8_t i = 0; i < PulseCnt; i++) {
            Serial.print("[");
            Serial.print(i);
            Serial.print("]\t");
            Serial.print(pulses[i][ONs] * SampleDelay_us, DEC);
            Serial.print(" ");
            Serial.println(pulses[i][OFFs] * SampleDelay_us, DEC);
        }
        if(ProcessPulseTimes()) {
            Serial.print("\nCodeCaptured:");
            for(uint8_t i = 0; i < numCode*2; i++) {
                Serial.print((char)(CodeCaptured[i]));
            }
        }
        Serial.println("");
        for (uint8_t i = 0; i < MaxPulses; i++) {
            // Clear after ECHO
            pulses[i][ONs]  = 0;
            pulses[i][OFFs] = 0;
        }
    }
    // To stop the garbage first data
    DidDummyCap = true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
