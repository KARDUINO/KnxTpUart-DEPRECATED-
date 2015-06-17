#include "KnxDevice.h"
#include "EEPROM.h"
#define DEBUG

// Index of Individual Address (PA) in EEPROM
#define EEPROM_INDEX_PA 0
#define PIN_PROG_BUTTON 2
#define PIN_PROG_LED 13

// -------- DON'T CHANGE ANYTHING BELOW THIS LINE ---------------------------
 

#if defined(DEBUG)
#define CONSOLEDEBUG(...)  sprintf (consolebuffer, __VA_ARGS__); Serial.println(consolebuffer);
char consolebuffer[80];
#else
#define CONSOLEDEBUG(...) 
#endif

#if defined(DEBUG)

#endif


KnxDevice::KnxDevice(KnxTpUart* knxTpUart) {
    _knxTpUart = knxTpUart;
    _programmingMode = false;
    _lastProgButtonValue = 0;

    // for testing only
    setProgrammingMode(true);
}

void KnxDevice::loop() {
    // prog switch button
    int button = digitalRead(PIN_PROG_BUTTON);
    if (button != _lastProgButtonValue) {
        //CONSOLEDEBUG("ProgButton: %i", button);
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
    CONSOLEDEBUG("setProgrammingMode: %i", on);
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
            CONSOLEDEBUG("\nProcessing ...");            
            processTelegram();
            CONSOLEDEBUG("Processing ... *DONE*\n\n");
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
    
        case A_GROUPVALUE_READ:
            break;

        case A_GROUPVALUE_WRITE:
            break;
            
        case A_GROUPVALUE_RESPONSE:
            break;
    
        case A_PHYSICALADDRESS_WRITE:
            if (_programmingMode && telegram->isTargetGroup()) {
            
                int area = (telegram->getBufferByte(8) & B11110000) >> 4;
                int line = telegram->getBufferByte(8) & B00001111;
                int member = telegram->getBufferByte(9);
                
                CONSOLEDEBUG("A_PHYSICALADDRESS_WRITE: %i.%i.%i", area, line, member);                
                
                _knxTpUart->setIndividualAddress(PA_INTEGER(area, line, member));
                
                byte individualAddress[2];
                _knxTpUart->getIndividualAddress(individualAddress);

                // store in eeprom
                EEPROM.write(EEPROM_INDEX_PA, individualAddress[0]);
                EEPROM.write(EEPROM_INDEX_PA+1, individualAddress[1]);
                
            }
            break;
            
        case A_PHYSICALADDRESS_READ:
            CONSOLEDEBUG("A_PHYSICALADDRESS_READ");
            if (_programmingMode) {
                // Broadcast request for individual addresses of all devices in programming mode
                CONSOLEDEBUG("A_PHYSICALADDRESS_READ answering ...");
		        // Send our answer back to sender
                _knxTpUart->individualAnswerAddress(); 
            }            
            break;
            
        case A_PHYSICALADDRESS_RESPONSE:
            CONSOLEDEBUG("A_PHYSICALADDRESS_RESPONSE");
            telegram->print(&Serial);
            break;
            
        case A_DEVICEDESCRIPTOR_READ:
            CONSOLEDEBUG("A_DEVICEDESCRIPTOR_READ");
            // Request for mask version (version of bus interface)
            _knxTpUart->individualAnswerMaskVersion(telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
            break;
            
        case A_MEMORY_WRITE:        
            CONSOLEDEBUG("A_MEMORY_WRITE");
            processCommandMemWrite(telegram);
            break;

        case A_MEMORY_READ:        
            CONSOLEDEBUG("A_MEMORY_READ");
            processCommandMemRead(telegram);
            break;

        case A_AUTHORIZE_REQUEST:
            CONSOLEDEBUG("A_AUTHORIZE_REQUEST");
            if (_programmingMode) {
                CONSOLEDEBUG("A_AUTHORIZE_REQUEST answering ...");
                // Authentication request to allow memory access
                _knxTpUart->individualAnswerAuth(15 /* accesslevel */, telegram->getSequenceNumber(), telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
            }
            break;       

        case A_PROPERTYVALUE_READ:
            CONSOLEDEBUG("A_PROPERTYVALUE_READ");                
            if (_programmingMode) {
                processCommandPropRead(telegram);
            } 
            break;
            
        case A_PROPERTYVALUE_WRITE:
            CONSOLEDEBUG("A_PROPERTYVALUE_WRITE");   
            if (_programmingMode) {
                processCommandPropWrite(telegram);
            }
            break;
                 

        case A_RESTART:
            CONSOLEDEBUG("A_RESTART received");
            if (_programmingMode) {
                // Restart the device -> end programming mode
                setProgrammingMode(false);
                CONSOLEDEBUG("Received restart, ending programming mode"); 
            }
            break;    
            

            
        default:
            CONSOLEDEBUG("Unhandled APCI: %i", apci);
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
    
    Serial.print("Start: ");
    Serial.println(start, HEX);
    Serial.print("Length: ");
    Serial.println(length);
    
    
    byte data[length];
    
    for(int i=0;i<length;i++){
        Serial.print("Read from eeprom: ");
        data[i] = EEPROM.read(start+i);
        Serial.print(start+i);
        Serial.print(" -> ");
        Serial.println(data[i],HEX);
    }
    
    bool result = _knxTpUart->sendMemoryReadResponse(start, length, data, seqNr, area, line, member);
    
    Serial.print("Result=");
    Serial.println(result);
    
}


void KnxDevice::processCommandMemWrite(KnxTelegram* telegram) {
    // dump telegram data to see structure
    telegram->print(&Serial);
    
    unsigned int start = telegram->getMemoryStart();
    int length = telegram->getMemoryLength();
    byte data[length];
    telegram->getMemoryData(data);
    
    Serial.print("Start: ");
    Serial.println(start, HEX);
    Serial.print("Length: ");
    Serial.println(length);
    
    for(int i=0;i<length;i++){
        Serial.print("data[");
        Serial.print(i);
        Serial.print("]=");
        Serial.println(data[i], HEX);
        
        Serial.print("Writing to eeprom: ");
        Serial.println(start+i);
        
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

