Debugger Usage
==============

Environment
-----------
- Target environment: teensy41_debugger
- Source: test/debugger/main.cpp
- Upload/monitor port: COM6

Prerequisites
-------------
- Teensy 4.1 connected on COM6 (USB for upload/monitor)
- If you want source-level debugging, connect an SWD probe (for example, J-Link)

Build and upload
----------------
1) pio run -e teensy41_debugger -t upload

Start debugging (PlatformIO)
----------------------------
1) In VS Code: select environment "teensy41_debugger" and run "PlatformIO: Start Debugging"
2) CLI: pio debug -e teensy41_debugger
3) Set breakpoints in test/debugger/main.cpp
4) Watch the globals: g_sin_a and g_sin_b

Serial monitor (optional)
-------------------------
- pio device monitor -p COM6 -b 115200