#ifndef KnxDevice_h
#define KnxDevice_h


#include "Arduino.h"

#include "KnxTpUart.h"

class KnxDevice {
public:
    KnxDevice(KnxTpUart*, bool authRequired);
    KnxTpUartSerialEventType serialEvent();
    
    void loop();
    
private:
    bool _programmingMode;
    int _lastProgButtonValue;
    // Flag that sets "authorization required for access"
    bool _authRequired;
    
    // current authorization status
    bool _authorized;

    KnxTpUart* _knxTpUart;

    void setProgrammingMode(bool on);
    void processTelegram();
    void processCommandMemRead(KnxTelegram* telegram);
    void processCommandMemWrite(KnxTelegram* telegram);

    void processCommandPropRead(KnxTelegram* telegram);
    void processCommandPropWrite(KnxTelegram* telegram);
};



#endif
