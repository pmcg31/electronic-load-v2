#include "TaskSyncShared.hpp"

TaskSyncShared *TaskSyncShared::g_instance = 0;

TaskSyncShared *TaskSyncShared::getInstance()
{
    if (g_instance == 0)
    {
        g_instance = new TaskSyncShared();
    }

    return g_instance;
}

void TaskSyncShared::takeI2c()
{
    xSemaphoreTake(_i2cMutex, portMAX_DELAY);
}

void TaskSyncShared::giveI2c()
{
    xSemaphoreGive(_i2cMutex);
}

void TaskSyncShared::takeSerial()
{
    xSemaphoreTake(_serialMutex, portMAX_DELAY);
}

void TaskSyncShared::giveSerial()
{
    xSemaphoreGive(_serialMutex);
}

TaskSyncShared::TaskSyncShared()
    : _i2cMutex(0),
      _serialMutex(0)
{
    _i2cMutex = xSemaphoreCreateMutex();
    _serialMutex = xSemaphoreCreateMutex();
}
