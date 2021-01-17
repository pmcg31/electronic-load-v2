#ifndef __H_MCP4726__
#define __H_MCP4726__

#include <stdlib.h>
#include <stdint.h>
#include <Wire.h>

class MCP4726
{
public:
    enum Reference
    {
        REF_VDD = 0b00,
        REF_VREF = 0b10,
        REF_VREF_BUFFERED = 0b11
    };

    enum PowerDown
    {
        PD_RUN = 0b00,
        PD_1K = 0b01,
        PD_100K = 0b10,
        PD_500K = 0b11
    };

    enum Gain
    {
        G_1X = 0b0,
        G_2X = 0b1
    };

public:
    MCP4726(uint8_t address = 0x60,
            TwoWire *i2c = &Wire,
            uint32_t frequency = 400000);

    bool writeDAC(uint16_t value,
                  PowerDown pd = PD_RUN);
    bool writeMem(Reference ref,
                  PowerDown pd,
                  Gain g,
                  uint16_t dacValue,
                  bool persistent = false);
    bool writeVolatileConfig(Reference ref,
                             PowerDown pd,
                             Gain g);

private:
    uint16_t getWord(uint8_t reg);
    bool writeData(uint8_t *data,
                   uint8_t len);

private:
    uint8_t _address;
    TwoWire *_i2c;
    uint32_t _frequency;
};

#endif