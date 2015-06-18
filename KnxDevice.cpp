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




KnxDevice::KnxDevice(KnxTpUart* knxTpUart) {
    _knxTpUart = knxTpUart;
    _programmingMode = false;
    _lastProgButtonValue = 0;

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
}

KnxTpUartSerialEventType KnxDevice::serialEvent() {
    // forward the event processing to KNX TpUart class
    KnxTpUartSerialEventType knxEventType = _knxTpUart->serialEvent();

    switch(knxEventType) {

        case KNX_TELEGRAM: 
//        case IRRELEVANT_KNX_TELEGRAM:
            DEBUG_INFO("\nProcessing ...");            
            processTelegram();
            DEBUG_INFO("Processing ... *DONE*\n\n");
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
        
        case A_PHYSICALADDRESS_WRITE:
            if (_programmingMode && telegram->isTargetGroup()) {
            
                int area = (telegram->getBufferByte(8) & B11110000) >> 4;
                int line = telegram->getBufferByte(8) & B00001111;
                int member = telegram->getBufferByte(9);
                
#ifdef DEBUGLEVEL_DEBUG  
                Serial.print("A_PHYSICALADDRESS_WRITE:);
                Serial.print(area);
                Serial.print(".");
                Serial.print(line);
                Serial.print(".");
                Serial.println(member);
#endif
                
                _knxTpUart->setIndividualAddress(PA_INTEGER(area, line, member));
                
                byte individualAddress[2];
                _knxTpUart->getIndividualAddress(individualAddress);

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
                _knxTpUart->individualAnswerAddress(); 
            }            
            break;
            
        case A_PHYSICALADDRESS_RESPONSE:
            DEBUG_INFO("A_PHYSICALADDRESS_RESPONSE");
            telegram->print(&Serial);
            break;
            
        case A_DEVICEDESCRIPTOR_READ:
            DEBUG_INFO("A_DEVICEDESCRIPTOR_READ");
            // Request for mask version (version of bus interface)
            _knxTpUart->individualAnswerMaskVersion(telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
            break;
            
        case A_MEMORY_WRITE:        
            DEBUG_INFO("A_MEMORY_WRITE");
            processCommandMemWrite(telegram);
            break;

        case A_MEMORY_READ:        
            DEBUG_INFO("A_MEMORY_READ");
            processCommandMemRead(telegram);
            break;

        case A_AUTHORIZE_REQUEST:
            DEBUG_INFO("A_AUTHORIZE_REQUEST");
            if (_programmingMode) {
                DEBUG_INFO("A_AUTHORIZE_REQUEST answering ...");
                // Authentication request to allow memory access
                _knxTpUart->individualAnswerAuth(15 /* accesslevel */, telegram->getSequenceNumber(), telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
            }
            break;       

        case A_PROPERTYVALUE_READ:
            DEBUG_INFO("A_PROPERTYVALUE_READ");                
            if (_programmingMode) {
                processCommandPropRead(telegram);
            } 
            break;
            
        case A_PROPERTYVALUE_WRITE:
            DEBUG_INFO("A_PROPERTYVALUE_WRITE");   
            if (_programmingMode) {
                processCommandPropWrite(telegram);
            }
            break;
                 

        case A_RESTART:
            DEBUG_INFO("A_RESTART received");
            if (_programmingMode) {
                // Restart the device -> end programming mode
                setProgrammingMode(false);
                Serial.println("Received restart, ending programming mode"); 
            }

            wdt_enable(WDTO_15MS);
            while(1) { 
                // loop as fast as you can
            }
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXX"); 
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

