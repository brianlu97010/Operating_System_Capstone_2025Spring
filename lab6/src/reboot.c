#include "reboot.h"
#include "utils.h"
#include "registers.h"

void reset(){
    unsigned int val;
    
    val = regRead(PM_RSTC);             // read the reset control register
    val &= PM_RSTC_CLEAR;               // clear the existing setting of reset control register
    val |= (PM_PASSWD|PM_RSTC_REBOOT);  // set (need the password) the full reset to reset control register
    regWrite(PM_RSTC, val); 
    
    regWrite(PM_WDOG, 10|PM_PASSWD);    // set the watchdog timer with a timeout 10 ticks ~ 150us
}