#include <Arduino.h>
#include <EEPROM.h>

// für den KNX Zugriff
#include <KnxTelegram.h>
#include <KnxTpUart.h>
#include <KnxDevice.h>



// Start with default PA
KnxTpUart knx(&Serial1, PA_INTEGER(15,15,20));
KnxDevice knxDevice(&knx);


/* *******************************************
 * SETUP
 */
void setup() {

    // On Leonardo/Micro:
    // Serial = Debug RS232 Port
    // Serial1 = KNX BTI

    Serial.begin(9600);  

    // Setup serial port for KNX BTI
    Serial1.begin(19200);
    UCSR1C = UCSR1C | B00100000; // Even Parity
    knx.uartReset(); 
    
}



/* *******************************************
 * Main Program Loop
 */
void loop(){
   
   knxDevice.loop();
}





/* *******************************************
 * Handle serial KNX events
 */
void serialEvent1() {

    // forward the event processing to KNX Software Device
    KnxTpUartSerialEventType knxEventType = knxDevice.serialEvent();

    switch(knxEventType) {

    case KNX_TELEGRAM:     
        Serial.println("it's a KNX telegram...");
        break;
      
    default:
        break;

    }
}

// EOF


