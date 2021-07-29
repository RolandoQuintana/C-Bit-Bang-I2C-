#include "BitBang.h"

BitBang::BitBang()
{

    sda = 18;
    pinMode(sda, OUTPUT);
    digitalWriteFast(sda, HIGH);

    scl = 19;
    pinMode(scl, OUTPUT);
    digitalWriteFast(scl, HIGH);

    holdClkHighMillis = 0;
    holdClkLowMillis = 0;

}

void BitBang::SetSCL(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    scl = pin;
}

void BitBang::SetSDA(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    sda = pin;
}

uint8_t BitBang::GetSCL() {
    return scl;
}

uint8_t BitBang::GetSDA() {
    return sda;
}


void BitBang::Clock() {

        do{
        digitalWriteFast(scl, HIGH);
        } while(digitalReadFast(scl) == 0);

    //digitalWriteFast(scl, HIGH);
    if(holdClkHighMillis != 0) {
            delay(holdClkHighMillis);
        }
    else if(holdClkHighMicro != 0) {
        delayMicroseconds(holdClkHighMicro);
    }
    else {
       dlay();
    }

    digitalWriteFast(scl, LOW);
}

bool BitBang::WriteByte(uint8_t data) {
        //UTF_LOGF("\r\nSend Data: ")
    for(uint8_t i = 0; i < 8; i++){
        (data & 0x80) ? digitalWriteFast(sda, HIGH) : digitalWriteFast(sda, LOW);
        //UTF_LOGF("%d",(data & 0x80));
        data<<=1;

        if(holdClkLowMillis != 0) delay(holdClkLowMillis);
        else if(holdClkLowMicro != 0) delayMicroseconds(holdClkLowMicro);
        else dlay();

        Clock();

    }
    dlay();
    digitalWriteFast(sda, HIGH);
    digitalWriteFast(scl, HIGH);
    dlay();
    bool ack = !digitalReadFast(sda);
    digitalWriteFast(scl, LOW);
    dlay();

    //digitalWrite(sda, LOW);
    return ack;
}

void BitBang::cmdRead(uint8_t i2cAddr) {
    Start();
    WriteByte((i2cAddr<<1)|0x00); //I2C address (Write)
    delay(2);

    WriteByte(0x01);
    WriteByte(0x09);
    WriteByte(5);

    //GroupConfig Base Address
    WriteByte(0x00);
    WriteByte(0x00);
    WriteByte(0x20);
    WriteByte(0x51);

    //Data Size
    WriteByte(0x0C);
    WriteByte(0x00);
    Stop();
}

uint8_t BitBang::GetCheckSum(U8Vector_t cmd) {
    uint8_t checkSum = 0;

    for(uint8_t data : cmd)
    {
        checkSum += data;
        //UTF_LOGF("Check Sum in Adding: %d\r\n", checkSum);
    }

    return checkSum;
}

bool BitBang::Write(uint32_t addr, U8Vector_t &data, uint8_t byteStrechIndx, uint32_t high, uint32_t low, bool micros) {
    U8Vector_t cmd;
    Union32_t uExtMemAddr;
    uExtMemAddr.U32 = addr;
    Union16_t uDataSize;
    uDataSize.U16 = data.size();

    cmd.push_back(0x00); //WRITE_CMD
    cmd.push_back(0x09); //READ_WRITE_BASE
    cmd.push_back(uExtMemAddr.LSW.LSB);
    cmd.push_back(uExtMemAddr.LSW.MSB);
    cmd.push_back(uExtMemAddr.MSW.LSB);
    cmd.push_back(uExtMemAddr.MSW.MSB);
    cmd.push_back(0x0C);
    cmd.push_back(0x00);

    Start();
    WriteByte((0x2C<<1)|0x00); //I2C address (Write)
    delay(2);

    WriteByte(0x00); //WRITE_CMD
    WriteByte(0x09); //READ_WRITE_BASE

    //GroupConfig Base Address
    WriteByte(uExtMemAddr.LSW.LSB);
    WriteByte(uExtMemAddr.LSW.MSB);
    WriteByte(uExtMemAddr.MSW.LSB);
    WriteByte(uExtMemAddr.MSW.MSB);

    //Data Size
    WriteByte(uDataSize.LSB);
    WriteByte(uDataSize.MSB);

    for(int i = 0; i < data.size(); i++) {
        if(i == byteStrechIndx) {
            StretchByte(high, low, micros);
            if(!WriteByte(data[i])) return false;
            ClearStretchByte();
        }
        else if(!WriteByte(data[i])) return false;
        cmd.push_back(data[i]);
    }

    if(!WriteByte(GetCheckSum(cmd))) return false;
    Stop();
    return true;
}

bool BitBang::Read(uint32_t addr, uint8_t numBytes, U8Vector_t &data, uint8_t byteStrechIndx, uint32_t high, uint32_t low, bool micros) {
    data.clear();
    uint8_t dataByte;
    uint8_t reqCount = numBytes + 3;
    U8Vector_t packet;
    Union32_t uExtMemAddr;
    uExtMemAddr.U32 = addr;
    Union16_t uDataSize;
    uDataSize.U16 = numBytes;

    Start();
    WriteByte((0x2C<<1)|0x00); //I2C address (Write)
    delay(2);

    WriteByte(0x01); //READ_CMD
    WriteByte(0x09); //READ_WRITE_BASE

    //GroupConfig Base Address
    WriteByte(uExtMemAddr.LSW.LSB);
    WriteByte(uExtMemAddr.LSW.MSB);
    WriteByte(uExtMemAddr.MSW.LSB);
    WriteByte(uExtMemAddr.MSW.MSB);

    //Data Size
    WriteByte(uDataSize.LSB);
    WriteByte(uDataSize.MSB);
    Stop();

    delay(3);

    Start();
    WriteByte((0x2C<<1)|0x01); //I2C address (Read)

    while(packet.size() <= reqCount - 1) {
        if(packet.size() == reqCount - 1) {
            packet.push_back(ReadByte(false, true));
        }
        else if(packet.size() == byteStrechIndx + 2) {
            StretchByte(high, low, micros);
            packet.push_back(ReadByte(true, false));
            ClearStretchByte();
        }

        else packet.push_back(ReadByte(true, false));
    }

    if(!packet.empty() && VerifyResponse(packet, numBytes))
    {

        data.insert(data.begin(), (packet.begin() + 2), (packet.end() - 1));
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t BitBang::ReadBit() {

    uint8_t b;

    if(holdClkLowMillis != 0) {
            delay(holdClkLowMillis);
        }
        else if(holdClkLowMicro != 0) {
            delayMicroseconds(holdClkLowMicro);
        }
        else {
            dlay();
        }

    digitalWriteFast(sda, HIGH);




    //if(digitalReadFast(sda)) b = 1;
    //else b = 0;

    //digitalWriteFast(scl, LOW);

      if(digitalReadFast(sda)) b = 1;
      else b = 0;

      Clock();

//      digitalWriteFast(scl, LOW);
//      dlay();
//      digitalWriteFast(scl, HIGH);
//      dlay();
//      if(digitalReadFast(sda)) b = 1;
//      else b = 0;
//      dlay();
//
//      digitalWriteFast(sda, HIGH);
//      dlay();

    return b;
}

uint8_t BitBang::ReadByte(bool ack, bool stop) {

    uint8_t B = 0;

    uint8_t i;
    for( i = 0; i <= 7; i++ )
    {
        B <<= 1;
        B |= ReadBit();
    }

    //digitalWriteFast(sda, LOW);
    if( ack ) digitalWriteFast(sda, LOW);
    else digitalWriteFast(sda, HIGH);

    dlay();
    digitalWriteFast(scl, HIGH);
    dlay();
    digitalWriteFast(scl, LOW);

    //digitalWriteFast(sda, HIGH);

    if( stop ) {
        dlay();
        Stop();
        dlay();
    }

    delayMicroseconds(7);
    //UTF_LOGF("Read Byte: 0x%X\r\n", B);
    return B;
}

void BitBang::Start(){
    digitalWriteFast(sda, HIGH);
    dlay();
    digitalWriteFast(scl,HIGH);
    dlay();
    digitalWriteFast(sda, LOW);
    dlay();
    digitalWriteFast(scl, LOW);
    dlay();

}

void BitBang::Stop(){
    digitalWriteFast(sda, LOW);
    dlay();
    digitalWriteFast(scl, HIGH);
    dlay();
    digitalWriteFast(sda, HIGH);
    dlay();
}

void BitBang::TurnOffClockHoldsMillis() {
    holdClkHighMillis = 0;
    holdClkLowMillis = 0;
}

void BitBang::TurnOffClockHoldsMicros() {
    holdClkHighMicro = 0;
    holdClkLowMicro = 0;
}

void BitBang::SetHoldClkLowMillis(uint32_t mills) {
    holdClkLowMicro = 0;
    holdClkLowMillis = mills;
}

void BitBang::SetHoldClkHighMillis(uint32_t mills) {
    holdClkHighMicro = 0;
    holdClkHighMillis = mills;
}

void BitBang::SetHoldClkLowMicros(uint32_t micros) {
    holdClkLowMillis = 0;
    holdClkLowMicro = micros;
}

void BitBang::SetHoldClkHighMicros(uint32_t micros) {
    holdClkHighMillis = 0;
    holdClkHighMicro = micros;
}

void BitBang::StretchByte(uint32_t high, uint32_t low, bool micros) {

    if(micros) {
        SetHoldClkHighMicros(high);
        SetHoldClkLowMicros(low);
    }
    else {
        SetHoldClkHighMillis(high);
        SetHoldClkLowMillis(low);
    }

}

void BitBang::ClearStretchByte() {
    TurnOffClockHoldsMicros();
    TurnOffClockHoldsMillis();
}

bool BitBang::VerifyResponse(U8Vector_t &packet, uint16_t numBytes) {
    bool result = true;
    Union16_t dataSize;
    uint8_t checkSum = 0;
    dataSize.LSB = packet[0];
    dataSize.MSB = packet[1];

    result = result && (packet.size() == numBytes + 3);
    result = result && (dataSize.U16 == numBytes + 3);

    //checkSum = GetCheckSum(packet);
    for(uint16_t i = 0; i <= (packet.size() - 1); i++)
    {
        checkSum += packet[i];
        //UTF_LOGF("adding checksum: 0x%X\r\n", packet[i]);
    }
    checkSum -= packet[packet.size() - 1];

//    for(uint8_t data : packet)
//    {
//        checkSum += data;
//        UTF_LOGF("Check Sum in Adding: %d\r\n", checkSum);
//    }
    //UTF_LOGF("Check Sum in Verify: 0x%X\r\n", checkSum);
    result = result && (checkSum == packet[packet.size() - 1]);
    //UTF_LOGF("Packet end: %d\r\n", packet[packet.size() - 1]);

    return result;
}




