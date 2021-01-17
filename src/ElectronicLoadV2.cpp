#include "TaskSyncShared.hpp"
#include "ElectronicLoadV2.hpp"

const int ElectronicLoadV2::g_aPin = 33;
const int ElectronicLoadV2::g_bPin = 32;
const int ElectronicLoadV2::g_zPin = 25;
const uint8_t ElectronicLoadV2::g_screenI2cAddr = 0x3C;
const int ElectronicLoadV2::g_encoderDetentsPerRev = 24;

static void mainTaskHelper(void *objPtr);

ElectronicLoadV2::ElectronicLoadV2()
    : _mcp4726(),
      _max11645(),
      _data(),
      _textUI(g_screenI2cAddr),
      _encoder(g_aPin, g_bPin, g_zPin,
               g_encoderDetentsPerRev),
      _mutex(0),
      _mainTaskHandle(NULL),
      _desiredCurrent(0.0),
      _isEnabled(false),
      _settingsChanged(false),
      _newDesiredCurrent(0.0),
      _newIsEnabled(false)
{
    _encoder.setListener(&_textUI);

    _mutex = xSemaphoreCreateMutex();
}

bool ElectronicLoadV2::start()
{
    if (xTaskCreate(mainTaskHelper,
                    "ElectronicLoadV2::mainTask",
                    4000,
                    (void *)this,
                    1,
                    &_mainTaskHandle) != pdPASS)
    {
        return false;
    }

    return true;
}

void ElectronicLoadV2::mainTask()
{
    TaskSyncShared *tss = TaskSyncShared::getInstance();

    tss->takeSerial();
    Serial.begin(115200);
    delay(1000);
    Serial.printf("\r\nelectronic-load-v2 %s %s\r\n", __DATE__, __TIME__);
    tss->giveSerial();

    // Join I2C bus
    tss->takeI2c();
    bool success = Wire.begin();
    tss->giveI2c();
    if (!success)
    {
        tss->takeSerial();
        Serial.println("Wire.begin() failed");
        tss->giveSerial();
        vTaskDelete(NULL);
    }

    _textUI.setListener(this);
    if (!_textUI.init())
    {
        tss->takeSerial();
        Serial.println("Failed to initialize text UI");
        tss->giveSerial();
        vTaskDelete(NULL);
    }

    _encoder.init();

    tss->takeSerial();
    Serial.print("Initializing MCP4726...");
    tss->giveSerial();
    tss->takeI2c();
    _mcp4726.writeMem(MCP4726::REF_VREF_BUFFERED, MCP4726::PD_RUN, MCP4726::G_1X, 0, true);
    tss->giveI2c();
    delay(200);
    tss->takeSerial();
    Serial.println("done.");

    Serial.print("Initializing MAX11645...");
    tss->giveSerial();
    tss->takeI2c();
    _max11645.writeAll(MAX11645::SM_UP_FROM_AIN0_TO_CS0,
                       MAX11645::CS_AIN1,
                       MAX11645::MODE_SINGLE_ENDED,
                       MAX11645::REF_INTERNAL_REFOUT,
                       MAX11645::CLK_INTERNAL,
                       MAX11645::DSM_UNIPOLAR);
    tss->giveI2c();
    delay(200);
    tss->takeSerial();
    Serial.println("done.");
    tss->giveSerial();

    while (true)
    {
        bool settingsChanged = false;
        double newDesiredCurrent = _desiredCurrent;
        bool newIsEnabled = _isEnabled;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        if (_settingsChanged)
        {
            _settingsChanged = false;
            settingsChanged = true;
            newDesiredCurrent = _newDesiredCurrent;
            newIsEnabled = _newIsEnabled;
        }
        xSemaphoreGive(_mutex);

        if (settingsChanged)
        {
            _updateSettings(newDesiredCurrent, newIsEnabled);
        }

        _readADC();

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void ElectronicLoadV2::desiredCurrentChanged(TextUI *source, double desiredCurrent)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _newDesiredCurrent = desiredCurrent;
    _settingsChanged = true;
    xSemaphoreGive(_mutex);
}

void ElectronicLoadV2::enabledChanged(TextUI *source, bool isEnabled)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _newIsEnabled = isEnabled;
    _settingsChanged = true;
    xSemaphoreGive(_mutex);
}

bool ElectronicLoadV2::_readADC()
{
    TaskSyncShared *tss = TaskSyncShared::getInstance();

    tss->takeI2c();
    uint16_t *result = _max11645.readSamples(_data, 2);
    tss->giveI2c();

    if (result != _data)
    {
        return false;
    }

    double loadVoltage = (_data[0] * 0.0005) * 15;
    double loadCurrent = ((_data[1] * 0.0005) / 67) / 0.01;

    tss->takeSerial();
    Serial.printf("AIN0: %4d [%5.3lfV] (%5.3lfV) AIN1: %4d [%5.3lfV] (%8.3lfmA)\r\n", _data[0], _data[0] * 0.0005, loadVoltage, _data[1], _data[1] * 0.0005, loadCurrent * 1000);
    tss->giveSerial();

    _textUI.loadVoltageChanged(loadVoltage);
    _textUI.loadCurrentChanged(loadCurrent);

    return true;
}

void ElectronicLoadV2::_updateSettings(double newDesiredCurrent,
                                       bool newIsEnabled)
{
    if (_desiredCurrent != newDesiredCurrent)
    {
        _desiredCurrent = newDesiredCurrent;
    }

    if (_isEnabled != newIsEnabled)
    {
        _isEnabled = newIsEnabled;
    }

    TaskSyncShared *tss = TaskSyncShared::getInstance();
    tss->takeI2c();
    if (_isEnabled)
    {
        double desiredSenseVoltage = _desiredCurrent * 0.01;
        uint16_t dacValue = uint16_t(desiredSenseVoltage * 67 / 0.0005);
        tss->takeSerial();
        Serial.printf("Set dac value %d\r\n", dacValue);
        tss->giveSerial();
        _mcp4726.writeDAC(dacValue);
    }
    else
    {
        tss->takeSerial();
        Serial.printf("Set dac value %d\r\n", 0);
        tss->giveSerial();
        _mcp4726.writeDAC(0);
    }
    tss->giveI2c();

    _textUI.setDesiredCurrent(_desiredCurrent);
    _textUI.setEnabled(_isEnabled);
}

void mainTaskHelper(void *objPtr)
{
    if (objPtr != 0)
    {
        ElectronicLoadV2 *app = (ElectronicLoadV2 *)objPtr;

        app->mainTask();
    }

    vTaskDelete(NULL);
}
