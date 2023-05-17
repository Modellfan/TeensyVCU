#include <ACAN_T4.h>
#include <Arduino.h>

void printFrame(CANMessage &frame)
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

void can_setup()
{
    Serial.println("CAN1 loopback test");
    ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    const uint32_t errorCode = ACAN_T4::can1.begin(settings);
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
        Serial.println("can1 ok");
    }
    else
    {
        Serial.print("Error can1: 0x");
        Serial.println(errorCode, HEX);
    }
}

void can_poll()
{
    CANMessage message;
    if (ACAN_T4::can1.receive(message))
    {
        // printFrame(message);
        int32_t current = 0;
        float Temperature = 0.0;
        long temp = 0;

        switch (message.id)
        {
        case 0x3c3:
            current = (int32_t)(((message.data[4] << 8) + (message.data[5])) - ((message.data[2] << 8) + (message.data[3])));
            Serial.print("Current: ");
            Serial.println(current);
            break;
        case 0x3D2:
            temp = (int32_t)(((message.data[4] << 8) + (message.data[5])) - ((message.data[2] << 8) + (message.data[3])));
            Temperature = temp / 10;
            Serial.print("Temperature: ");
            Serial.println(Temperature, 2);
        default:
            break;
        }
    }
}
