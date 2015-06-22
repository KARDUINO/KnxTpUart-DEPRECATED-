#include <avr/wdt.h>
#include "EEPROM.h"
#define DEBUG
#define DEBUGLEVEL_INFO
#include "DebugLog.h"
#include "KnxTpUart.h"
#include "KnxDevice.h"



// Index of Individual Address (PA) in EEPROM
#define EEPROM_INDEX_PA 0
#define PIN_PROG_BUTTON 2
#define PIN_PROG_LED 13

// -------- DON'T CHANGE ANYTHING BELOW THIS LINE ---------------------------




KnxDevice::KnxDevice(KnxTpUart* knxTpUart, bool authRequired) {
    _knxTpUart = knxTpUart;
    _programmingMode = false;
    _lastProgButtonValue = 0;
    _authRequired = authRequired;
    
    // if no authorization is required, set "all is authorized"
    if (!_authRequired) {
        _authorized = true;
    }
    // for testing only
//    setProgrammingMode(true);
}

void KnxDevice::loop() {
    // prog switch button
    int button = digitalRead(PIN_PROG_BUTTON);
    if (button != _lastProgButtonValue) {
        _lastProgButtonValue = button;
        delay(10);
        if (button==1)  {
            setProgrammingMode(!_programmingMode);
        }
    }
}

/*
 * Enable or disable programming mode
 */
void KnxDevice::setProgrammingMode(bool on) {
    DEBUG_INFO("setProgrammingMode:")
    DEBUG_INFO(on);
    _programmingMode = on;
    digitalWrite(PIN_PROG_LED, on);
    _knxTpUart->setListenToBroadcasts(on);
    
    // set back to "not authenticated"
    if (!on && _authRequired) {
        _authorized = false;
    }
    
}

KnxTpUartSerialEventType KnxDevice::serialEvent() {
    // forward the event processing to KNX TpUart class
    KnxTpUartSerialEventType knxEventType = _knxTpUart->serialEvent();

    switch(knxEventType) {

        case KNX_TELEGRAM: 
//        case IRRELEVANT_KNX_TELEGRAM:
            processTelegram();
            break;
          
        default:
/*
            Serial.print("\nNOT HANDLED TYPE: bin=");
            Serial.println(knxEventType, BIN);
            KnxTelegram* telegram = _knxTpUart->getReceivedTelegram();
            telegram->print(&Serial);
*/
            break;

    }

    return knxEventType;
}

/* *******************************************
 * Processes KNX telegrams: Check for the command and execute what ever is needed to process it
 */
void KnxDevice::processTelegram() {

    KnxTelegram* telegram = _knxTpUart->getReceivedTelegram();
    
    ApplicationControlField apci = telegram->getApplicationControlField();
    
    switch (apci) {
    
/*
        case A_GROUPVALUE_WRITE:
            telegram->print(&Serial);
            break;
*/
        
        case A_PHYSICALADDRESS_WRITE:
            if (_programmingMode && telegram->isTargetGroup()) {
            
                
#ifdef DEBUGLEVEL_DEBUG  
                int area = (telegram->getBufferByte(8) & B11110000) >> 4;
                int line = telegram->getBufferByte(8) & B00001111;
                int member = telegram->getBufferByte(9);
                Serial.print("A_PHYSICALADDRESS_WRITE:);
                Serial.print(area);
                Serial.print(".");
                Serial.print(line);
                Serial.print(".");
                Serial.println(member);
#endif
                
                byte individualAddress[2];
                individualAddress[0] = telegram->getBufferByte(8);
                individualAddress[1] = telegram->getBufferByte(9);
                
                _knxTpUart->setIndividualAddress(individualAddress);

                // store in eeprom
                EEPROM.write(EEPROM_INDEX_PA, individualAddress[0]);
                EEPROM.write(EEPROM_INDEX_PA+1, individualAddress[1]);
                
            }
            break;
            
        case A_PHYSICALADDRESS_READ:
            DEBUG_INFO("A_PHYSICALADDRESS_READ");
            if (_programmingMode) {
                // Broadcast request for individual addresses of all devices in programming mode
                DEBUG_INFO("A_PHYSICALADDRESS_READ answering ...");
		        // Send our answer back to sender
                _knxTpUart->sendPhysicalAddressResponse(); 
            }            
            break;
                       
        case A_DEVICEDESCRIPTOR_READ:
            DEBUG_INFO("A_DEVICEDESCRIPTOR_READ");
//            telegram->print(&Serial);
            int descriptorType;
            descriptorType = telegram->getFirstDataByte();
            
            byte answerData[2];
            switch(descriptorType) {
                case 0:
                    /*
                      Ein KNX Busankoppler besteht prinzipiell aus zwei Teilen: 
                      ein Kontroller und ein Übertragermodul passend zum verbundenen
                      Medium. In den verschiedenen Speicherarten des Mikroprozessors 
                      werden folgenden Daten gespeichert: 
                      
                        * die Systemsoftware : die verschiedenen standardisierten 
                          KNX Softwareprofile werden über deren „Maskenversionen“ 
                          oder „Device Descriptor Typ 0“ identifiziert. Die 
                          Maskenbezeichnung ist eine 2 Byte Kodierung wobei: 
                          
                            * die erste Ziffer y sich auf das jeweilige Medium bezieht, 
                              mit 0 für TP1, 1 für PL110, 2 für RF und 5 für KNXnet/IP. 
                              Nicht alle Profile bestehen auf allen vorgenannten Medien.
                              
                            * Die letzte Ziffer x sich auf die jeweilige Version des 
                              Profils bezieht.
                          
                          Folgende Systemprofile werden über untenstehenden 
                          Maskenversionen ETS bekanntgegeben: 
                          
                            y01xh: System 1(2)
                            y02xh: System 2(3)
                            y70xh: System 7(4)
                            y7Bxh: System B
                            y300h: LTE
                            091xh: TP1 Linien/Bereichskoppler - Repeater
                            190xh: Medienkoppler TP1-PL110
                            2010h: RF bi-direktionale Geräte
                            2110h: RF unidirektionale Geräte
                            
                            (2) früher BCU1 genannt
                            (3) früher BCU2 genannt
                            (4) früher BIM M 112 genannt                
                    */
                    answerData[0] = 0x07; // TP1, System 7
                    answerData[1] = 0x01; // Version 1
                    break;
                case 2:
                    // read-request for type 2 has not yet been received by Tpuart, triggered by calimero.
                    // so we define DEAD as result as long as we don't know what we should send
                    answerData[0] = 0xDE; 
                    answerData[1] = 0xAD;
                    break;
            }
            
            DEBUG_INFO("A_DEVICEDESCRIPTOR_READ ....");            
            _knxTpUart->sendDeviceDescriptorResponse(telegram->getSourceArea(), 
                                                    telegram->getSourceLine(), 
                                                    telegram->getSourceMember(), 
                                                    descriptorType, 
                                                    answerData);
            break;
            
        case A_MEMORY_WRITE:        
            DEBUG_INFO("A_MEMORY_WRITE");
            if (_programmingMode && _authRequired) {
                processCommandMemWrite(telegram);
            }
            break;

        case A_MEMORY_READ:        
            DEBUG_INFO("A_MEMORY_READ");
            if (_programmingMode && _authRequired) {
                processCommandMemRead(telegram);
            }
            break;

        case A_AUTHORIZE_REQUEST:
            DEBUG_INFO("A_AUTHORIZE_REQUEST");
            if (_programmingMode) {
                DEBUG_INFO("A_AUTHORIZE_REQUEST answering ...telegram dump");

                // dump telegram data to see structure
//                telegram->print(&Serial);
                
                byte key[4];
                key[0] = telegram->getBufferByte(9);
                key[1] = telegram->getBufferByte(10);
                key[2] = telegram->getBufferByte(11);
                key[3] = telegram->getBufferByte(12);
                
                int accessLevel;
                
                if (key[0] == 0x00 &&
                    key[1] == 0x0E &&
                    key[2] == 0x01 &&
                    key[3] == 0x0B) {
                    accessLevel = 15;
                } else {
                    accessLevel = 0;
                }

                // Authentication request to allow memory access
                _knxTpUart->sendAuthResponse(accessLevel /* accesslevel */, telegram->getSequenceNumber(), telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
                if (accessLevel==15) {
                    _authorized = true;
                }
            }
            break;       
/*
        case A_PROPERTYVALUE_READ:
            DEBUG_INFO("A_PROPERTYVALUE_READ");                
            if (_programmingMode && _authorized) {
                processCommandPropRead(telegram);
            } 
            break;
            
        case A_PROPERTYVALUE_WRITE:
            DEBUG_INFO("A_PROPERTYVALUE_WRITE");   
            if (_programmingMode && _authorized) {
                processCommandPropWrite(telegram);
            }
            break;
*/                 

        case A_RESTART:
            DEBUG_INFO("A_RESTART received");
            if (_programmingMode && _authorized) {
                // Restart the device -> end programming mode
                setProgrammingMode(false);                
            }

            wdt_enable(WDTO_15MS);
            while(1) { 
                // loop as fast as you can
            }
            break;    
            

            
        default:
            DEBUG_INFO("Unhandled APCI:");
            DEBUG_INFO(apci);
            break;
            
    }

}

void KnxDevice::processCommandMemRead(KnxTelegram* telegram) {
    // dump telegram data to see structure
    telegram->print(&Serial);
    
    int area = telegram->getSourceArea();
    int line = telegram->getSourceLine();
    int member = telegram->getSourceMember();
    
    int seqNr = telegram->getSequenceNumber();
    
    unsigned int start = telegram->getMemoryStart();
    int length = telegram->getMemoryLength();

#ifdef DEBUGLEVEL_DEBUG    
    Serial.print("Start: ");
    Serial.println(start, HEX);
    Serial.print("Length: ");
    Serial.println(length);
#endif   
    
    byte data[length];
    
    for(int i=0;i<length;i++){
        
        data[i] = EEPROM.read(start+i);
#ifdef DEBUGLEVEL_DEBUG
		Serial.print("Read from eeprom: ");
        Serial.print(start+i);
        Serial.print(" -> ");
        Serial.println(data[i],HEX);
#endif		
    }
    
    bool result = _knxTpUart->sendMemoryReadResponse(start, length, data, seqNr, area, line, member);
    
    DEBUG_DEBUG("Result=")
    DEBUG_DEBUG(result);
    
}


void KnxDevice::processCommandMemWrite(KnxTelegram* telegram) {
    // dump telegram data to see structure
    telegram->print(&Serial);
    
    unsigned int start = telegram->getMemoryStart();
    int length = telegram->getMemoryLength();
    byte data[length];
    telegram->getMemoryData(data);

#ifdef DEBUGLEVEL_DEBUG    
    Serial.print("Start: ");
    Serial.println(start, HEX);
    Serial.print("Length: ");
    Serial.println(length);
#endif
    
    for(int i=0;i<length;i++){
#ifdef DEBUGLEVEL_DEBUG    	
        Serial.print("data[");
        Serial.print(i);
        Serial.print("]=");
        Serial.println(data[i], HEX);
        
        Serial.print("Writing to eeprom: ");
        Serial.println(start+i);
#endif
        EEPROM.write(start+i, data[i]);
    }
    
}

/*

void KnxDevice::processCommandPropRead(KnxTelegram* telegram) {
    // dump telegram data to see structure
    telegram->print(&Serial);
    
    int objIndex = telegram->getPropertyObjIndex();
    int propId = telegram->getPropertyPropId();
    int start = telegram->getPropertyStart();
    int elements = telegram->getPropertyElements();
    
    int area = telegram->getSourceArea();
    int line = telegram->getSourceLine();
    int member = telegram->getSourceMember();
    
    int seqNr = telegram->getSequenceNumber();
    
    byte data[elements];
    telegram->getPropertyData(data);
        
    Serial.print("target: ");
    Serial.print(area);
    Serial.print(".");
    Serial.print(line);
    Serial.print(".");
    Serial.println(member);
    
    
    Serial.print("ObjIndex=");
    Serial.println(objIndex);
    Serial.print("PropId=");
    Serial.println(propId);
    Serial.print("Start=");
    Serial.println(start);
    Serial.print("Elements=");
    Serial.println(elements);
    byte dataResponse[1];
    
    dataResponse[0] = 10;
    
    bool result = _knxTpUart->sendPropertyResponse(objIndex, propId, start, elements, dataResponse, seqNr, area, line, member);
    
    Serial.print("Result=");
    Serial.println(result);
    
}
void KnxDevice::processCommandPropWrite(KnxTelegram* telegram) {
    // dump telegram data to see structure
    telegram->print(&Serial);
}

*/

