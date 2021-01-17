#ifndef __H_MAX11645__
#define __H_MAX11645__

#include <stdlib.h>
#include <stdint.h>
#include <Wire.h>

class MAX11645
{
public:
    enum Reference
    {
        REF_VDD = 0b000,
        REF_EXTERNAL_REFIN = 0b010,
        REF_INTERNAL = 0b101,
        REF_INTERNAL_REFOUT = 0b111
    };

    enum RegSel
    {
        RS_CONFIG = 0b0,
        RS_SETUP = 0b1
    };

    enum ClkSel
    {
        CLK_INTERNAL = 0b0,
        CLK_EXTERNAL = 0b1
    };

    enum DiffSubMode
    {
        DSM_UNIPOLAR = 0b0,
        DSM_BIPOLAR = 0b1
    };

    enum ScanMode
    {
        SM_UP_FROM_AIN0_TO_CS0 = 0b00,
        SM_CS0_8X = 0b01,
        SM_CS0 = 0b11
    };

    enum ChanSel
    {
        CS_AIN0 = 0b0,
        CS_AIN1 = 0b1
    };

    enum Mode
    {
        MODE_DIFFERENTIAL = 0b0,
        MODE_SINGLE_ENDED = 0b1
    };

public:
    MAX11645(uint8_t address = 0x36,
             TwoWire *i2c = &Wire,
             uint32_t frequency = 400000);

    bool writeConfig(ScanMode scanMode,
                     ChanSel chanSel,
                     Mode mode);
    bool writeSetup(Reference ref,
                    ClkSel clkSel,
                    DiffSubMode subMode,
                    bool resetConfig);
    bool writeAll(ScanMode scanMode,
                  ChanSel chanSel,
                  Mode mode,
                  Reference ref,
                  ClkSel clkSel,
                  DiffSubMode subMode);

    uint16_t *readSamples(uint16_t *buf, size_t bufLen);

private:
    uint8_t makeConfig(ScanMode scanMode,
                       ChanSel chanSel,
                       Mode mode);
    uint8_t makeSetup(Reference ref,
                      ClkSel clkSel,
                      DiffSubMode subMode,
                      bool resetConfig);

    bool writeData(uint8_t *data,
                   uint8_t len);

private:
    uint8_t _address;
    TwoWire *_i2c;
    uint32_t _frequency;
};

#endif