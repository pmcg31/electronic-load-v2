#include <string.h>
#include "TaskSyncShared.hpp"
#include "TextUI.hpp"

static void uiTaskHelper(void *objPtr);

const int TextUI::g_cursorPointsCount = 6;
const TextUI::Point TextUI::g_cursorPoints[] = {
    {2, 5},
    {4, 5},
    {5, 5},
    {6, 5},
    {7, 5},
    {0, 7}};

TextUI::TextUI(uint8_t i2cAddr)
    : _i2cAddr(i2cAddr),
      _screenWidth(128),
      _screenHeight(64),
      _fontWidth(6),
      _fontHeight(8),
      _widthChars(128 / 6),
      _heightChars(64 / 8),
      _screenBuf(0),
      _dirtyRegions(0),
      _display(128,
               64,
               &Wire, -1),
      _loadVoltage(0.0),
      _loadCurrent(0.0),
      _desiredCurrent(0.0),
      _isEnabled(false),
      _encoderDelta(0),
      _encoderClicked(false),
      _uiDirty(false),
      _cursorIdx(-1),
      _listener(0) {}

bool TextUI::init()
{
    TaskSyncShared *tss = tss->getInstance();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    tss->takeI2c();
    bool success = _display.begin(SSD1306_SWITCHCAPVCC, _i2cAddr);
    tss->giveI2c();
    if (!success)
    {
        return false;
    }

    int screenBufSize = _widthChars * _heightChars;

    _screenBuf = new char[screenBufSize];
    _dirtyRegions = new Dirty[_heightChars];

    for (int i = 0; i < screenBufSize; i++)
    {
        _screenBuf[i] = ' ';
    }

    _mutex = xSemaphoreCreateMutex();

    if (xTaskCreate(uiTaskHelper,
                    "TextUI::uiTask",
                    4000,
                    (void *)this,
                    1,
                    &_uiTaskHandle) != pdPASS)
    {
        return false;
    }

    return true;
}

void TextUI::setListener(TextUIListener *listener)
{
    _listener = listener;
}

void TextUI::uiTask()
{
    TaskSyncShared *tss = TaskSyncShared::getInstance();

    clear();
    splash();

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    clear();
    _drawUI();
    _moveCursor(2);

    while (true)
    {
        bool encoderClicked = false;
        int encoderDelta = 0;
        xSemaphoreTake(_mutex, portMAX_DELAY);
        if (_encoderDelta != 0)
        {
            encoderDelta = _encoderDelta;
            _encoderDelta = 0;
        }
        if (_encoderClicked)
        {
            encoderClicked = true;
            _encoderClicked = false;
        }
        xSemaphoreGive(_mutex);

        if (encoderClicked)
        {
            tss->takeSerial();
            Serial.println("Encoder clicked!");
            tss->giveSerial();

            int newCursorIdx = _cursorIdx + 1;
            if (!(newCursorIdx < g_cursorPointsCount))
            {
                newCursorIdx = 0;
            }
            _moveCursor(newCursorIdx);
        }

        if (encoderDelta != 0)
        {
            tss->takeSerial();
            Serial.printf("Encoder moved %d clicks\r\n", encoderDelta);
            tss->giveSerial();

            if (_listener != 0)
            {
                if (g_cursorPoints[_cursorIdx].y == 5)
                {
                    double newDesiredCurrent = _desiredCurrent + (encoderDelta / exp10(_cursorIdx));
                    if (newDesiredCurrent > 3.0)
                    {
                        newDesiredCurrent = 3.0;
                    }
                    if (newDesiredCurrent < 0.0)
                    {
                        newDesiredCurrent = 0;
                    }

                    _listener->desiredCurrentChanged(this, newDesiredCurrent);
                }
                else
                {
                    if ((encoderDelta & 1) == 1)
                    {
                        _listener->enabledChanged(this, !_isEnabled);
                    }
                }
            }
        }

        if (_uiDirty)
        {
            _uiDirty = false;
            _drawUI();
        }

        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
}

void TextUI::clear()
{
    for (int i = 0; i < _heightChars; i++)
    {
        for (int j = 0; j < _widthChars; j++)
        {
            _screenBuf[(i * _widthChars) + j] = ' ';
        }
        _dirtyRegions[i].y = -1;
    }

    TaskSyncShared *tss = TaskSyncShared::getInstance();
    tss->takeI2c();
    _display.clearDisplay();
    _display.display();
    tss->giveI2c();
}

void TextUI::splash()
{
    const size_t bufLen = 100;
    char buf[100];

    const char *tmp = "Electronic Load V2";
    int x = (_widthChars - strlen(tmp)) / 2;
    int y = (_heightChars - 5) / 2;
    _writeChars(x, y, tmp);

    int len = snprintf(buf, bufLen - 1, "%s %s", __DATE__, __TIME__);
    x = (_widthChars - len) / 2;
    y += 2;
    _writeChars(x, y, buf);

    tmp = "http://ideaup.online";
    x = (_widthChars - strlen(tmp)) / 2;
    y += 2;
    _writeChars(x, y, tmp);

    _commitChangesToDisplay();
}

void TextUI::turned(RotaryEncoder *source, int deltaClicks, int rpm)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _encoderDelta += deltaClicks;
    xSemaphoreGive(_mutex);
}

void TextUI::clicked(RotaryEncoder *source)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _encoderClicked = true;
    xSemaphoreGive(_mutex);
}

void TextUI::loadVoltageChanged(double newVoltage)
{
    if (newVoltage != _loadVoltage)
    {
        _loadVoltage = newVoltage;
        _uiDirty = true;
    }
}

void TextUI::loadCurrentChanged(double newCurrent)
{
    if (newCurrent != _loadCurrent)
    {
        _loadCurrent = newCurrent;
        _uiDirty = true;
    }
}

void TextUI::setDesiredCurrent(double desiredCurrent)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (desiredCurrent != _desiredCurrent)
    {
        _desiredCurrent = desiredCurrent;

        if (_listener != 0)
        {
            _listener->desiredCurrentChanged(this, _desiredCurrent);
        }

        _uiDirty = true;
    }
    xSemaphoreGive(_mutex);
}

void TextUI::setEnabled(bool isEnabled)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (isEnabled != _isEnabled)
    {
        _isEnabled = isEnabled;

        if (_listener != 0)
        {
            _listener->enabledChanged(this, _isEnabled);
        }

        _uiDirty = true;
    }
    xSemaphoreGive(_mutex);
}

void TextUI::_drawUI()
{
    const char *tmp = "Electronic Load V2";
    int x = (_widthChars - strlen(tmp)) / 2;
    _writeChars(x, 0, tmp);

    _printf(2, 2, "%6.3lf V", _loadVoltage);
    _printf(11, 2, "%6.4lf A", _loadCurrent);

    _writeChars(0, 4, "SET");

    _printf(2, 5, "%6.4lf A", _desiredCurrent);

    _writeChars(0, 7, _isEnabled ? "ON " : "OFF");

    bool cursorAffected = false;
    _commitChangesToDisplay(&cursorAffected);

    if (cursorAffected)
    {
        _drawCursor();
    }
}

void TextUI::_moveCursor(int newCursorIdx)
{
    if (_cursorIdx != -1)
    {
        Dirty *dirty = _getDirtyForRow(g_cursorPoints[_cursorIdx].y);
        if (dirty != 0)
        {
            dirty->update(g_cursorPoints[_cursorIdx].y, g_cursorPoints[_cursorIdx].x, g_cursorPoints[_cursorIdx].x);
            _commitChangesToDisplay();
        }
    }

    _cursorIdx = newCursorIdx;

    _drawCursor();
}

void TextUI::_drawCursor()
{
    TaskSyncShared *tss = TaskSyncShared::getInstance();
    tss->takeI2c();
    int pixX = g_cursorPoints[_cursorIdx].x * _fontWidth;
    int pixY = g_cursorPoints[_cursorIdx].y * _fontHeight + (_fontHeight - 2);
    _display.fillRect(pixX, pixY, _fontWidth, 2, SSD1306_WHITE);
    tss->giveI2c();
}

void TextUI::_writeChars(int x, int y, const char *text)
{
    size_t maxLen = _widthChars - x;

    char *bufStart = _screenBuf + (y * _widthChars) + x;

    size_t len = strlen(text);
    if (len > maxLen)
    {
        len = maxLen;
    }

    int iLo = -1;
    int iHi = -1;
    for (int i = 0; i < len; i++)
    {
        if (*(bufStart + i) != *(text + i))
        {
            *(bufStart + i) = *(text + i);
            if (iLo == -1)
            {
                iLo = i;
            }
            if (iHi < i)
            {
                iHi = i;
            }
        }
    }

    if (iLo != -1)
    {
        Dirty *dirty = _getDirtyForRow(y);
        if (dirty != 0)
        {
            dirty->update(y, x + iLo, x + iHi);
        }
    }
}

void TextUI::_printf(int x, int y, const char *fmt, ...)
{
    const size_t bufLen = 100;
    char buf[bufLen];

    va_list args;
    va_start(args, fmt);

    size_t maxLen = _widthChars - x;
    if (maxLen > (bufLen - 1))
    {
        maxLen = bufLen - 1;
    }
    int len = vsnprintf(buf, maxLen, fmt, args);
    buf[len] = '\0';

    _writeChars(x, y, buf);
}

void TextUI::_commitChangesToDisplay(bool *cursorAffected /* = 0 */)
{
    TaskSyncShared *tss = TaskSyncShared::getInstance();

    tss->takeI2c();

    for (int i = 0; i < _heightChars; i++)
    {
        if (_dirtyRegions[i].y == -1)
        {
            break;
        }

        if (cursorAffected != 0)
        {
            if (_dirtyRegions[i].y == g_cursorPoints[_cursorIdx].y)
            {
                *cursorAffected = (_dirtyRegions[i].xLo <= g_cursorPoints[_cursorIdx].x) &&
                                  (g_cursorPoints[_cursorIdx].x <= _dirtyRegions[i].xHi);
            }
        }

        const char *screenBufStart = _screenBuf + ((_dirtyRegions[i].y * _widthChars) + _dirtyRegions[i].xLo);
        size_t len = _dirtyRegions[i].xHi - _dirtyRegions[i].xLo + 1;

        _writeTextToDisplay(_dirtyRegions[i].xLo, _dirtyRegions[i].y, screenBufStart, len);

        _dirtyRegions[i].y = -1;
    }

    _display.display();

    tss->giveI2c();
}

void TextUI::_writeTextToDisplay(int x, int y, const char *text, size_t len)
{
    int pixY = y * _fontHeight;
    int pixHeight = _fontHeight;
    int pixX = x * _fontWidth;
    int pixWidth = _fontWidth * len;

    _display.fillRect(pixX, pixY, pixWidth, pixHeight, SSD1306_BLACK);

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(pixX, pixY);
    _display.printf("%.*s", len, text);
}

TextUI::Dirty *TextUI::_getDirtyForRow(int y)
{
    for (int i = 0; i < _heightChars; i++)
    {
        if ((_dirtyRegions[i].y == y) ||
            (_dirtyRegions[i].y == -1))
        {
            return &(_dirtyRegions[i]);
        }
    }

    return 0;
}

TextUI::Dirty::Dirty()
    : y(-1), xLo(-1), xHi(-1) {}

TextUI::Dirty::Dirty(int yIn, int xLoIn, int xHiIn)
    : y(yIn), xLo(xLoIn), xHi(xHiIn) {}

void TextUI::Dirty::update(int yIn, int xLoIn, int xHiIn)
{
    if (y == yIn)
    {
        if (xLoIn < xLo)
        {
            xLo = xLoIn;
        }
        if (xHiIn > xHi)
        {
            xHi = xHiIn;
        }
    }
    else
    {
        y = yIn;
        xLo = xLoIn;
        xHi = xHiIn;
    }
}

TextUI::Point::Point()
    : x(0), y(0) {}

TextUI::Point::Point(int xIn, int yIn)
    : x(xIn), y(yIn) {}

void uiTaskHelper(void *objPtr)
{
    if (objPtr != 0)
    {
        TextUI *ui = (TextUI *)objPtr;

        ui->uiTask();
    }

    vTaskDelete(NULL);
}
