#include "gpio_ctrl.h"
#include <stdlib.h>


void init_jpeg_feedback_gpio(){
    system("echo 440 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio440/direction");
    system("echo 0 > /sys/class/gpio/gpio440/value");
}


void set_feedback_gpio(int value){
    if(value == 0){
        system("echo 0 > /sys/class/gpio/gpio440/value");
    }else{
        system("echo 1 > /sys/class/gpio/gpio440/value");
    }
}