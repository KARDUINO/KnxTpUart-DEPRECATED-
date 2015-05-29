#ifndef KnxDevice_h
#define KnxDevice_h


#include "Arduino.h"


#include "KnxTpUart.h"

class KnxDevice {
public:
    KnxDevice(KnxTpUart*);
    KnxTpUartSerialEventType serialEvent();
    
    void loop();
    
private:
    bool _programmingMode = false;
    int _lastProgButtonValue = 0;

    KnxTpUart* _knxTpUart;

    void setProgrammingMode(bool on);
    void processTelegram();
    void processCommandMemWrite(KnxTelegram* telegram);

};



#endif
