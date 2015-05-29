#include "KnxDevice.h"
#include <EEPROM.h>

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
}

void KnxDevice::loop() {
    // prog switch button
    int button = digitalRead(PIN_PROG_BUTTON);
    if (button != _lastProgButtonValue) {
        CONSOLEDEBUG("ProgButton: %i", button);
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
            CONSOLEDEBUG("it's a KNX telegram...");
            processTelegram();
            break;
          
        default:
            break;

    }

    return knxEventType;
}

/* *******************************************
 * Processes KNX telegrams: Check for the command and execute what ever is needed to process it
 */
void KnxDevice::processTelegram() {

    KnxTelegram* telegram = _knxTpUart->getReceivedTelegram();
    CONSOLEDEBUG("-> Telegram command: ");
#if defined(DEBUG)
    Serial.println(telegram->getCommand(), BIN);
#endif

    // Check which kind of telegram it is
    switch(telegram->getCommand()) {

        // someone wants to read data (from us?)
        case KNX_COMMAND_READ:
//            CONSOLEDEBUG("--> KNX command read");
            break;

        // someone is sending data (to us?)
        case KNX_COMMAND_WRITE: 
//            CONSOLEDEBUG("--> KNX command write");        
            break;

        // someone has answered a read request (ours?)
        case KNX_COMMAND_ANSWER:
//            CONSOLEDEBUG("--> KNX command answer");        
            break;

        // someone wants to set a PA
        case KNX_COMMAND_INDIVIDUAL_ADDR_WRITE:
            if (_programmingMode && telegram->isTargetGroup()) {
            
                int area = (telegram->getBufferByte(8) & B11110000) >> 4;
                int line = telegram->getBufferByte(8) & B00001111;
                int member = telegram->getBufferByte(9);
                
                CONSOLEDEBUG("--> KNX_COMMAND_INDIVIDUAL_ADDR_WRITE: %i.%i.%i", area, line, member);                
                
                _knxTpUart->setIndividualAddress(PA_INTEGER(area, line, member));
                
                byte individualAddress[2];
                _knxTpUart->getIndividualAddress(individualAddress);

                // store in eeprom
                EEPROM.write(EEPROM_INDEX_PA, individualAddress[0]);
                EEPROM.write(EEPROM_INDEX_PA+1, individualAddress[1]);
                
            }
            break;

        case KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST:
//            CONSOLEDEBUG("KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST");
            if (_programmingMode) {
                // Broadcast request for individual addresses of all devices in programming mode
                CONSOLEDEBUG("KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST answering ...");
		        // Send our answer back to sender
                _knxTpUart->individualAnswerAddress(); 
            }
            break;

        case KNX_COMMAND_MASK_VERSION_READ:
            CONSOLEDEBUG("KNX_COMMAND_MASK_VERSION_READ");
            // Request for mask version (version of bus interface)
            _knxTpUart->individualAnswerMaskVersion(telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
            break;

        // received command is of type "extended"
        case KNX_COMMAND_ESCAPE:
//            CONSOLEDEBUG("KNX_COMMAND_ESCAPE");
            switch (telegram->getExtendedCommand()) {
            
                case KNX_EXT_COMMAND_AUTH_REQUEST:
//                    CONSOLEDEBUG("KNX_EXT_COMMAND_AUTH_REQUEST");
                    if (_programmingMode) {
                        CONSOLEDEBUG("KNX_EXT_COMMAND_AUTH_REQUEST answering ...");
                        // Authentication request to allow memory access
                        _knxTpUart->individualAnswerAuth(15 /* access level */, telegram->getSequenceNumber(), telegram->getSourceArea(), telegram->getSourceLine(), telegram->getSourceMember());
                    }
                break;
                
//                case KNX_EXT_COMMAND_MEM_WRITE:
                
                    CONSOLEDEBUG("KNX_EXT_COMMAND_MEM_WRITE received");
                    processCommandMemWrite(telegram);
                break;
                
            }           
        
            break;

        case KNX_COMMAND_RESTART:
            CONSOLEDEBUG("KNX_COMMAND_RESTART received");
            if (_programmingMode) {
                // Restart the device -> end programming mode
                setProgrammingMode(false);
                CONSOLEDEBUG("Received restart, ending programming mode"); 
            }
            break;

      default:
            CONSOLEDEBUG("############# Unhandled: %i", telegram->getCommand());
            break;
    } 




}

void KnxDevice::processCommandMemWrite(KnxTelegram* telegram) {
}
