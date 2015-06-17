#include "KnxTpUart.h"
#include "KnxTelegram.h"

KnxTelegram::KnxTelegram() {
    clear();
}

void KnxTelegram::clear() {
    for (int i = 0; i < MAX_KNX_TELEGRAM_SIZE; i++) {
        buffer[i] = 0;
    }

    // Control Field, Normal Priority, No Repeat
    buffer[0] = B10111100;

    // Target Group Address, Routing Counter = 6, Length = 1 (= 2 Bytes)
    buffer[5] = B11100001;
}

int KnxTelegram::getBufferByte(int index) {
    return buffer[index];
}

void KnxTelegram::setBufferByte(int index, int content) {
    buffer[index] = content;
}

bool KnxTelegram::isRepeated() {
    // Parse Repeat Flag
    if (buffer[0] & B00100000) {
        return false;
    } else {
        return true;
    }
}

void KnxTelegram::setRepeated(bool repeat) {
    if (repeat) {
        buffer[0] = buffer[0] & B11011111;
    } else {
        buffer[0] = buffer[0] | B00100000;
    }
}

void KnxTelegram::setPriority(KnxPriorityType prio) {
    buffer[0] = buffer[0] & B11110011;
    buffer[0] = buffer[0] | (prio << 2);
}

KnxPriorityType KnxTelegram::getPriority() {
    // Priority
    return (KnxPriorityType) ((buffer[0] & B00001100) >> 2);
}

void KnxTelegram::setSourceAddress(byte sourceAddress[2]) {
    buffer[1] = sourceAddress[0];
    buffer[2] = sourceAddress[1];
}

int KnxTelegram::getSourceArea() {
    return (buffer[1] >> 4);
}

int KnxTelegram::getSourceLine() {
    return (buffer[1] & B00001111);
}

int KnxTelegram::getSourceMember() {
    return buffer[2];
}

void KnxTelegram::setTargetGroupAddress(byte targetGroupAddress[2]) {
    buffer[3] = targetGroupAddress[0];
    buffer[4] = targetGroupAddress[1];
    buffer[5] = buffer[5] | B10000000;
}

void KnxTelegram::setTargetIndividualAddress(byte targetIndividualAddress[2]) {
    buffer[3] = targetIndividualAddress[0];
    buffer[4] = targetIndividualAddress[1];
    buffer[5] = buffer[5] & B01111111;
}

/*
 * Is the target a GA? If not, it's a PA
 * Octet 5 Bit 8, Transport Control Field
 */
bool KnxTelegram::isTargetGroup() {
    return buffer[5] & B10000000;
}

bool KnxTelegram::isBroadcast() {
    return isTargetGroup() && buffer[3] == 0 && buffer[4] == 0;
}

/*
 * Returns target address as 2bytes
 * Depends on "isTargetGroup" how to interpret it: GA or PA
 */
void KnxTelegram::getTarget(byte target[2]) {
    target[0] = buffer[3];
    target[1] = buffer[4];
}

int KnxTelegram::getTargetMainGroup() {
    return ((buffer[3] & B01111000) >> 3);
}

int KnxTelegram::getTargetMiddleGroup() {
    return (buffer[3] & B00000111);
}

int KnxTelegram::getTargetSubGroup() {
    return buffer[4];
}

int KnxTelegram::getTargetArea() {
    return ((buffer[3] & B11110000) >> 4);
}

int KnxTelegram::getTargetLine() {
    return (buffer[3] & B00001111);
}

int KnxTelegram::getTargetMember() {
    return buffer[4];
}

void KnxTelegram::setRoutingCounter(int counter) {
    buffer[5] = buffer[5] & B10000000;
    buffer[5] = buffer[5] | (counter << 4);
}

int KnxTelegram::getRoutingCounter() {
    return ((buffer[5] & B01110000) >> 4);
}

void KnxTelegram::setPayloadLength(int length) {
    buffer[5] = buffer[5] & B11110000;
    buffer[5] = buffer[5] | (length - 1);
}

int KnxTelegram::getPayloadLength() {
    int length = (buffer[5] & B00001111) + 1;
    return length;
}

ApplicationControlField KnxTelegram::getApplicationControlField() {

    ApplicationControlField result = A_UNKNOWN;

    // first four bits: octet 6 bit 1+2 PLUS octet 7 bit 7+8
    int bits4 = ((buffer[6] & B00000011) << 2) | ((buffer[7] & B11000000) >> 6);
    // following 6 bits: octet 7 bit 1-7
    int bits6 = (buffer[7] & B00111111);

    switch (bits4) {

        // GROUP
        case B0000:    
            result = A_GROUPVALUE_READ;
            break;
        case B0001:
            result = A_GROUPVALUE_RESPONSE;
            break;
        case B0010:
            result = A_GROUPVALUE_WRITE;
            break;
            
        // PHYSICAL
        case B0011:
            if (bits6 == B000000) result = A_PHYSICALADDRESS_WRITE;
            break;
        case B0100:
            if (bits6 == B000000) result = A_PHYSICALADDRESS_READ;
            break;
        case B0101:
            if (bits6 == B000000) result = A_PHYSICALADDRESS_RESPONSE;
            break;
            
        case B1111:
            // PROPERTY    
            if (bits6 == B010101) result = A_PROPERTYVALUE_READ;
            else
            if (bits6 == B010110) result = A_PROPERTYVALUE_RESPONSE;
            else
            if (bits6 == B010111) result = A_PROPERTYVALUE_WRITE;
            else
            // AUTHORIZE
            if (bits6 == B010001) result = A_AUTHORIZE_REQUEST;
            else
            if (bits6 == B010010) result = A_AUTHORIZE_RESPONSE;
            break;
            
        // MEMORY    
        case B1000:
            if ((bits6 & B110000) == B00) result = A_MEMORY_READ;
            break;            
        case B1001:
            if ((bits6 & B110000) == B00) result = A_MEMORY_RESPONSE;
            break;            
        case B1010:
            if ((bits6 & B110000) == B00) result = A_MEMORY_WRITE;
            break;            

        // DEVICE DESCRIPTOR    
        case B1100:
            if (bits6 == B000000) result = A_DEVICEDESCRIPTOR_READ;
            break;
        case B1101:
            if (bits6 == B000000) result = A_DEVICEDESCRIPTOR_RESPONSE;
            break;
            
        // RESTART    
        case B1110:
            if (bits6 == B000000) result = A_RESTART;
            
		default:
			DEBUG_TRACE("Unhandled ApplicationControlField: 4bit: %02x 6bit: %02x", bits4, bits6);
			break;

    }
            
    return result;
}

void KnxTelegram::setCommand(KnxCommandType command) {
    buffer[6] = buffer[6] & B11111100; // erase first two bits
    buffer[7] = buffer[7] & B00111111; // erase last two bits

    buffer[6] = buffer[6] | (command >> 2); // Command first two bits
    buffer[7] = buffer[7] | (command << 6); // Command last two bits
}

KnxCommandType KnxTelegram::getCommand() {
    return (KnxCommandType) (((buffer[6] & B00000011) << 2) | ((buffer[7] & B11000000) >> 6));
}

void KnxTelegram::setExtendedCommand(KnxExtendedCommandType extCommand) {
    buffer[7] = buffer[7] & B11000000; // erase last six bits
    buffer[7] = buffer[7] | (extCommand >> 6); // ExtCommand first six bits
}

KnxExtendedCommandType KnxTelegram::getExtendedCommand() {
    return (KnxExtendedCommandType) (buffer[7] & B00111111); // get only first six bits
}

void KnxTelegram::setControlData(KnxControlDataType cd) {
    buffer[6] = buffer[6] & B11111100;
    buffer[6] = buffer[6] | cd;
}

/*
 * Octet 6, part of "Transport Control Field"
 */
KnxControlDataType KnxTelegram::getControlData() {
    return (KnxControlDataType) (buffer[6] & B00000011);
}

KnxCommunicationType KnxTelegram::getCommunicationType() {
    return (KnxCommunicationType) ((buffer[6] & B11000000) >> 6);
}

void KnxTelegram::setCommunicationType(KnxCommunicationType type) {
    buffer[6] = buffer[6] & B00111111;
    buffer[6] = buffer[6] | (type << 6);
}

int KnxTelegram::getSequenceNumber() {
    return (buffer[6] & B00111100) >> 2;
}

void KnxTelegram::setSequenceNumber(int number) {
    buffer[6] = buffer[6] & B11000011;
    buffer[6] = buffer[6] | (number << 2);
}

void KnxTelegram::setFirstDataByte(int data) {
    buffer[7] = buffer[7] & B11000000;
    buffer[7] = buffer[7] | data;
}

int KnxTelegram::getFirstDataByte() {
    return (buffer[7] & B00111111);
}

void KnxTelegram::createChecksum() {
    int checksumPos = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;
    buffer[checksumPos] = calculateChecksum();
}

int KnxTelegram::getChecksum() {
    int checksumPos = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;
    return buffer[checksumPos];
}

bool KnxTelegram::verifyChecksum() {
    int calculatedChecksum = calculateChecksum();
    return (getChecksum() == calculatedChecksum);
}

void KnxTelegram::print(TPUART_SERIAL_CLASS* serial) {
#if defined(DEBUGLEVEL_TRACE)
    serial->println("##### DUMP START #####");

    for (int i = 0; i < MAX_KNX_TELEGRAM_SIZE; i++) {
        serial->print("buffer[");
        serial->print(i);
        serial->print("] hex=");
        serial->print(buffer[i], HEX);
        serial->print(" \tbin=");
        serial->println(buffer[i], BIN);
    }

    serial->print("Repeated: ");
    serial->println(isRepeated());

    serial->print("Priority: ");
    serial->println(getPriority());

    serial->print("Source: ");
    serial->print(getSourceArea());
    serial->print(".");
    serial->print(getSourceLine());
    serial->print(".");
    serial->println(getSourceMember());

    if (isTargetGroup()) {
        serial->print("Target Group: ");
        serial->print(getTargetMainGroup());
        serial->print("/");
        serial->print(getTargetMiddleGroup());
        serial->print("/");
        serial->println(getTargetSubGroup());
    } else {
        serial->print("Target Physical: ");
        serial->print(getTargetArea());
        serial->print(".");
        serial->print(getTargetLine());
        serial->print(".");
        serial->println(getTargetMember());
    }
        
    serial->print("Control Data: bin=");
    serial->println(getControlData(), BIN);

    serial->print("Routing Counter: ");
    serial->println(getRoutingCounter());

    serial->print("Payload Length: ");
    serial->println(getPayloadLength());   

    serial->print("Command: hex=");
    serial->print(getCommand(), HEX);
    serial->print(" bin=");
    serial->println(getCommand(), BIN);

    serial->print("First Data Byte  hex=");
    serial->print(getFirstDataByte(), HEX);
    serial->print("\tbin=");
    serial->println(getFirstDataByte(), BIN);


    for (int i = 2; i < getPayloadLength(); i++) {
        serial->print("Data Byte ");
        serial->print(i);
        serial->print("      hex=");
        serial->print(buffer[6+i], HEX);
        serial->print("\tbin=");
        serial->println(buffer[6+i], BIN);
    }


    if (verifyChecksum()) {
        serial->println("Checksum matches");
    } else {
        serial->println("Checksum mismatch");
        serial->println(getChecksum(), BIN);
        serial->println(calculateChecksum(), BIN);
    }
    serial->println("##### DUMP END #####");
#endif
}

int KnxTelegram::calculateChecksum() {
    int bcc = 0xFF;
    int size = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;

    for (int i = 0; i < size; i++) {
        bcc ^= buffer[i];
    }

    return bcc;
}

int KnxTelegram::getTotalLength() {
    return KNX_TELEGRAM_HEADER_SIZE + getPayloadLength() + 1;
}

/*
 * DPT 1
 * 1 bit
 */
bool KnxTelegram::getBool() {
    if (getPayloadLength() != 2) {
        // Wrong payload length
        return 0;
    }

    return(getFirstDataByte() & B00000001);
}

/*
 * DPT 3
 * 3 bit controlled
 * 3 bit
 */
/*byte KnxTelegram::get3Bit() {
    if (getPayloadLength() != 2) {
        // Wrong payload length
        return 0;
    }

    return(getFirstDataByte() & B00001111);
}
*/
/*
 * DPT 4 / DPT 5
 */
void KnxTelegram::set1ByteIntValue(int value) {
    setPayloadLength(3);
    buffer[8]=value;
}

/*
 * DPT 4 / DPT 5
 */
int KnxTelegram::get1ByteIntValue() {
    if (getPayloadLength() != 3) {
        // Wrong payload length
        return 0;
    }

    return(buffer[8]);
}

/*
 * DPT 9
 * 2 byte float value
 * 2 byte
 */
void KnxTelegram::set2ByteFloatValue(float value) {
    setPayloadLength(4);

    float v = value * 100.0f;
    int exponent = 0;
    for (; v < -2048.0f; v /= 2) exponent++;
    for (; v > 2047.0f; v /= 2) exponent++;
    long m = round(v) & 0x7FF;
    short msb = (short) (exponent << 3 | m >> 8);
    if (value < 0.0f) msb |= 0x80;
    buffer[8] = msb;
    buffer[9] = (byte)m;
}

/*
 * DPT 9
 * 2 byte float value
 * 2 byte
 */
float KnxTelegram::get2ByteFloatValue() {
    if (getPayloadLength() != 4) {
        // Wrong payload length
        return 0;
    }

    int exponent = (buffer[8] & B01111000) >> 3;
    int mantissa = ((buffer[8] & B00000111) << 8) | (buffer[9]);

    int sign = 1;

    if (buffer[8] & B10000000) {
        sign = -1;
    }

    return (mantissa * 0.01) * pow(2.0, exponent);
}

/*
 * DPT 14
 * 4 byte float value
 * 4 byte
 */
void KnxTelegram::set4ByteFloatValue(float value) {
  setPayloadLength(6);

  byte b[4];  
  float *f = (float*)(void*)&(b[0]);
  *f=value;

  buffer[8+3]=b[0];
  buffer[8+2]=b[1];
  buffer[8+1]=b[2];
  buffer[8+0]=b[3];
}

/*
 * DPT 14
 * 4 byte float value
 * 4 byte
 */
float KnxTelegram::get4ByteFloatValue() {
    if (getPayloadLength() != 6) {
        // Wrong payload length
        return 0;
    }
  byte b[4];
  b[0]=buffer[8+3];
  b[1]=buffer[8+2];
  b[2]=buffer[8+1];
  b[3]=buffer[8+0];
  float *f=(float*)(void*)&(b[0]);
  float  r=*f;
  return r;
}

/*
 * DPT 16
 * Character string
 * 14 byte
 */
void KnxTelegram::set14ByteValue(String value) {
  // load definieren
  char _load[15];
  
  // load mit space leeren/initialisieren
  for (int i=0; i<14; ++i)
  {_load[i]= 0;}
  setPayloadLength(16);
  //mache aus Value das CharArray
  value.toCharArray(_load,15); // muss 15 sein - weil mit 0 abgeschlossen wird
  buffer[8+0]=_load [0];
  buffer[8+1]=_load [1];
  buffer[8+2]=_load [2];
  buffer[8+3]=_load [3];
  buffer[8+4]=_load [4];
  buffer[8+5]=_load [5];
  buffer[8+6]=_load [6];
  buffer[8+7]=_load [7];
  buffer[8+8]=_load [8];
  buffer[8+9]=_load [9];
  buffer[8+10]=_load [10];
  buffer[8+11]=_load [11];
  buffer[8+12]=_load [12];
  buffer[8+13]=_load [13];
}

/*
 * DPT 16
 * Character string
 * 14 byte
 */
String KnxTelegram::get14ByteValue(String value) {
if (getPayloadLength() != 16) {
        // Wrong payload length
        return "";
    }
    char _load[15];
    _load[0]=buffer[8+0];
    _load[1]=buffer[8+1];
    _load[2]=buffer[8+2];
    _load[3]=buffer[8+3];
    _load[4]=buffer[8+4];
    _load[5]=buffer[8+5];
    _load[6]=buffer[8+6];
    _load[7]=buffer[8+7];
    _load[8]=buffer[8+8];
    _load[9]=buffer[8+9];
    _load[10]=buffer[8+10];
    _load[11]=buffer[8+11];
    _load[12]=buffer[8+12];
    _load[13]=buffer[8+13];
    return (_load); 
}

void KnxTelegram::setKNXTime(int day, int hours, int minutes, int seconds) {
    // Payload (3 byte) + 2
    setPayloadLength(5);

    // Day um 5 byte nach links verschieben
    day = day << 5;
    // Buffer[8] füllen: die ersten 3 Bits day, die nächsten 5 hour
    buffer[8] = (day & B11100000) + (hours & B00011111);

    // buffer[9] füllen: 2 bits leer dann 6 bits für minuten
    buffer[9] =  minutes & B00111111;
    
    // buffer[10] füllen: 2 bits leer dann 6 bits für sekunden
    buffer[10] = seconds & B00111111;
}

/*
 * Property / Memory Access stuff
 */

int KnxTelegram::getPropertyObjIndex(){
    return buffer[8];
}

int KnxTelegram::getPropertyPropId() {
    return buffer[9];
}

int KnxTelegram::getPropertyElements() {
    //asdu[2] = (byte) ((elements << 4) | ((start >>> 8) & 0xF));
    return buffer[10]>>4;
}

int KnxTelegram::getPropertyStart() {
    return (buffer[10]&0x0F)<<8 | buffer[11];
}

void KnxTelegram::getPropertyData(byte data[]) {
    for (int i=0;i<getPropertyElements();i++){
        data[i] = buffer[12+i];
    }
}

int KnxTelegram::getMemoryLength() {
    return getFirstDataByte();
}

unsigned int KnxTelegram::getMemoryStart() {
    return (buffer[8] << 8 | buffer[9]) & 0xffff;
}

void KnxTelegram::getMemoryData(byte data[]) {
    for (int i=0;i<getMemoryLength();i++){
        data[i] = buffer[10+i];
    }
}



