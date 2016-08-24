// Do not remove the include below
#include "ax2358f.h"

const byte numbers[10] = { SSEG_0, SSEG_1, SSEG_2, SSEG_3, SSEG_4, SSEG_5, SSEG_6, SSEG_7, SSEG_8, SSEG_9 };

byte paramPower = DEFAULT_POWER;
byte paramInput = DEFAULT_INPUT;
byte paramMute = DEFAULT_MUTE;
byte paramEnhancement = DEFAULT_ENHANCEMENT;
byte paramMixChBoost = DEFAULT_MIXCH_BOOST;
byte paramMainVolume = DEFAULT_VOLUME;
byte paramVolumeOffsets[6] = {DEFAULT_OFFSET, DEFAULT_OFFSET, DEFAULT_OFFSET, DEFAULT_OFFSET, DEFAULT_OFFSET, DEFAULT_OFFSET};

int serialBuffer[16] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
byte serialLength = 0;

//long encPosition = INITIAL_ENCODER_POS;

//Encoder encMain(ENC_A, ENC_B);
IRrecv irReceiver(IR);
decode_results res;

//The setup function is called once at startup of the sketch
void setup() {
    Serial.begin(SERIAL_SPEED);
    irReceiver.enableIRIn();
    Wire.begin();

    pinMode(LED_CLK, OUTPUT);
    pinMode(LED_DATA, OUTPUT);
    pinMode(LED_EN1, OUTPUT);
    pinMode(LED_EN2, OUTPUT);
    pinMode(MUTE_NEG, OUTPUT);
    pinMode(ONBOARD_LED, OUTPUT);

    //displayChar();
    delay(1000);
    initAmp();
}

void initAmp() {
    /* mute all channels */
    setMute(ON);

    /* wait 2 seconds for the amp to settle
     * meanwhile load all parameters from EEPROM */
    restoreParameters();
    delay(2000);

    /* set the states defined in the parameters */
    setInput(paramInput);
    setSurroundEnhancement(paramEnhancement);
    setMixerChannel6Db(paramMixChBoost);
    setMute(paramMute);

    applyGlobalVolume();
}

void displayChar() {
    digitalWrite(LED_EN1, HIGH);
    digitalWrite(LED_EN2, HIGH);
    shiftOut(LED_DATA, LED_CLK, LSBFIRST, SSEG_DASH);
}

void displayNumber(byte num) {
    digitalWrite(LED_EN2, LOW);
    digitalWrite(LED_EN1, HIGH);
    shiftOut(LED_DATA, LED_CLK, LSBFIRST, numbers[num / 10]);
    delay(10);
    digitalWrite(LED_EN1, LOW);
    digitalWrite(LED_EN2, HIGH);
    shiftOut(LED_DATA, LED_CLK, LSBFIRST, numbers[num % 10]);
    delay(10);
}

// The loop function is called in an endvoid initAmp()less loop
void loop() {
  #if 0 // Encoder function not using commented by jithin
    long encNew = encMain.read();
    if (encNew != encPosition) {
        if (encNew > encPosition && encNew % 4 != 0) {
            increaseVolume();
        } else if (encNew < encPosition && encNew % 4 != 0) {
            decreaseVolume();
        }
        encPosition = encNew;
    }
  #endif // End of comment by jithin
    if (irReceiver.decode(&res)) {
        handleInfrared(res.value);
        delay(30);
        irReceiver.resume();
    }

    displayNumber(paramMainVolume);
    handleSerial();
}

void setInput(byte input) {
    switch (input) {
        case INPUT_STEREO:
            ax2358fAudioSwitcing(AX2358F_INST1);
            break;
        case INPUT_SURROUND:
            ax2358fAudioSwitcing(AX2358F_IN6CH);
            break;
    }
    
}

void setSurroundEnhancement(byte enhancement) {
    if (enhancement) {
        ax2358fAudioSwitcing(AX2358F_SURRENH_ON);
    } else {
        ax2358fAudioSwitcing(AX2358F_SURRENH_OFF);
    }
}

void setMixerChannel6Db(byte mix6db) {
    if (mix6db) {
        ax2358fAudioSwitcing(AX2358F_MIXCHAN_6DB);
    } else {
        ax2358fAudioSwitcing(AX2358F_MIXCHAN_0DB);
    }
}

void setMute(byte mute) {
    if (mute) {
         ax2358f(CHAN_MUTE, 0);
         digitalWrite(MUTE_NEG, LOW);
    } else {
        ax2358f(CHAN_UNMUTE, 0);
        digitalWrite(MUTE_NEG, HIGH);
    }
}

void increaseVolume() {
    if (paramMainVolume < MAX_ATTENUATION) {
        paramMainVolume++;
        applyGlobalVolume();
    }
}

void decreaseVolume() {
    if (paramMainVolume > MIN_ATTENUATION) {
        paramMainVolume--;
        applyGlobalVolume();
    }
}

void applyGlobalVolume() {
  int channelVolume;
  
    for (byte i = OFFSET_FL; i <= OFFSET_SW; i++) {
        channelVolume = paramMainVolume + paramVolumeOffsets[i] - VOLUME_OFFSET_HALF;
        if (channelVolume < MIN_ATTENUATION) {
            channelVolume = MIN_ATTENUATION;
        }
        if (channelVolume > MAX_ATTENUATION) {
            channelVolume = MAX_ATTENUATION;
        }
         setChannelVolume(i+1, channelVolume);
    }
}

void setChannelVolume(byte channel, byte volume) {
    if (volume >= MIN_ATTENUATION && volume <= MAX_ATTENUATION) {
        byte attenuation = MAX_ATTENUATION - volume;
        ax2358f(channel, attenuation);
    }
}

void ax2358fAudioSwitcing(byte command) {
    Wire.beginTransmission(AX2358F_ADDRESS);
    Wire.write(command);
    Wire.endTransmission();
}

void ax2358f(byte channel, byte value) {
    byte x10 = value / 10;
    byte x1 = value % 10;

    switch (channel) {
        case CHAN_ALL:
            x1 += AX2358F_ALLCH_1DB;
            x10 += AX2358F_ALLCH_10DB;
            break;
        case CHAN_FL:
            x1 += AX2358F_FL_1DB;
            x10 += AX2358F_FL_10DB;
            break;
        case CHAN_FR:
            x1 += AX2358F_FR_1DB;
            x10 += AX2358F_FR_10DB;
            break;
        case CHAN_CEN:
            x1 += AX2358F_CEN_1DB;
            x10 += AX2358F_CEN_10DB;
            break;
        case CHAN_SW:
            x1 += AX2358F_SW_1DB;
            x10 += AX2358F_SW_10DB;
            break;
        case CHAN_RL:
            x1 += AX2358F_RL_1DB;
            x10 += AX2358F_RL_10DB;
            break;
        case CHAN_RR:
            x1 += AX2358F_RR_1DB;
            x10 += AX2358F_RR_10DB;
            break;
        case CHAN_MUTE:
             x10 = x1 = AX2358F_ALL_MUTE;
             break;
         case CHAN_UNMUTE:
             x10 = x1 = AX2358F_ALL_UNMUTE;
             break;
         default:
             break;        
    }
    Wire.beginTransmission(AX2358F_ADDRESS);
    Wire.write(x10);
    Wire.write(x1);
    Wire.endTransmission();
}

void handleSerial() {
    byte c;
    bool eol = false;

    while (Serial.available() > 0) {
        c = Serial.read();
            Serial.write(c);
        if (c == 13 || c == 10 || serialLength >= 16) {
            eol = true;
        } else {
            serialBuffer[serialLength] = c;
            serialLength++;
        }
    }

    if (eol) {
        bool commandOk = false;
        byte chan = UNKNOWN_BYTE, vol = 0;
        if (checkHeader()) {
                switch (serialBuffer[SERIAL_COMMAND_POS]) {
                case 'P':
                case 'p':
                    paramPower = (isOn() ? ON : OFF);
                    commandOk = true;
                    break;
                case 'V':
                case 'v':
                    chan = getChannel();
                    if (chan == CHAN_ALL) {  //mainVolume is between 0 and 79
                        vol = getNumber();
                        if (vol >= 0 && vol <= 79) { // Validate main volume is in between 0 and 79
                            paramMainVolume = vol;
                            applyGlobalVolume();
                        } else {
                           Serial.println("mainVolume is between 0 and 79");
                        }
                    } else if (chan == FRONT_CHLS) { // OPTION 7
                            vol = getNumber();
                            if (vol >= 0 && vol <= 30) {
                                paramVolumeOffsets[OFFSET_FL] = vol;
                                paramVolumeOffsets[OFFSET_FR] = vol;
                                applyGlobalVolume();
                            }  else {
                              Serial.println("Front Channel offset is between 0 and 30");
                            }
                        } else if (chan == REAR_CHLS) { // OPTION 8
                            vol = getNumber();
                            if (vol >= 0 && vol <= 30) {
                                paramVolumeOffsets[OFFSET_RL] = vol;
                                paramVolumeOffsets[OFFSET_RR] = vol;
                                applyGlobalVolume();
                            } else {
                              Serial.println("Rear Channel offset is between 0 and 30");
                            }
                        } else {
                            vol = getNumber();
                            if (vol >= 0 && vol <= 30) {
                            switch(chan) {
                              case CHAN_FL:
                              paramVolumeOffsets[OFFSET_FL] = vol;
                              break;
                              case CHAN_FR:
                              paramVolumeOffsets[OFFSET_FR] = vol;
                              break;
                              case CHAN_RL:
                              paramVolumeOffsets[OFFSET_RL] = vol;
                              break;
                              case CHAN_RR:
                              paramVolumeOffsets[OFFSET_RR] = vol;
                              break;
                              case CHAN_CEN:
                              paramVolumeOffsets[OFFSET_CEN] = vol;
                              break;
                              case CHAN_SW:
                              paramVolumeOffsets[OFFSET_SW] = vol;
                              break;
                              default:
                              break;
                            } 
                                applyGlobalVolume();
                                //setChannelVolume(chan, vol);
                            } else {
                                Serial.println("Channel offset is between 0 and 30");
                            }
                       }
                    commandOk = true;
                    break;
                case 'E':
                case 'e':
                    paramEnhancement = (isOn() ? ON : OFF);
                    setSurroundEnhancement(paramEnhancement);
                    commandOk = true;
                    break;
                case 'B':
                case 'b':
                    paramMixChBoost = (isOn() ? ON : OFF);
                    setMixerChannel6Db(paramMixChBoost);
                    commandOk = true;
                    break;
                case 'M':
                case 'm':
                    paramMute = (isOn() ? ON : OFF);
                    setMute(paramMute);
                    commandOk = true;
                    break;
                case 'S':
                case 's':
                    if (isOn()) {
                        Serial.print("Store Settings..");
                        storeParameters();
                        commandOk = true;
                    }
                    break;
                case 'R':
                case 'r':
                    if (isOn()) {
                        Serial.print("Restore Settings ");
                        restoreParameters();
                        applyGlobalVolume();
                        commandOk = true;
                    }
                    break;
                case 'I':
                case 'i':
                    paramInput = (isOn() ? INPUT_SURROUND : INPUT_STEREO);
                    setInput(paramInput);
                    commandOk = true;
                    break;
                case 'D':
                case 'd':
                    printStatus();
                    commandOk = true;
                    break;
                case 'H':
                case 'h':
                    printHelp();
                    commandOk = true;
                    break;
                case 'F':
                case 'f':
                    restoreDefaults();
                    applyGlobalVolume();
                    commandOk = true;
                    break;
                default:
                    break;
            }

            if (commandOk) {
                Serial.println("OK");
            } else {
                Serial.println("ERROR");
            }
        } else {
             Serial.println("Ex: cmdv50");
        }

        clearSerialBuffer();
    }
}

void handleInfrared(unsigned long decodedValue) {
    
    if (ValidateIRCode(decodedValue)) {
        blinkLed();
    }      
    switch (decodedValue) {
        case IR_VOLDOWN:
        case IR_FD_VOLDOWN:
            decreaseVolume();
            break;
        case IR_VOLUP:
        case IR_FD_VOLUP:
            increaseVolume();
            break;
        case IR_MUTE_UNMUTE:
        case IR_FD_MUTE_UNMUTE:
            paramMute = (paramMute ? OFF : ON);
            setMute(paramMute);
            break;
        case IR_INPUTSEL:
        case IR_FD_INPUTSEL:
            paramInput = (paramInput ? INPUT_STEREO : INPUT_SURROUND);
            setInput(paramInput);
            break;
        case IR_RESET_CFG:  //restore from EEPROM
            restoreParameters();
            applyGlobalVolume();
            break;
        case IR_SAVE_CFG:  //Save cfg to EEPROM
        case IR_FD_SAVE_CFG:
            storeParameters();
            break;
        case IR_SURR_EN:  //toggle enhancement
        case IR_FD_SURR_EN:
            paramEnhancement = (paramEnhancement ? OFF : ON);
            setSurroundEnhancement(paramEnhancement);
            break;
        case IR_MIX_CH_BOOST:  //toggle mixchboost
            paramMixChBoost = (paramMixChBoost ? OFF : ON);
            setMixerChannel6Db(paramMixChBoost);
            break;
        case IR_POWER:
        case IR_FD_POWER:
            if (paramPower)
              paramPower = OFF;
            else
              paramPower = ON;
            break;
        //case IR_REPEAT:
        case IR_FRL_VOLUP: // Front Left ch vol up
                if ( paramVolumeOffsets[OFFSET_FL] >= 30) {   
                     paramVolumeOffsets[OFFSET_FL] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_FL]++;
                }
                applyGlobalVolume();     
            break;
        case IR_FRL_VOLDOWN: // Front Left ch vol down
              if ( paramVolumeOffsets[OFFSET_FL] <= 0) {   
                     paramVolumeOffsets[OFFSET_FL] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_FL]--;
                 }
                applyGlobalVolume();     
            break;
        case IR_FRR_VOLUP: // Front Right ch vol up
               if ( paramVolumeOffsets[OFFSET_FR] >= 30) {   
                     paramVolumeOffsets[OFFSET_FR] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_FR]++;
                }
                applyGlobalVolume();     
            break;
        case IR_FRR_VOLDOWN: // Front Right ch vol down
              if ( paramVolumeOffsets[OFFSET_FR] <= 0) {   
                     paramVolumeOffsets[OFFSET_FR] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_FR]--;
               }
               applyGlobalVolume();     
           break;
        case IR_CT_VOLUP: // Front Center ch vol up
        case IR_FD_CT_VOLUP:
              if ( paramVolumeOffsets[OFFSET_CEN] >= 30) {   
                     paramVolumeOffsets[OFFSET_CEN] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_CEN]++;
                }
                applyGlobalVolume(); 
            break;
        case IR_CT_VOLDOWN: // Front Center ch vol up
        case IR_FD_CT_VOLDOWN:
              if ( paramVolumeOffsets[OFFSET_CEN] <= 0) {   
                     paramVolumeOffsets[OFFSET_CEN] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_CEN]--;
                }
                applyGlobalVolume();
            break;
        case IR_RRL_VOLUP: // Rear Left ch vol up
              if ( paramVolumeOffsets[OFFSET_RL] >= 30) {   
                     paramVolumeOffsets[OFFSET_RL] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_RL]++;
                }
                applyGlobalVolume();     
            break;
            case IR_RRL_VOLDOWN: // // Rear Left ch vol up
              if ( paramVolumeOffsets[OFFSET_RL] <= 0) {   
                     paramVolumeOffsets[OFFSET_RL] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_RL]--;
                }
                applyGlobalVolume();     
            break;
         case IR_RRR_VOLUP: // Rear Right ch vol up
              if ( paramVolumeOffsets[OFFSET_RR] >= 30) {   
                     paramVolumeOffsets[OFFSET_RR] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_RR]++;
                }
                applyGlobalVolume();     
            break;
        case IR_RRR_VOLDOWN:
              if ( paramVolumeOffsets[OFFSET_RR] <= 0) {   
                     paramVolumeOffsets[OFFSET_RR] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_RR]--;
                }
                applyGlobalVolume();     
              break;   
         case IR_SUB_VOLUP:
         case IR_FD_SUB_VOLUP:
                if ( paramVolumeOffsets[OFFSET_SW] >= 30) {   
                     paramVolumeOffsets[OFFSET_SW] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_SW]++; 
                }
                applyGlobalVolume();     
            break;
            case IR_SUB_VOLDOWN:
            case IR_FD_SUB_VOLDOWN:
                if ( paramVolumeOffsets[OFFSET_SW] <= 0) {   
                     paramVolumeOffsets[OFFSET_SW] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_SW]--;
                }
                applyGlobalVolume();     
            break;
            case IR_FD_RR_VOLUP: // Rear L+R ch vol up
              if ( paramVolumeOffsets[OFFSET_RL] >= 30) {   
                     paramVolumeOffsets[OFFSET_RL] = 30;
                     paramVolumeOffsets[OFFSET_RR] = paramVolumeOffsets[OFFSET_RL];
                } else { 
                     paramVolumeOffsets[OFFSET_RL]++;
                     paramVolumeOffsets[OFFSET_RR] = paramVolumeOffsets[OFFSET_RL];
;                }
                applyGlobalVolume();     
            break;
            case IR_FD_RR_VOLDOWN: // Rear L+R ch vol down
              if ( paramVolumeOffsets[OFFSET_RL] <= 0) {   
                     paramVolumeOffsets[OFFSET_RL] = 0;
                     paramVolumeOffsets[OFFSET_RR] = paramVolumeOffsets[OFFSET_RL];
                } else { 
                     paramVolumeOffsets[OFFSET_RL]--;
                     paramVolumeOffsets[OFFSET_RR] = paramVolumeOffsets[OFFSET_RL];
                }
                applyGlobalVolume();     
            break;
            case IR_FD_FR_VOLUP: // Front L+R Volume up
              if ( paramVolumeOffsets[OFFSET_FL] >= 30) {   
                     paramVolumeOffsets[OFFSET_FL] = 30;
                     paramVolumeOffsets[OFFSET_FR] = paramVolumeOffsets[OFFSET_FL];
                } else { 
                     paramVolumeOffsets[OFFSET_FL]++;
                     paramVolumeOffsets[OFFSET_FR] = paramVolumeOffsets[OFFSET_FL];
                }
                applyGlobalVolume();
         break;
         case IR_FD_FR_VOLDOWN: // Front L+R Volume down
              if ( paramVolumeOffsets[OFFSET_FL] <= 0) {   
                     paramVolumeOffsets[OFFSET_FL] = 0;
                     paramVolumeOffsets[OFFSET_FR] = paramVolumeOffsets[OFFSET_FL];
                } else { 
                     paramVolumeOffsets[OFFSET_FL]--;
                     paramVolumeOffsets[OFFSET_FR] = paramVolumeOffsets[OFFSET_FL];
                }
                applyGlobalVolume();
         break;
    }
}

bool checkHeader() {
    byte header_c[] = {'C','M','D'};
    byte header_s[] = {'c','m','d'};

    for (byte i = 0; i < SERIAL_HEADER_LENGTH; i++) {
        if (serialBuffer[i] != header_s[i]) {
            return false;
        }
    }
    return true;
}

bool isOn() {
    if (serialBuffer[SERIAL_VALUE_POS] == '1') {
        return true;
    } else {
        return false;
    }
}

byte getChannel() {
    switch (serialBuffer[SERIAL_VALUE_POS]) {
        case '0': // 1 for all channels
            return CHAN_ALL;
        case '1':
            return CHAN_FL;
        case '2':
            return CHAN_FR;
        case '3':
            return CHAN_RL;
        case '4':
            return CHAN_RR;
        case '5':
            return CHAN_CEN;
        case '6':
            return CHAN_SW;
        case '7':
            return FRONT_CHLS;
        case '8':
            return REAR_CHLS;
    }

    return UNKNOWN_BYTE;
}

byte getNumber() {
    char value[2] = {(char)serialBuffer[SERIAL_VALUE_POS + 1], (char)serialBuffer[SERIAL_VALUE_POS + 2]};
    return atoi(value);
}

void printStatus() {
    Serial.print("Power: ");
    Serial.println(paramPower);
    Serial.print("Input: ");
    Serial.println(paramInput);
    Serial.print("0.Main volume: ");
    Serial.println(paramMainVolume);
    Serial.println("Volume offsets [0-30]: ");
    Serial.print("1.  FL   : ");
    Serial.println(paramVolumeOffsets[OFFSET_FL] - VOLUME_OFFSET_HALF);
    Serial.print("2.  FR   : ");
    Serial.println(paramVolumeOffsets[OFFSET_FR] - VOLUME_OFFSET_HALF);
    Serial.print("3.  SL   : ");
    Serial.println(paramVolumeOffsets[OFFSET_RL] - VOLUME_OFFSET_HALF);
    Serial.print("4.  SR   : ");
    Serial.println(paramVolumeOffsets[OFFSET_RR] - VOLUME_OFFSET_HALF);
    Serial.print("5.  CE   : ");
    Serial.println(paramVolumeOffsets[OFFSET_CEN] - VOLUME_OFFSET_HALF);
    Serial.print("6.  SW   : ");
    Serial.println(paramVolumeOffsets[OFFSET_SW] - VOLUME_OFFSET_HALF);
    Serial.print("7.  FL&FR: ");
    Serial.print(paramVolumeOffsets[OFFSET_FL] - VOLUME_OFFSET_HALF);
    Serial.print(":");
    Serial.println(paramVolumeOffsets[OFFSET_FR] - VOLUME_OFFSET_HALF);
    Serial.print("8.  SL&SR: ");
    Serial.print(paramVolumeOffsets[OFFSET_RL] - VOLUME_OFFSET_HALF);
    Serial.print(":");
    Serial.println(paramVolumeOffsets[OFFSET_RR] - VOLUME_OFFSET_HALF);
    Serial.print("Muting: ");
    Serial.println(paramMute);
    Serial.print("Enhancement: ");
    Serial.println(paramEnhancement);
    Serial.print("Mixed channel boost: ");
    Serial.println(paramMixChBoost);
}

void printHelp() {
    Serial.println("H: Help");
    Serial.println("P: Power");
    Serial.println("V: Volume");
    Serial.println("E: Surr. Sound");
    Serial.println("B: Enable Ch boost");
    Serial.println("I: Surr. Sound/Stereo ");
    Serial.println("M: Mute");
    Serial.println("S: Save Settings");
    Serial.println("R: Restore Settings");
    Serial.println("F: Factory Restore");
    Serial.println("D: Display Settings");     
    
}

void clearSerialConsole() {
    Serial.write(27);
    Serial.print("[2J");
    Serial.write(27);
    Serial.print("[H");
}

void clearSerialBuffer() {
    for (byte i = 0; i < 16; i++) {
        serialBuffer[i] = '\0';
    }
    serialLength = 0;
}

void storeParameters() {
    EEPROM.write(ADDR_INPUT, paramInput);
    EEPROM.write(ADDR_MUTE, paramMute);
    EEPROM.write(ADDR_ENHANCEMENT, paramEnhancement);
    EEPROM.write(ADDR_MIXCHBOOST, paramMixChBoost);
    EEPROM.write(ADDR_MAINVOLUME, paramMainVolume);
    EEPROM.write(ADDR_OFFSET_FL, paramVolumeOffsets[OFFSET_FL]);
    EEPROM.write(ADDR_OFFSET_FR, paramVolumeOffsets[OFFSET_FR]);
    EEPROM.write(ADDR_OFFSET_RL, paramVolumeOffsets[OFFSET_RL]);
    EEPROM.write(ADDR_OFFSET_RR, paramVolumeOffsets[OFFSET_RR]);
    EEPROM.write(ADDR_OFFSET_CEN, paramVolumeOffsets[OFFSET_CEN]);
    EEPROM.write(ADDR_OFFSET_SUB, paramVolumeOffsets[OFFSET_SW]);
}

void restoreParameters() {
    char ch_no, eeprom_addr;
    
    paramInput = EEPROM.read(ADDR_INPUT);
    if (paramInput < 0 || paramInput > 1) {
        paramInput = DEFAULT_INPUT;
        EEPROM.write(ADDR_INPUT, paramInput);
    }
    paramMute = EEPROM.read(ADDR_MUTE);
    if (paramMute < 0 || paramMute > 1) {
        paramMute = DEFAULT_MUTE;
        EEPROM.write(ADDR_MUTE, paramMute);
    }
    paramEnhancement = EEPROM.read(ADDR_ENHANCEMENT);
    if (paramEnhancement < 0 || paramEnhancement > 1) {
        paramEnhancement = DEFAULT_ENHANCEMENT;
        EEPROM.write(ADDR_ENHANCEMENT, paramEnhancement);
    }
    paramMixChBoost = EEPROM.read(ADDR_MIXCHBOOST);
    if (paramMixChBoost < 0 || paramMixChBoost > 1) {
        paramMixChBoost = DEFAULT_MIXCH_BOOST;
        EEPROM.write(ADDR_MIXCHBOOST, paramMixChBoost);
    }
    paramMainVolume = EEPROM.read(ADDR_MAINVOLUME);
    if (paramMainVolume < MIN_ATTENUATION  || paramMainVolume > MAX_ATTENUATION ) {
        paramMainVolume = DEFAULT_VOLUME;
        EEPROM.write(ADDR_MAINVOLUME, paramMainVolume);
    }
    for (ch_no = OFFSET_FL, eeprom_addr = ADDR_OFFSET_FL; ch_no < OFFSET_SW; ch_no++, eeprom_addr++) {
        paramVolumeOffsets[ch_no] = EEPROM.read(eeprom_addr);
        if (paramVolumeOffsets[ch_no] < 0 || paramVolumeOffsets[ch_no] > 30) {
            paramVolumeOffsets[ch_no] = DEFAULT_OFFSET;
            EEPROM.write(eeprom_addr, paramVolumeOffsets[ch_no]);
        }
    }
}

void restoreDefaults() {
    paramInput = DEFAULT_INPUT;
    paramMute = DEFAULT_MUTE;
    paramEnhancement = DEFAULT_ENHANCEMENT;
    paramMixChBoost = DEFAULT_MIXCH_BOOST;
    paramMainVolume = DEFAULT_VOLUME;
    paramVolumeOffsets[OFFSET_FL] = DEFAULT_OFFSET;
    paramVolumeOffsets[OFFSET_FR] = DEFAULT_OFFSET;
    paramVolumeOffsets[OFFSET_RL] = DEFAULT_OFFSET;
    paramVolumeOffsets[OFFSET_RR] = DEFAULT_OFFSET;
    paramVolumeOffsets[OFFSET_CEN]= DEFAULT_OFFSET;
    paramVolumeOffsets[OFFSET_SW] = DEFAULT_OFFSET;
    storeParameters();
  
}

void blinkLed() {
    digitalWrite(ONBOARD_LED, HIGH);
    delay(25);
    digitalWrite(ONBOARD_LED, LOW);
    delay(25);
}

bool ValidateIRCode(unsigned long decodedValue) {
 
  switch(decodedValue) {
      case IR_VOLDOWN:
      case IR_FD_VOLDOWN:
      case IR_VOLUP:
      case IR_FD_VOLUP:
      case IR_POWER:
      case IR_FD_POWER:
      case IR_INPUTSEL:
      case IR_FD_INPUTSEL: 
      case IR_RESET_CFG:
      case IR_SURR_EN:
      case IR_FD_SURR_EN: 
      case IR_MIX_CH_BOOST:
      //case IR_REPEAT:
      case IR_FRL_VOLUP:
      case IR_FRL_VOLDOWN:
      case IR_FRR_VOLUP:
      case IR_FRR_VOLDOWN:
      case IR_FD_FR_VOLUP:
      case IR_FD_FR_VOLDOWN: 
      case IR_CT_VOLUP:
      case IR_FD_CT_VOLUP:
      case IR_CT_VOLDOWN:
      case IR_FD_CT_VOLDOWN:
      case IR_RRL_VOLUP:
      case IR_FD_RR_VOLUP:
      case IR_FD_RR_VOLDOWN:
      case IR_RRL_VOLDOWN:
      case IR_RRR_VOLUP:
      case IR_RRR_VOLDOWN:
      case IR_MUTE_UNMUTE:
      case IR_FD_MUTE_UNMUTE:
      case IR_SUB_VOLUP:
      case IR_FD_SUB_VOLUP:
      case IR_SUB_VOLDOWN:
      case IR_FD_SUB_VOLDOWN:
      case IR_SAVE_CFG:
      case IR_FD_SAVE_CFG:
      return 1;
      break;
    default:
      return 0;
      break;
  }    
}

