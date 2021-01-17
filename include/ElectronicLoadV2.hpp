#ifndef __H_ELECTRONICLOADV2__
#define __H_ELECTRONICLOADV2__

#include "RotaryEncoder.hpp"
#include "mcp4726.hpp"
#include "max11645.hpp"
#include "TextUI.hpp"
#include "TextUIListener.hpp"

class ElectronicLoadV2 : public TextUIListener
{
public:
    ElectronicLoadV2();

    bool start();

    void mainTask();

    virtual void desiredCurrentChanged(TextUI *source, double desiredCurrent);
    virtual void enabledChanged(TextUI *source, bool isEnabled);

private:
    bool _readADC();
    void _updateSettings(double newDesiredCurrent,
                         bool newIsEnabled);

private:
    static const int g_aPin;
    static const int g_bPin;
    static const int g_zPin;
    static const uint8_t g_screenI2cAddr;
    static const int g_encoderDetentsPerRev;

private:
    MCP4726 _mcp4726;
    MAX11645 _max11645;
    uint16_t _data[2];

    TextUI _textUI;

    RotaryEncoder _encoder;

    SemaphoreHandle_t _mutex;

    TaskHandle_t _mainTaskHandle;

    double _desiredCurrent;
    bool _isEnabled;

    bool _settingsChanged;
    double _newDesiredCurrent;
    bool _newIsEnabled;
};

#endif