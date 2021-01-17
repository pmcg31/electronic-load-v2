#ifndef __H_TEXTUI__
#define __H_TEXTUI__

#include <stdint.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RotaryEncoderListener.hpp"
#include "TextUIListener.hpp"

class TextUI : public RotaryEncoderListener
{
public:
    TextUI(uint8_t i2cAddr);

    bool init();

    void setListener(TextUIListener *listener);

    void uiTask();

    void clear();
    void splash();

    virtual void turned(RotaryEncoder *source, int deltaClicks, int rpm);
    virtual void clicked(RotaryEncoder *source);

    void loadVoltageChanged(double newVoltage);
    void loadCurrentChanged(double newCurrent);

    void setDesiredCurrent(double desiredCurrent);

    void setEnabled(bool isEnabled);

private:
    struct Dirty
    {
        Dirty();
        Dirty(int yIn, int xLoIn, int xHiIn);

        void update(int yIn, int xLoIn, int xHiIn);

        int y;
        int xLo;
        int xHi;
    };

    struct Point
    {
        Point();
        Point(int xIn, int yIn);

        int x;
        int y;
    };

private:
    void _drawUI();
    void _moveCursor(int newCursorIdx);
    void _drawCursor();
    void _writeChars(int x, int y, const char *text);
    void _printf(int x, int y, const char *fmt, ...);
    void _commitChangesToDisplay(bool *cursorAffected = 0);
    void _writeTextToDisplay(int x, int y, const char *text, size_t len);

    Dirty *_getDirtyForRow(int y);

private:
    uint8_t _i2cAddr;
    int _screenWidth;
    int _screenHeight;
    int _fontWidth;
    int _fontHeight;
    int _widthChars;
    int _heightChars;
    char *_screenBuf;
    Dirty *_dirtyRegions;
    Adafruit_SSD1306 _display;

    TaskHandle_t _uiTaskHandle;

    SemaphoreHandle_t _mutex;

    double _loadVoltage;
    double _loadCurrent;
    double _desiredCurrent;
    bool _isEnabled;

    int _encoderDelta;
    bool _encoderClicked;

    bool _uiDirty;

    int _cursorIdx;

    TextUIListener *_listener;

private:
    static const int g_cursorPointsCount;
    static const Point g_cursorPoints[];
};

#endif