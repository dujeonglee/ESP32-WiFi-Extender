/**
 * @file main.cpp
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief ESP32 L3 WiFi Extender
 * @version 0.1
 * @date 2020-02-08
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "flash.h"
#include "state_machine.h"

extern "C" {
	void app_main(void);
}

void app_main()
{
    Flash::instance();
    StateMachine::instance()->start();
}
