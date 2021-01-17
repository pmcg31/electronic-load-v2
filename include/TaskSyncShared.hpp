#ifndef __H_TASKSYNCSHARED__
#define __H_TASKSYNCSHARED__

#include <Arduino.h>

class TaskSyncShared
{
public:
    static TaskSyncShared *getInstance();

    void takeI2c();
    void giveI2c();

    void takeSerial();
    void giveSerial();

private:
    TaskSyncShared();

private:
    SemaphoreHandle_t _i2cMutex;
    SemaphoreHandle_t _serialMutex;

private:
    static TaskSyncShared *g_instance;
};

#endif