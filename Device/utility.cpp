// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "AzureIotHub.h"
#include "Arduino.h"
#include "parson.h"
#include "config.h"
#include "RGB_LED.h"

#define RGB_LED_BRIGHTNESS 32

DevI2C *i2c;
HTS221Sensor *hts221Sensor;
LPS22HBSensor *lps22hbSensor;

static RGB_LED rgbLed;
static int interval = INTERVAL;

static float humidity;
static float temperature;
static float pressure;
static float temperature2;

int getInterval()
{
    return interval;
}

void blinkLED()
{
    rgbLed.turnOff();
    rgbLed.setColor(RGB_LED_BRIGHTNESS, 0, 0);
    delay(500);
    rgbLed.turnOff();
}

void blinkSendConfirmation()
{
    rgbLed.turnOff();
    rgbLed.setColor(0, 0, RGB_LED_BRIGHTNESS);
    delay(500);
    rgbLed.turnOff();
}

void parseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message)
{
    JSON_Value *root_value;
    root_value = json_parse_string(message);
    if (json_value_get_type(root_value) != JSONObject)
    {
        if (root_value != NULL)
        {
            json_value_free(root_value);
        }
        LogError("parse %s failed", message);
        return;
    }
    JSON_Object *root_object = json_value_get_object(root_value);

    double val = 0;
    if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        JSON_Object *desired_object = json_object_get_object(root_object, "desired");
        if (desired_object != NULL)
        {
            val = json_object_get_number(desired_object, "interval");
        }
    }
    else
    {
        val = json_object_get_number(root_object, "interval");
    }
    if (val > 500)
    {
        interval = (int)val;
        LogInfo(">>>Device twin updated: set interval to %d", interval);
    }
    json_value_free(root_value);
}

int SensorInit()
{
    i2c = new DevI2C(D14, D15);
    hts221Sensor = new HTS221Sensor(*i2c);
    int result = hts221Sensor->init(NULL);
    lps22hbSensor = new LPS22HBSensor(*i2c);
    lps22hbSensor->init(NULL);

    humidity = -1;
    temperature = -1000;
    pressure = -1;
    temperature2 = -1000;

    return result;
}

bool readMessage(int messageId, char *payload, float *temperatureValue, float *humidityValue)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;

    json_object_set_number(root_object, "messageId", messageId);

    hts221Sensor->reset();
    if(hts221Sensor->getTemperature(&temperature) == 0)
    {
        *temperatureValue = temperature;
        json_object_set_number(root_object, "temperature", temperature);
    }
    
    if(hts221Sensor->getHumidity(&humidity) == 0)
    {
        *humidityValue = humidity;
        json_object_set_number(root_object, "humidity", humidity);
    }

    if(lps22hbSensor->getPressure(&pressure) == 0)
    {
        json_object_set_number(root_object, "pressure", pressure);
    }

    if(lps22hbSensor->getTemperature(&temperature) == 0)
    {
        json_object_set_number(root_object, "temperature2", temperature2);
    }
    
    serialized_string = json_serialize_to_string_pretty(root_value);

    snprintf(payload, MESSAGE_MAX_LEN, "%s", serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

#if (DEVKIT_SDK_VERSION >= 10602)
void __sys_setup(void)
{
    // Enable Web Server for system configuration when system enter AP mode
    EnableSystemWeb(WEB_SETTING_IOT_DEVICE_CONN_STRING);
}
#endif