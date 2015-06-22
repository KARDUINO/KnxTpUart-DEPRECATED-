#ifndef KnxTpUart_h
#define KnxTpUart_h

#include "HardwareSerial.h"
#include "Arduino.h"

#include "KnxTelegram.h"

// --------------------------------------------
// Debug-Stuff

// enable/disable log-levels

// --------------------------------------------


// See http://www.hqs.sbt.siemens.com/cps_product_data/gamma-b2b/tpuart.pdf , Page 12

// Services from TPUART (means: services we are receiving from BCU), Chapter of above PDF: 3.2.3.2 Services from UART
#define TPUART_RESET_INDICATION_BYTE B11

// Services to TPUART (means: services we are sending to BCU), Chapter of above PDF: 3.2.3.1 Services to UART
#define TPUART_DATA_START_CONTINUE  B10000000
#define TPUART_DATA_END             B01000000




// Debugging
// uncomment the following line to enable debugging
//#define TPUART_DEBUG true
//#define TPUART_DEBUG_PORT Serial

#define TPUART_SERIAL_CLASS Stream

// Delay in ms between sending of packets to the bus
// set to 0 if you keep care of that by yourself
#define SERIAL_WRITE_DELAY_MS 0

// Timeout for reading a byte from TPUART
#define SERIAL_READ_TIMEOUT_MS 300

// Maximum number of group addresses that can be listened on
#define MAX_LISTEN_GROUP_ADDRESSES 48

// Macros for converting PA and GA to 2-byte
#define PA_INTEGER(area, line, member) (byte*)(const byte[]){(area << 4) | line, member}
#define PA_STRING(address) (byte*)(const byte[]){(String(address).substring(0, String(address).indexOf('.')).toInt() << 4) | String(address).substring(String(address).indexOf('.')+1, String(address).lastIndexOf('.')).toInt(), String(address).substring(String(address).lastIndexOf('.')+1,String(address).length()).toInt()}

#define GA_ARRAY(area, line, member) {((area << 3) | line), member}
#define GA_INTEGER(area, line, member) (byte*)(const byte[]){(area << 3) | line, member}
#define GA_STRING(address) (byte*)(const byte[]){(String(address).substring(0, String(address).indexOf('/')).toInt() << 3) | String(address).substring(String(address).indexOf('/')+1, String(address).lastIndexOf('/')).toInt(), String(address).substring(String(address).lastIndexOf('/')+1,String(address).length()).toInt()}


enum KnxTpUartSerialEventType {
    TPUART_RESET_INDICATION,
    KNX_TELEGRAM,
    IRRELEVANT_KNX_TELEGRAM,
    UNKNOWN
};

class KnxTpUart {
public:
    KnxTpUart(TPUART_SERIAL_CLASS*, byte*);
    void uartReset();
    void uartStateRequest();
    KnxTpUartSerialEventType serialEvent();
    KnxTelegram* getReceivedTelegram(); 

    void setIndividualAddress(byte*);
    void getIndividualAddress(byte address[2]);
    
    void sendAck();
    void sendNotAddressed();
    
    bool groupWriteBool(byte* groupAddress, bool);
    bool groupWrite2ByteFloat(byte* groupAddress, float);
    bool groupWrite1ByteInt(byte* groupAddress, int);
    bool groupWrite2ByteInt(byte* groupAddress, int);
    bool groupWrite4ByteFloat(byte* groupAddress, float);
    bool groupWrite14ByteText(byte* groupAddress, String);

    bool groupAnswerBool(byte* groupAddress, bool);
    bool groupAnswer2ByteFloat(byte* groupAddress, float);
    bool groupAnswer1ByteInt(byte* groupAddress, int);
    bool groupAnswer2ByteInt(byte* groupAddress, int);
    bool groupAnswer4ByteFloat(byte* groupAddress, float);
    bool groupAnswer14ByteText(byte* groupAddress, String);
    
    bool groupWriteTime(byte* groupAddress, int, int, int, int);
    
    void addListenGroupAddress(byte* groupAddress);  
    bool isListeningToGroupAddress(byte* groupAddress);
    
    bool sendPhysicalAddressResponse();
    bool sendDeviceDescriptorResponse(int area, int line, int member, int descriptorType, byte* deviceDescData);
    bool sendAuthResponse(int, int, int, int, int);

    bool sendPropertyResponse(int objIndex, int propId, int start, int elements, byte* data, int sequenceNo, int area, int line, int member);
    bool sendMemoryReadResponse(int start, int length, byte* data, int seqNr, int area, int line, int member);

    void setListenToBroadcasts(bool);
    
    void groupBytesToInt(byte*, int*);
    
    
private:
    Stream* _serialport;
    KnxTelegram* _tg;       // for normal communication
    KnxTelegram* _tg_reply;   // for reply communication
    byte _individualAddress[2];
    byte _listen_group_addresses[MAX_LISTEN_GROUP_ADDRESSES][2];
    byte _listen_group_address_count;
    bool _listen_to_broadcasts;
    
    bool isKNXControlByte(int);
    void checkErrors();
    void printByte(int);
    bool readKNXTelegram();
    void createKNXMessageFrame(int, KnxCommandType, byte* targetGroupAddress, int);
    void createKNXMessageFrameIndividualReply(int, KnxCommandType, byte* targetIndividualAddress, int);
    bool sendMessage(KnxTelegram* telegram);
    bool sendTransportDataAckPDU(int, byte* targetIndividualAddress);
    int serialRead();

};



#endif

