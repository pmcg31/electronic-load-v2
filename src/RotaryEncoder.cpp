#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include "RotaryEncoder.hpp"

// #define AZ_INVERT

static void eventTaskHelper(void *objPtr);

const int RotaryEncoder::g_debounceTime_us = 250;

RotaryEncoder::RotaryEncoder(int aPin, int bPin, int buttonPin,
                             int detents /*= 24*/)
    : _listener(0),
      _aPin(aPin),
      _bPin(bPin),
      _zPin(buttonPin),
      _detents(detents),
      _lastRotaryTick_ms(0),
      _eventTaskHandle(NULL),
      _aPinPending(false),
      _zPinPending(false),
      _aPinDelta(0),
      _aPinTotalTriggers(0),
      _bPinValue(0),
      _zPinTriggered(false)
{
}

void RotaryEncoder::setListener(RotaryEncoderListener *listener)
{
    _listener = listener;
}

bool RotaryEncoder::init()
{
    pinMode(_aPin, INPUT);
    pinMode(_bPin, INPUT_PULLUP);
    pinMode(_zPin, INPUT_PULLUP);

#ifdef AZ_INVERT
    attachInterrupt(_aPin, std::bind(&RotaryEncoder::_aPinHandler, this), RISING);
    attachInterrupt(_zPin, std::bind(&RotaryEncoder::_zPinHandler, this), RISING);
#else
    attachInterrupt(_aPin, std::bind(&RotaryEncoder::_aPinHandler, this), FALLING);
    attachInterrupt(_zPin, std::bind(&RotaryEncoder::_zPinHandler, this), FALLING);
#endif

    _aPinTimerArgs.callback = &RotaryEncoder::g_aPinDebounce;
    _aPinTimerArgs.arg = this;
    esp_timer_create(&_aPinTimerArgs, &_aPinTimer);

    _zPinTimerArgs.callback = &RotaryEncoder::g_zPinDebounce;
    _zPinTimerArgs.arg = this;
    esp_timer_create(&_zPinTimerArgs, &_zPinTimer);

    if (xTaskCreate(eventTaskHelper,
                    "RotaryEncoder::loop",
                    1000,
                    (void *)this,
                    1,
                    &_eventTaskHandle) != pdPASS)
    {
        return false;
    }

    return true;
}

void RotaryEncoder::eventTask()
{
    while (true)
    {
        unsigned long now = millis();

        if (_aPinTotalTriggers != 0)
        {
            int tempTotalTriggers = _aPinTotalTriggers;
            int tempDelta = _aPinDelta;
            _aPinTotalTriggers = 0;
            _aPinDelta = 0;
            int tickTime_ms = now > _lastRotaryTick_ms ? (now - _lastRotaryTick_ms) / tempTotalTriggers : 50;
            _lastRotaryTick_ms = now;

            int rpm = int(60000.0 / (double(tickTime_ms) * 24.0));

            if (_listener != 0)
            {
                _listener->turned(this, tempDelta, rpm);
            }
        }

        if (_zPinTriggered)
        {
            _zPinTriggered = false;

            if (_listener != 0)
            {
                _listener->clicked(this);
            }
        }

        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
}

void IRAM_ATTR RotaryEncoder::_aPinHandler()
{
    if (!_aPinPending)
    {
        detachInterrupt(_aPin);
        _aPinPending = true;
        _bPinValue = digitalRead(_bPin) == LOW;
        if (_aPinTimer != 0)
        {
            esp_timer_start_once(_aPinTimer, g_debounceTime_us);
        }
        else
        {
            Serial.println("Holy shit its null (A)");
        }
    }
}

void IRAM_ATTR RotaryEncoder::_zPinHandler()
{
    if (!_zPinPending)
    {
        _zPinPending = true;
        if (_zPinTimer != 0)
        {
            esp_timer_start_once(_zPinTimer, g_debounceTime_us);
        }
        else
        {
            Serial.println("Holy shit its null (Z)");
        }
    }
}

void IRAM_ATTR RotaryEncoder::_aPinDebounce()
{
#ifdef AZ_INVERT
    attachInterrupt(_aPin, std::bind(&RotaryEncoder::_aPinHandler, this), RISING);
#else
    attachInterrupt(_aPin, std::bind(&RotaryEncoder::_aPinHandler, this), FALLING);
#endif

    _aPinPending = false;
#ifdef AZ_INVERT
    if (digitalRead(_aPin) == HIGH)
#else
    if (digitalRead(_aPin) == LOW)
#endif
    {
        _aPinTotalTriggers++;
        if (_bPinValue)
        {
            _aPinDelta++;
        }
        else
        {
            _aPinDelta--;
        }
    }
}

void IRAM_ATTR RotaryEncoder::_zPinDebounce()
{
    _zPinPending = false;
#ifdef AZ_INVERT
    if (digitalRead(_zPin) == HIGH)
#else
    if (digitalRead(_zPin) == LOW)
#endif
    {
        _zPinTriggered = true;
    }
}

void IRAM_ATTR RotaryEncoder::g_aPinDebounce(void *arg)
{
    RotaryEncoder *obj = (RotaryEncoder *)arg;
    obj->_aPinDebounce();
}

void IRAM_ATTR RotaryEncoder::g_zPinDebounce(void *arg)
{
    RotaryEncoder *obj = (RotaryEncoder *)arg;
    obj->_zPinDebounce();
}

void eventTaskHelper(void *objPtr)
{
    if (objPtr != 0)
    {
        RotaryEncoder *p = (RotaryEncoder *)objPtr;

        p->eventTask();
    }

    vTaskDelete(NULL);
}
