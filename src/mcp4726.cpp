#include <Arduino.h>
#include "mcp4726.hpp"

MCP4726::MCP4726(uint8_t address /*  = 0x60 */,
                 TwoWire *i2c /* = &Wire */,
                 uint32_t frequency /* = 400000 */)
    : _address(address),
      _i2c(i2c),
      _frequency(frequency) {}

bool MCP4726::writeDAC(uint16_t value,
                       MCP4726::PowerDown pd /* = PD_RUN */)
{
    uint8_t data[2];

    data[0] = uint8_t((int(pd) << 4) | ((value >> 8) & 0x0f));
    data[1] = uint8_t(value & 0x00ff);

    return writeData(data, 2);
}

bool MCP4726::writeMem(MCP4726::Reference ref,
                       MCP4726::PowerDown pd,
                       MCP4726::Gain g,
                       uint16_t dacValue,
                       bool persistent /* = false */)
{
    uint8_t data[3];

    int cmd = persistent ? 0b011 : 0b010;

    data[0] = uint8_t((cmd << 5) | (int(ref) << 3) | (int(pd) << 1) | int(g));
    data[1] = uint8_t((dacValue >> 4) & 0x00ff);
    data[2] = uint8_t((dacValue << 4) & 0x00ff);

    return writeData(data, 3);
}

bool MCP4726::writeVolatileConfig(MCP4726::Reference ref,
                                  MCP4726::PowerDown pd,
                                  MCP4726::Gain g)
{
    uint8_t data[1];

    int cmd = 0b100;

    data[0] = uint8_t((cmd << 5) | (int(ref) << 3) | (int(pd) << 1) | int(g));

    return writeData(data, 1);
}

uint16_t MCP4726::getWord(uint8_t reg)
{
    // Begin a transmission to the device
    // at "_address"
    uint32_t oldFreq = _i2c->getClock();
    _i2c->setClock(_frequency);
    _i2c->beginTransmission(_address);

    // Send register pointer value for
    // the specified register
    if (_i2c->write(reg) != 1)
    {
        Serial.println("Wire.write() failed");

        _i2c->setClock(oldFreq);
        return 0;
    }

    // Send message
    uint8_t error = _i2c->endTransmission(false);
    if (error)
    {
        Serial.printf("Wire.endTransmission() failed: %s\r\n", _i2c->getErrorText(error));

        _i2c->setClock(oldFreq);
        return 0;
    }

    // Give it a (milli)second
    delay(1);

    // Request the two-byte word for the
    // shunt voltage
    uint8_t requestLen = 2;
    _i2c->requestFrom(_address, requestLen);

    uint16_t word = 0;
    if (_i2c->available() >= requestLen)
    {
        word = _i2c->read();

        word <<= 8;

        word |= _i2c->read();

        _i2c->setClock(oldFreq);
        return word;
    }
    else
    {
        Serial.println("Not enough data returned");

        _i2c->setClock(oldFreq);
        return 0;
    }
}

bool MCP4726::writeData(uint8_t *data,
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
