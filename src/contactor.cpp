// #include <Arduino.h>

// /**
//  * This class represents all properties of a contactor with a feedback signal
//  * - on initialize it is checked, if the contactor is not welded
//  */

// class Contactor
// {
// public:
//     typedef enum State
//     {
//         INIT,    // Contactors are being initialized
//         OPEN,    // Contactors are open
//         OPENING, // Contactors are in the process of opening
//         CLOSED,  // Contactors are closed
//         CLOSING, // Contactors are in the process of closing
//         FAULT    // Contactors are in fault state
//     };

//     Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms) : _outputPin(outputPin),
//                                                                               _inputPin(inputPin),
//                                                                               _debounce_ms(debounce_ms),
//                                                                               _timeout_ms(timeout_ms),
//                                                                               _currentState(INIT)

//     {
//         pinMode(_outputPin, OUTPUT);
//         pinMode(_inputPin, INPUT_PULLUP);
//         digitalWrite(_outputPin, LOW);
//         _lastStateChange = millis();
//     }

//     void initialise()
//     {
//         if (_currentState == INIT)
//         {
//             digitalWrite(_outputPin, LOW);
//             if (digitalRead(_inputPin) == HIGH)
//             {
//                 _currentState = FAULT;
//                 digitalWrite(_outputPin, LOW);
//                 Serial.println("Fault: input signal high at init");
//             }
//             else
//             {
//                 _currentState = OPEN;
//                 _lastStateChange = millis();
//             }
//         }
//         else {
//             Serial.println("Fault: Contacor was already initialized. A second initialize is not possible.");
//         }
//     }

//     void close()
//     {
//         if (_currentState == OPEN || _currentState == OPENING)
//         {
//             _currentState = CLOSING;
//             _lastStateChange = millis();
//             digitalWrite(_outputPin, HIGH);
//         }
//     }

//     void open()
//     {
//         if (_currentState == CLOSED || _currentState == CLOSING)
//         {
//             _currentState = OPENING;
//             _lastStateChange = millis();
//             digitalWrite(_outputPin, LOW);
//         }
//     }

//     State getState()
//     {
//         return _currentState;
//     }

//     void update()
//     {
//         switch (_currentState)
//         {
//         case INIT:
//             break;
//         case OPEN:
//             // If open, but feedback gets high something is realy wrong -> fault
//             if (digitalRead(_inputPin) == HIGH)
//             {
//                 _currentState = FAULT;
//                 digitalWrite(_outputPin, LOW);
//             }
//             break;
//         case CLOSED:
//             // If closed but the feedback gets low, either energy supply for the contactor or the feedbackline was cut -> fault
//             if (digitalRead(_inputPin) == LOW)
//             {
//                 _currentState = FAULT;
//                 digitalWrite(_outputPin, LOW);
//             }
//             break;
//         case OPENING:
//             if ((millis() - _lastStateChange) > _timeout_ms) // Check if timedout
//             {
//                 _currentState = FAULT;
//                 digitalWrite(_outputPin, LOW);
//             }
//             else
//             {
//                 if ((millis() - _lastStateChange) > _debounce_ms) // Check if a state change is already allowed
//                 {
//                     if (digitalRead(_inputPin) == LOW) // If low, then the transition is to open is done
//                     {
//                         _currentState = OPEN;
//                         _lastStateChange = millis();
//                     }
//                 }
//             }
//             break;
//         case CLOSING:
//             if ((millis() - _lastStateChange) > _timeout_ms) // Check if timedout
//             {
//                 _currentState = FAULT;
//                 digitalWrite(_outputPin, LOW);
//             }
//             else
//             {
//                 if ((millis() - _lastStateChange) > _debounce_ms) // Check if a state change is already allowed
//                 {
//                     if (digitalRead(_inputPin) == HIGH) // If high, then the transition is to closed is done
//                     {
//                         _currentState = CLOSED;
//                         _lastStateChange = millis();
//                     }
//                 }
//             }
//             break;
//         case FAULT:
//             digitalWrite(_outputPin, LOW); // Keep writing low as safety
//             break;
//         }
//     }

// private:
//     int _outputPin;
//     int _inputPin;
//     int _debounce_ms;
//     int _timeout_ms;
//     unsigned long _lastStateChange;
//     State _currentState;
// };
