#include <Arduino.h>
#include "max11645.hpp"

MAX11645::MAX11645(uint8_t address /* = 0x36 */,
                   TwoWire *i2c /* = &Wire */,
                   uint32_t frequency /* = 400000 */)
    : _address(address),
      _i2c(i2c),
      _frequency(frequency) {}

bool MAX11645::writeConfig(ScanMode scanMode,
                           ChanSel chanSel,
                           Mode mode)
{
    uint8_t data[1];

    data[0] = makeConfig(scanMode, chanSel, mode);

    return writeData(data, 1);
}

bool MAX11645::writeSetup(Reference ref,
                          ClkSel clkSel,
                          DiffSubMode subMode,
                          bool resetConfig)
{
    uint8_t data[1];

    data[0] = makeSetup(ref, clkSel, subMode, resetConfig);

    return writeData(data, 1);
}

bool MAX11645::writeAll(ScanMode scanMode,
                        ChanSel chanSel,
                        Mode mode,
                        Reference ref,
                        ClkSel clkSel,
                        DiffSubMode subMode)
{
    uint8_t data[2];

    data[0] = makeSetup(ref, clkSel, subMode, false);
    data[1] = makeConfig(scanMode, chanSel, mode);

    return writeData(data, 2);
}

uint16_t *MAX11645::readSamples(uint16_t *buf, size_t count)
{
    uint32_t oldFreq = _i2c->getClock();
    _i2c->setClock(_frequency);

    uint8_t byteCount = count * 2;
    _i2c->requestFrom(_address, byteCount);
    if (_i2c->available() == byteCount)
    {
        for (int i = 0; i < count; i++)
        {
            buf[i] = ((_i2c->read() & 0x0f) << 8) | _i2c->read();
        }
        _i2c->setClock(oldFreq);
        return buf;
    }
    else
    {
        _i2c->setClock(oldFreq);
        return 0;
    }
}

uint8_t MAX11645::makeConfig(ScanMode scanMode,
                             ChanSel chanSel,
                             Mode mode)
{
    return uint8_t((int(RS_CONFIG) << 7) | (int(scanMode) << 5) | (int(chanSel) << 1) | int(mode));
}

uint8_t MAX11645::makeSetup(Reference ref,
                            ClkSel clkSel,
                            DiffSubMode subMode,
                            bool resetConfig)
{
    return uint8_t((int(RS_SETUP) << 7) | (int(ref) << 4) | (int(clkSel) << 3) | (int(subMode) << 2) | ((resetConfig ? 0 : 1) << 1));
}

bool MAX11645::writeData(uint8_t *data,
                         uint8_t len)
{
    // Begin a transmission to the device
    // at "_address"
    uint32_t oldFreq = _i2c->getClock();
    _i2c->setClock(_frequency);
    _i2c->beginTransmission(_address);

    // Send data
    for (int i = 0; i < len; i++)
    {
        if (_i2c->write(data[i]) != 1)
        {
            Serial.println("Wire.write() failed");

            _i2c->setClock(oldFreq);
            return false;
        }
    }

    // Send message
    uint8_t error = _i2c->endTransmission();
    if (error)
    {
        Serial.printf("Wire.endTransmission() failed: %s\r\n", _i2c->getErrorText(error));

        _i2c->setClock(oldFreq);
        return false;
    }
    _i2c->setClock(oldFreq);

    return true;
}
