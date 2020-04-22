// This application illustrates a simple use of interrupts for both GPIO and Timer32
// Whenever the GPIO experiences a "high" to "low" transition, it sends an interrupt
// Whenever Timer32 expires, it sends an interrupt.
// The code is written such that whenever there is a "high" to "low" transition on S1
// on Launchpad, the Launchpad LED1 is turned on for half a second.
// Note that since the button is bouncy, even when the button is released, an interrupt is
// generated.

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Based on system clock of 3MHz and prescaler of 1
#define HALF_SECOND 1500000

// This function initializes all the peripherals
void initialize();

void TurnOn_Launchpad_LED1();
void TurnOff_Launchpad_LED1();
void Toggle_Launchpad_LED1();

// The global variables used by the ISRs

// A boolean variable that is true when a transition from "high" to "low" is sensed on S1
volatile bool S1modified = false;

// A boolean variable that is true when Timer32 is expired
volatile bool TimerExpired = false;


// The ISR for port 1 (all of port 1, not any specific pin)
void PORT1_IRQHandler() {

    // In our case on only S1 (attached to Pin1) can provide interrupt.
    // However, this is not true for all programs, so in general, you have to check
    // to make sure what pin created the interrupt
    if (GPIO_getInterruptStatus(GPIO_PORT_P1,
                                GPIO_PIN1))
        S1modified = true;

    // The very critical step to make sure once we leave ISR, we don't come back to ISR again.
    // This tells the GPIO, the CPU has heard the interrupt and it should clear it.
    GPIO_clearInterruptFlag(GPIO_PORT_P1,
                             GPIO_PIN1);
}

void T32_INT1_IRQHandler()
{
    // We use this global variable to communicate with the main
    TimerExpired = true;

    // We tell the Timer32 to remove the interrupt as we already handled it.
    Timer32_clearInterruptFlag(TIMER32_0_BASE);
}

void TurnOn_Launchpad_LED1();
void TurnOff_Launchpad_LED1();
char SwitchStatus_Launchpad_Button1();

int main(void)
{

    initialize();

    while (1) {
        if (S1modified)
        {
            // If a change on S1 is sensed, we start the half-second timer and turn on the LED
            Timer32_setCount(TIMER32_0_BASE, HALF_SECOND);
            Timer32_startTimer(TIMER32_0_BASE, true);
            TurnOn_Launchpad_LED1();

            // It is important to revert back this boolean variable. Otherwise, next loop
            // we will again start the timer.
            S1modified = false;
        }

        if (TimerExpired)
        {
            // Once the timer is expired, we turn off the LED
            TurnOff_Launchpad_LED1();

            // Again, since we took action for the expired timer we should revert back the boolean flag.
            TimerExpired = false;
        }

    }
}


// Initialization part is always device dependent and therefore does not change much with HAL
void initialize()
{

    // step 1: Stop watchdog timer
    // We do this at the beginning of all our programs for now.Later we learn more about it.
    WDT_A_hold(WDT_A_BASE);

    // step 2: Initializing LED1, which is on Pin 0 of Port P1 (from page 37 of the Launchpad User Guide)
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    // step 3: Initializing S1 (switch 1 or button 1),
    // which is on Pin1 of Port 1 (from page 37 of the Launchpad User Guide)
    GPIO_setAsInputPinWithPullUpResistor (GPIO_PORT_P1, GPIO_PIN1);

    GPIO_enableInterrupt(GPIO_PORT_P1,
                         GPIO_PIN1);

    GPIO_interruptEdgeSelect(GPIO_PORT_P1,
                             GPIO_PIN1,
                             GPIO_HIGH_TO_LOW_TRANSITION);

    Interrupt_enableInterrupt(INT_PORT1);

    // Initialize the timers needed for debouncing
    Timer32_initModule(TIMER32_0_BASE, // There are two timers, we are using the one with the index 0
                       TIMER32_PRESCALER_1, // The prescaler value is 1; The clock is not divided before feeding the counter
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

    Interrupt_enableInterrupt(INT_T32_INT1);

}

void TurnOn_Launchpad_LED1()
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
}
void TurnOff_Launchpad_LED1()
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
}

void Toggle_Launchpad_LED1()
{
    GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
}
char SwitchStatus_Launchpad_Button1()
{
    return (GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN1));
}


