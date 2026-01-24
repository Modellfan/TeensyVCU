TeensyVCU
=========

Legacy branch note
------------------

The `legacy` branch is the code base from before the change from a Jaguar I-Pace ISA shunt
to a voltage-sensing shunt ISA IVT-S U3 and before usage of an AI agent.

I2C (AT24C)
-----------

On Teensy 4.1, the default `Wire` bus uses:
- SDA: pin 18
- SCL: pin 19
