#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"
#include "current.h"

Shunt_ISA_iPace::Shunt_ISA_iPace()
{
    _state = INIT;

    _temperature = 0.0;
    _current = 0.0;
    _current_average = 0.0;
    _ampere_seconds = 0.0;
    _previousMillis = 0;
    _firstUpdate = true;
}

void Shunt_ISA_iPace::printFrame(CANMessage &frame)
{
    // Print message
    Serial.print("ID: ");
    Serial.print(frame.id, HEX);
    Serial.print(" Ext: ");
    if (frame.ext)
    {
        Serial.print("Y");
    }
    else
    {
        Serial.print("N");
    }
    Serial.print(" Len: ");
    Serial.print(frame.len, DEC);
    Serial.print(" ");
    for (int i = 0; i < frame.len; i++)
    {
        Serial.print(frame.data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void Shunt_ISA_iPace::initialise()
{
    Serial.println("ISA Shunt CAN Initialize");
    ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    const uint32_t errorCode = ACAN_T4::ISA_SHUNT_CAN.begin(settings);
    Serial.print("Bitrate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Propagation Segment: ");
    Serial.println(settings.mPropagationSegment);
    Serial.print("Phase segment 1: ");
    Serial.println(settings.mPhaseSegment1);
    Serial.print("Phase segment 2: ");
    Serial.println(settings.mPhaseSegment2);
    Serial.print("RJW: ");
    Serial.println(settings.mRJW);
    Serial.print("Triple Sampling: ");
    Serial.println(settings.mTripleSampling ? "yes" : "no");
    Serial.print("Actual bitrate: ");
    Serial.print(settings.actualBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact bitrate ? ");
    Serial.println(settings.exactBitRate() ? "yes" : "no");
    Serial.print("Distance from wished bitrate: ");
    Serial.print(settings.ppmFromWishedBitRate());
    Serial.println(" ppm");
    Serial.print("Sample point: ");
    Serial.print(settings.samplePointFromBitStart());
    Serial.println("%");
    if (0 == errorCode)
    {
        Serial.println("ISA Shunt CAN ok");
    }
    else
    {
        Serial.print("Error ISA Shunt CAN: 0x");
        Serial.println(errorCode, HEX);
    }
}

void Shunt_ISA_iPace::update()
{
    CANMessage message;
    if (ACAN_T4::ISA_SHUNT_CAN.receive(message))
    {
        // printFrame(message);

        // toDO
        // check state

        switch (message.id)
        {
        case 0x3c3:
            _current = (int32_t)(((message.data[4] << 8) + (message.data[5])) - ((message.data[2] << 8) + (message.data[3]))) - 0;
            Serial.print("Current: ");
            Serial.println(_current);
            calculate_current_values();
            break;
        case 0x3D2:
            _temperature = (int32_t)(((message.data[4] << 8) + (message.data[5])) - ((message.data[2] << 8) + (message.data[3]))) / 10.0;
            Serial.print("Temperature: ");
            Serial.println(_temperature, 2);
        default:
            break;
        }
    }
}

void Shunt_ISA_iPace::calculate_current_values()
{
    unsigned long currentMillis = millis();
    unsigned long deltaTime = currentMillis - _previousMillis;
    _previousMillis = currentMillis;

    if (_firstUpdate)
    {
        _last_current = _current;
        _firstUpdate = false;
    }
    else
    {
        float deltaTimeSeconds = deltaTime / 1000.0; // Convert deltaTime to seconds for integration

        // Calculate moving average over 1 second
        float alpha = deltaTimeSeconds / 1.0; // Smoothing factor (1 second time span)
        _current_average = alpha * _current + (1 - alpha) * _last_current;

        // Perform integration
        _ampere_seconds += (_last_current + _current) * 0.5 * deltaTimeSeconds;

        // Calculate current derivative in Amps per second
        _current_derivative = (_current - _last_current) / deltaTimeSeconds;

        // Update current value
        _last_current = _current;
    }
}

float Shunt_ISA_iPace::getCurrent()
{
    return _current;
}

float Shunt_ISA_iPace::getTemperature()
{
    return _temperature;
}

float Shunt_ISA_iPace::getCurrentAverage()
{
    return _current_average;
}

// Get the integrated ampere-seconds and reset the integral
float Shunt_ISA_iPace::getAmpereSeconds()
{
    float integratedValue = _ampere_seconds;
    _ampere_seconds = 0.0;
    return integratedValue;
}

float Shunt_ISA_iPace::getCurrentDerivative()
{
    return _current_derivative;
}