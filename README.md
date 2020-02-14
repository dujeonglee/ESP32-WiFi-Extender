# WiFi station example

(See the README.md file in the upper level 'examples' directory for more information about examples.)


## How to use example

### Configure the project

```
make menuconfig
```

* Set serial port under Serial Flasher Options.

* Set WiFi SSID and WiFi Password and Maximum retry under Example Configuration Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```