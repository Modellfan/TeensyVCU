#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"
#include "current.h"
#include "utils/can_packer.h"

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

        // Check for timeout of the shunt
        if (_state = OPERATING)
        {
            if (_lastUpdate + ISA_SHUNT_TIMEOUT > millis())
            {
                _state = FAULT;
                // toDo raise fault. Present replacement value
            }
        }

        // First byte has the mux-id:
        //  MuxID Result Unit
        //  0x00 IVT_Msg_Result_I 1 mA
        //  0x01 IVT_Msg_Result_U1 1 mV
        //  0x02 IVT_Msg_Result_U2 1 mV
        //  0x03 IVT_Msg_Result_U3 1 mV
        //  0x04 IVT_Msg_Result_T 0,1 °C
        //  0x05 IVT_Msg_Result_W 1 W
        //  0x06 IVT_Msg_Result_As 1 As
        //  0x07 IVT_Msg_Result_Wh 1 Wh

        // Low nibble of second byte is cyclic frame counter

        // high nibble second byte is
        //  bit 0: set if OCS is true
        //  bit 1: set if
        //  - this result is out of (spec-) range,
        //  - this result has reduced precision
        //  - this result has a measurement-error
        //  bit 2: set if
        //  - any result has a measurement-error
        //  bit 3: set if
        //  - system-error, sensor functionality is not ensured!

        switch (message.id)
        {
        case 0x3c0: // Signal send on startup
            break;
        case 0x3c3: // 10ms signal
            _last_status_bits = (u_int8_t)(message.data[2] & 0xF0) >> 4;
            if (_last_status_bits = 0)
            {
                if (_state = INIT) // First time we recieve a valid frame we put the shunt in operating state.
                {
                    _state = OPERATING;
                }
                _lastUpdate = millis();

                _current = (int32_t)(message.data[5] + (message.data[4] << 8) + (message.data[3] << 16) + (message.data[2] << 24)) / 1000.0;
                _0x3c3_framecounter = (u_int8_t)(message.data[2] & 0x0F);

                // Serial.print("Current: ");
                // Serial.println(_current, 3);
                calculate_current_values();
            }
            break;
        case 0x3D2: // 100ms signal
            _temperature = (int32_t)(message.data[5] + (message.data[4] << 8) + (message.data[3] << 16) + (message.data[2] << 24)) / 10.0;
            _0x3D2_framecounter = (u_int8_t)(message.data[2] & 0x0F);
            // Serial.print("Temperature: ");
            // Serial.println(_temperature, 2);
        default:
            break;
        }
    }
}

void Shunt_ISA_iPace::monitor(SendMessageCallback callback)
{
    CANMessage msg;

    msg.data64 = 0;
    msg.id = 1070; // Message 0x42e shunt_state 8bits None
    msg.len = 8;
    pack(msg, getState(), 0, 8, false, 1, 0);                  // getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, getDTC(), 8, 8, false, 1, 0);                    // getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, getCurrent(), 16, 16, false, 0.1, -1000);        // getCurrent : 16|16 little_endian unsigned scale: 0.1, offset: -1000, unit: Volt, None
    pack(msg, getTemperature(), 32, 16, false, 0.1, -40);      // getTemperature : 32|16 little_endian unsigned scale: 0.1, offset: -40, unit: None, None
    pack(msg, getCurrentAverage(), 48, 16, false, 0.1, -1000); // getCurrentAverage : 48|16 little_endian unsigned scale: 0.1, offset: -1000, unit: None, None

    // Send the CAN message using the provided callback
    if (callback != nullptr)
    {
        callback(&msg);
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

Shunt_ISA_iPace::STATE_ISA Shunt_ISA_iPace::getState() { return _state; }

Shunt_ISA_iPace::DTC_ISA Shunt_ISA_iPace::getDTC() { return _dtc; }