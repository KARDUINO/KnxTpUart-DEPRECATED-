#ifndef KnxTelegram_h
#define KnxTelegram_h

#include "Arduino.h"

#define MAX_KNX_TELEGRAM_SIZE 23
#define KNX_TELEGRAM_HEADER_SIZE 6

#define TPUART_SERIAL_CLASS Stream

// KNX priorities
enum KnxPriorityType {
    KNX_PRIORITY_SYSTEM = B00,
    KNX_PRIORITY_ALARM = B10,
    KNX_PRIORITY_HIGH = B01,
    KNX_PRIORITY_NORMAL = B11
};

// KNX commands / APCI Coding
// see: http://www.mikrocontroller.net/attachment/151008/KNX_Twisted_Pair_Protokollbeschreibung.pdf
// Octet 6, Bit 2+1
enum KnxCommandType {
    KNX_COMMAND_READ                     = B0000, // Multicast
    KNX_COMMAND_ANSWER                   = B0001, // "
    KNX_COMMAND_WRITE                    = B0010, // "

    KNX_COMMAND_INDIVIDUAL_ADDR_WRITE    = B0011, // Broadcast
    KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST  = B0100, // "
    KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE = B0101, // "
    
    KNX_COMMAND_ADC_READ                 = B0110, // one-to-one connection-oriented
    KNX_COMMAND_ADC_ANSWER               = B0111, // "
    KNX_COMMAND_MEM_READ                 = B1000, // "
    KNX_COMMAND_MEM_ANSWER               = B1001, // "
    KNX_COMMAND_MEM_WRITE                = B1010, // "
    
    KNX_COMMAND_MASK_VERSION_READ        = B1100,
    KNX_COMMAND_MASK_VERSION_RESPONSE    = B1101,
    KNX_COMMAND_RESTART                  = B1110,
    KNX_COMMAND_ESCAPE                   = B1111
};

// Extended (escaped) KNX commands
// requires KNX_COMMAND_ESCAPE
// see: http://www.mikrocontroller.net/attachment/151008/KNX_Twisted_Pair_Protokollbeschreibung.pdf
// Octet 7,
enum KnxExtendedCommandType {
    KNX_EXT_COMMAND_PROP_READ        = B010101, 
    KNX_EXT_COMMAND_PROP_ANSWER      = B010110,
    KNX_EXT_COMMAND_PROP_WRITE       = B010111,
    KNX_EXT_COMMAND_PROP_DESC_READ   = B011000,
    KNX_EXT_COMMAND_PROP_DESC_ANSWER = B011001,
    KNX_EXT_COMMAND_AUTH_REQUEST     = B010001,
    KNX_EXT_COMMAND_AUTH_RESPONSE    = B010010 
};

enum ApplicationControlField {

    A_UNKNOWN = -1,

    A_GROUPVALUE_READ           = 1,
    A_GROUPVALUE_RESPONSE       = 2,
    A_GROUPVALUE_WRITE          = 3,
    
    A_PHYSICALADDRESS_READ      = 4,
    A_PHYSICALADDRESS_RESPONSE  = 5,
    A_PHYSICALADDRESS_WRITE     = 6,
    
    A_PROPERTYVALUE_READ        = 7,
    A_PROPERTYVALUE_RESPONSE    = 8,
    A_PROPERTYVALUE_WRITE       = 9,
    
    A_MEMORY_READ               = 10,
    A_MEMORY_RESPONSE           = 11,
    A_MEMORY_WRITE              = 12,
    
    A_DEVICEDESCRIPTOR_READ     = 13,
    A_DEVICEDESCRIPTOR_RESPONSE = 14,
    
    A_RESTART                   = 15,
    
    A_AUTHORIZE_REQUEST         = 16,
    A_AUTHORIZE_RESPONSE        = 17    
     
};

// KNX Transport Layer Communication Type
enum KnxCommunicationType {
    KNX_COMM_UDP = B00, // Unnumbered Data Packet
    KNX_COMM_NDP = B01, // Numbered Data Packet
    KNX_COMM_UCD = B10, // Unnumbered Control Data
    KNX_COMM_NCD = B11  // Numbered Control Data
};

// KNX Control Data (for UCD / NCD packets)
enum KnxControlDataType {
    KNX_CONTROLDATA_CONNECT = B00,      // UCD
    KNX_CONTROLDATA_DISCONNECT = B01,   // UCD
    KNX_CONTROLDATA_POS_CONFIRM = B10,  // NCD
    KNX_CONTROLDATA_NEG_CONFIRM = B11   // NCD
};

class KnxTelegram {
    public:
        KnxTelegram();
        
        void clear();
        void setBufferByte(int index, int content);
        int getBufferByte(int index);
        void setPayloadLength(int size);
        int getPayloadLength();
        void setRepeated(bool repeat);
        bool isRepeated();
        void setPriority(KnxPriorityType prio);
        KnxPriorityType getPriority();
        
        void setSourceAddress(byte* sourceAddress);
        int getSourceArea();
        int getSourceLine();
        int getSourceMember();
        
        void setTargetGroupAddress(byte* targetGroupAddress);
        int getTargetMainGroup();
        int getTargetMiddleGroup();
        int getTargetSubGroup();
        
        void setTargetIndividualAddress(byte* targetIndividualAddress);
        int getTargetArea();
        int getTargetLine();
        int getTargetMember();
        
        void getTarget(byte* target); // returns individualaddress target style
        void getTargetGroup(byte* target); // returns groupaddress target style
        
        bool isTargetGroup();
        bool isBroadcast();
        
        void setRoutingCounter(int counter);
        int getRoutingCounter();
        
        ApplicationControlField getApplicationControlField();
        
        void setCommand(KnxCommandType command);
        KnxCommandType getCommand();
        
        void setExtendedCommand(KnxExtendedCommandType command);
        KnxExtendedCommandType getExtendedCommand();
        
        void createChecksum();
        bool verifyChecksum();
        int getChecksum();
        void print(TPUART_SERIAL_CLASS*);
        int getTotalLength();
        KnxCommunicationType getCommunicationType();
        void setCommunicationType(KnxCommunicationType);
        
        int getSequenceNumber();
        void setSequenceNumber(int);
        
        void setControlData(KnxControlDataType);
        KnxControlDataType getControlData();

        
        // Getter+Setter for DPTs
        void setFirstDataByte(int data);
        int getFirstDataByte();
        bool getBool();
        
        void set2ByteFloatValue(float value);
        float get2ByteFloatValue();
        
        void set2ByteIntValue(float value);
        int get1ByteIntValue();
        
        void set1ByteIntValue(int value);
        float get2ByteIntValue();
        
        void set4ByteFloatValue(float value);
        float get4ByteFloatValue();
        
        void setKNXTime(int day, int hours, int minutes, int seconds);
        
        void set14ByteValue(String value);
        String get14ByteValue(String value);

        // Getter+Setter for Properties/Memory Access
//    int curr_object;
//    int curr_property; 
//    int curr_length = 2;
//    byte curr_data[curr_length];
        int getPropertyObjIndex();
        int getPropertyPropId();
        int getPropertyStart();
        int getPropertyElements();
        void getPropertyData(byte* data);
        
        int getMemoryLength();
        unsigned int getMemoryStart();
        void getMemoryData(byte* data);

    private:
        int buffer[MAX_KNX_TELEGRAM_SIZE];
        int calculateChecksum();

};

#endif

