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
    bool _programmingMode;
    int _lastProgButtonValue;

    KnxTpUart* _knxTpUart;

    void setProgrammingMode(bool on);
    void processTelegram();
    void processCommandMemRead(KnxTelegram* telegram);
    void processCommandMemWrite(KnxTelegram* telegram);

    void processCommandPropRead(KnxTelegram* telegram);
    void processCommandPropWrite(KnxTelegram* telegram);
};



#endif
