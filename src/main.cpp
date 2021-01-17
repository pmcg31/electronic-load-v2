#include <Arduino.h>
#include "ElectronicLoadV2.hpp"

void setup()
{
    ElectronicLoadV2 *app = new ElectronicLoadV2();

    if ((app != 0) && app->start())
    {
        vTaskDelete(NULL);
    }
}

void loop()
{
}
