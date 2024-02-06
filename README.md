# Smart Plug

This repository contains a simple software I wrote & am currently using on SmartDGM PP-W162 SmartPlugs (more known as Tuya).

## Features
- Hosts local website allowing checking parameters and changing configurations.
- Remote enabling and disabling power relay (http://IP/enable or disable, or via the website).
- Showing realtime voltage,current and active power statistics on the website.
- Reporting statistics mentioned above to a data collection server (link below).
- Collecting energy usage statistics (how many Wh used in period of time) and sending it to data collection server every period of time.
- Ability to perform OTA update via the website instead of having to connect to UART.
