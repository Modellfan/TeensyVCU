:::mermaid
classDiagram
    class NissanPDM {
        - enum STATE_PDM
        - enum DTC_PDM
        - enum CHARGE_CURRENT_LIMITING_REASON_PDM
        - NissanPDM()
        - void initialize()
        - void Task2Ms()
        - void Task10Ms()
        - void Task100Ms()
        - void Monitor100Ms()
        - void Monitor1000Ms()
        - void print()
        - bool ControlCharge(bool RunCh, bool ACReq)
        - static void nissan_crc(uint8_t *data, uint8_t polynomial)
        - static int8_t fahrenheit_to_celsius(uint16_t fahrenheit)
        - void send_message(CANMessage *frame)
        - const char *getStateString()
        - const char *getChargeLimitingReasonString()
        - String getDTCString()
        - bool _OBCwake
        - bool _plugInserted
        - uint8_t _OBC_Charge_Status
        - uint8_t _OBC_Status_AC_Voltage
        - float _OBC_Charge_Power
        - float _OBCPowerSetpoint
        - float _batteryCurrent
        - float _batteryVoltage
        - float _batteryCurrentSetpoint
        - float _batteryVoltageSetpoint
        - bool _allowChargingBMS
        - float _maxAcPowerSetpoint
        - bool _activateCharging
        - uint8_t _counter_55b
        - uint8_t _counter_1db
        - uint8_t _counter_1dc
        - uint8_t _counter_1f2
        - uint8_t _counter_1d4
        - CHARGE_CURRENT_LIMITING_REASON_PDM _limitReason
        - STATE_PDM _state
        - DTC_PDM _dtc
    }
:::


:::mermaid
graph TD
    subgraph SWC [Software Component]
        direction TB

        subgraph Runnables
            direction TB
            R1[Runnable 1]
            R2[Runnable 2]
            R3[Runnable 3]
        end

        subgraph Ports
            direction TB
            P1[(Port 1)]
            P2[(Port 2)]
            P3[(Port 3)]
        end

        R1 -- Read/Write --> P1
        R2 -- Read --> P2
        R3 -- Write --> P3
        R1 -- Inter-Runnable Variable --> R2
        R2 -- Inter-Runnable Variable --> R3
    end
:::


:::mermaid
graph TD
    subgraph HeatingControllerASWC[Heating Controller ASWC]
        direction TB
        
        DialLedLeft[(DialLedLeft)]
        DialLedRight[(DialLedRight)]
        HeatingElementLeft((HeatingElement Left))
        HeatingElementRight((HeatingElement Right))
        SeatSwitchLeft>SeatSwitchLeft]
        SeatSwitchRight>SeatSwitchRight]
    end

    LedDial1[LedDial]
    LedDial2[LedDial]
    Coil1[Coil]
    Coil2[Coil]

    LedDial1 ---|LED| DialLedLeft
    LedDial2 ---|LED| DialLedRight
    Coil1 ---|Heating| HeatingElementLeft
    Coil2 ---|Heating| HeatingElementRight

    HeatingControllerASWC --> DialLedLeft
    HeatingControllerASWC --> DialLedRight
    HeatingControllerASWC --> HeatingElementLeft
    HeatingControllerASWC --> HeatingElementRight
    HeatingControllerASWC --> SeatSwitchLeft
    HeatingControllerASWC --> SeatSwitchRight

:::

:::mermaid
graph TD
    subgraph SoftwareComponent[Software Component]
        direction TB

        subgraph Runnable1[Runnable 1]
            direction TB
            S1[( )] --> R1[( )]
        end

        subgraph Runnable2[Runnable 2]
            direction TB
            P1[( )] --> P2[( )]
        end
    end

    subgraph SR_RPorts[SR r-Ports Receiver]
        direction TB
        SR_R1[( )] --> SR_R2[( )]
    end

    subgraph CS_PPorts[CS p-Ports Server]
        direction TB
        CS_P1[( )] --> CS_P2[( )]
    end

    subgraph SR_PPorts[SR p-Ports Sender]
        direction TB
        SR_P1[( )] --> SR_P2[( )]
    end

    subgraph CS_RPorts[CS r-Ports Client]
        direction TB
        CS_R1[( )] --> CS_R2[( )]
    end

    SR_RPorts --> Runnable1
    CS_PPorts --> Runnable2
    Runnable1 --> SR_PPorts
    Runnable2 --> CS_RPorts

:::