class BitBang
{
public:
    BitBang(void);


    void SetSCL(uint8_t pin);
    void SetSDA(uint8_t pin);
    uint8_t GetSCL(void);
    uint8_t GetSDA(void);
    void Start(void);
    void Stop(void);
    void ClockHigh(void);
    void Clock(void);


    void cmdRead(uint8_t i2cAddr);
    bool Read(uint32_t addr, uint8_t numBytes, U8Vector_t &data, uint8_t byteStrechIndx = -3, uint32_t high = 0, uint32_t low = 0, bool micros = true);
    bool VerifyResponse(U8Vector_t &packet, uint16_t numBytes);

    bool WriteByte(uint8_t data);
    bool Write(uint32_t addr, U8Vector_t &data, uint8_t byteStrechIndx = -3, uint32_t high = 0, uint32_t low = 0, bool micros = true);

    uint8_t ReadBit(void);
    uint8_t ReadByte(bool ack, bool stop);

    uint8_t GetCheckSum(U8Vector_t cmd);


    void dlay(){ delayMicroseconds(4); }

    void TurnOffClockHoldsMillis();
    void TurnOffClockHoldsMicros();

    void SetHoldClkLowMillis(uint32_t mills);
    void SetHoldClkHighMillis(uint32_t mills);

    void SetHoldClkLowMicros(uint32_t micros);
    void SetHoldClkHighMicros(uint32_t micros);

    void StretchByte(uint32_t high, uint32_t low, bool micros);
    void ClearStretchByte();


protected:
    uint8_t sda;
    uint8_t scl;
    void sendAck();

    uint32_t holdClkLowMillis = 0;
    uint32_t holdClkHighMillis = 0;

    uint32_t holdClkLowMicro = 0;
    uint32_t holdClkHighMicro = 0;


};
