# target-3-server

`target-3` is an open-source recovery machine for shooting targets.

The server and remote communicate with Bluetooth Low Energy (BLE) protocol.
This server program runs on small hardware such as IoT hardware.
The hardware is attached on the servo motor and the metal arm to recover shooting targets.

Please refer to these repositories if you have interest in other modules of `target-3`.

* Remote: https://github.com/katsuster/target-3-remote
* Board: https://github.com/katsuster/target-3-board


## Requires

* Software
  * Arduino IDE
  * Boards Manager: M5Stack
* Hardware
  * M5Stamp C3


## How to use

Same as target-6-server.

* target-6-server: https://github.com/katsuster/target-6-server


## Source structures

* c3/
  * For M5Stamp C3


## LED

The RGB LED of M5Stamp C3 shows the state of the server.
If several states are active at the same time, the upper one in the table is shown.

| Color | Pattern | State |
|--|--|--|
| Red   | On for 3 seconds     | Error: the servo was powered off forcibly |
| Green | On                   | The servo is moving |
| Blue  | On                   | Connected via BLE |
| Blue  | Blink (250ms on/off) | Not connected, advertising |


## Other documents

* [GPIO](doc/gpio.md)
