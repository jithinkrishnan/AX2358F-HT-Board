/************************************************************************
*   AX2358F Audio Processor Based Home Theatre Board
*   File:   ax2358f.ino
*   Author:  Jithin Krishnan.K
*       Rev. 1.0 : 26/11/2019 :  10:17 AM
* 
* This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Email: jithinkrishnan.k@gmail.com
*   
************************************************************************/

#include <radio.h>
#include <RDA5807M.h>
#include <RDSParser.h>
#include "ax2358f.h"

byte paramPower;
byte paramInput;
byte paramMute;
byte paramEnhancement;
byte paramMixChBoost;
byte paramMainVolume;
byte paramVolumeOffsets[6];

IRrecv irReceiver(IR);
decode_results res;
RDA5807M radio;


void setup() {
    irReceiver.enableIRIn();
    Wire.begin();
    pinMode(ONBOARD_LED, OUTPUT);

    delay(1000);
    initAmp();
    //RDA FM Chip
    radio.init();
    radio.setBandFrequency(RADIO_BAND_FMWORLD, 10270);
    radio.setMono(false);
    radio.setMute(false);
    radio.setVolume(15);
    radio.setBassBoost(1);
    
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
}

void setInput(byte input) {
    switch (input) {
        case INPUT_FM:
            ax2358fAudioSwitcing(AX2358F_IN_FM);
            break;
        case INPUT_SURROUND:
            ax2358fAudioSwitcing(AX2358F_IN6CH);
            break;
        case INPUT_AUX1:
            ax2358fAudioSwitcing(AX2358F_IN_AUX1);
            break;
        case INPUT_AUX2:
            ax2358fAudioSwitcing(AX2358F_IN_AUX2);
            break;
        case INPUT_USB:
            ax2358fAudioSwitcing(AX2358F_IN_USB);
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
    } else {
        ax2358f(CHAN_UNMUTE, 0);
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

void handleInfrared(unsigned long decodedValue) {
  unsigned long startSeek;
  RADIO_FREQ fSave, fLast;
  RADIO_FREQ fMin = radio.getMinFrequency();
  RADIO_FREQ fMax = radio.getMaxFrequency();
 
  
    if (ValidateIRCode(decodedValue)) {
        blinkLed();
    }      
    switch (decodedValue) {
        case IR_VOLDOWN:
            decreaseVolume();
            break;
        case IR_VOLUP:
            increaseVolume();
            break;
        case IR_MUTE_UNMUTE:
            paramMute = (paramMute ? OFF : ON);
            setMute(paramMute);
            break;
        case IR_INPUT_DVD:
            setInput(INPUT_SURROUND);
            break;
        case IR_INPUT_FM:        
            setInput(INPUT_FM);
            break;
        case IR_INPUT_AUX1:        
            setInput(INPUT_AUX1);
            break;
        case IR_INPUT_AUX2:        
            setInput(INPUT_AUX1);
            break;
        case IR_INPUT_USB:        
            setInput(INPUT_USB);
            break;        
        case IR_RESET_CFG:  //restore from EEPROM
            restoreParameters();
            applyGlobalVolume();
            break;
        case IR_SAVE_CFG:  //Save cfg to EEPROM
            storeParameters();
            break;
        case IR_SURR_EN:  //toggle enhancement
            paramEnhancement = (paramEnhancement ? OFF : ON);
            setSurroundEnhancement(paramEnhancement);
            break;
        case IR_MIX_CH_BOOST:  //toggle mixchboost
            paramMixChBoost = (paramMixChBoost ? OFF : ON);
            setMixerChannel6Db(paramMixChBoost);
            break;
        case IR_POWER:
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
              if ( paramVolumeOffsets[OFFSET_CEN] >= 30) {   
                     paramVolumeOffsets[OFFSET_CEN] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_CEN]++;
                }
                applyGlobalVolume(); 
            break;
        case IR_CT_VOLDOWN: // Front Center ch vol up
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
              if ( paramVolumeOffsets[OFFSET_SW] >= 30) {   
                     paramVolumeOffsets[OFFSET_SW] = 30;
                } else { 
                     paramVolumeOffsets[OFFSET_SW]++; 
                }
                applyGlobalVolume();     
            break;
            case IR_SUB_VOLDOWN:
               if ( paramVolumeOffsets[OFFSET_SW] <= 0) {   
                     paramVolumeOffsets[OFFSET_SW] = 0;
                } else { 
                     paramVolumeOffsets[OFFSET_SW]--;
                }
                applyGlobalVolume();     
            break;
            case IR_TUNE_UP:
                radio.seekUp(true);
            break;
            case IR_TUNE_DOWN:
                radio.seekDown(true);
            break;
    }
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
      case IR_VOLUP:
      case IR_POWER:
      case IR_INPUT_DVD:
      case IR_INPUT_AUX1:
      case IR_INPUT_AUX2:
      case IR_INPUT_FM:
      case IR_INPUT_USB:
      case IR_RESET_CFG:
      case IR_SURR_EN:
      case IR_MIX_CH_BOOST:
      case IR_FRL_VOLUP:
      case IR_FRL_VOLDOWN:
      case IR_FRR_VOLUP:
      case IR_FRR_VOLDOWN:
      case IR_CT_VOLDOWN:
      case IR_RRL_VOLUP:
      case IR_RRL_VOLDOWN:
      case IR_RRR_VOLUP:
      case IR_RRR_VOLDOWN:
      case IR_MUTE_UNMUTE:
      case IR_SUB_VOLUP:
      case IR_SUB_VOLDOWN:
      case IR_SAVE_CFG:
      return 1;
      break;
    default:
      return 0;
      break;
  }    
}

