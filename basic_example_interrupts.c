// This application illustrates a simple use of interrupts for both GPIO and Timer32


/*
 * The application implements a simple debouncing logic. Whenever a high-to-low transition is sensed on Port 1, pin 1 (connected to S1)
 * an interrupt is triggered. The ISR changes a variable called S1modified to tell the other function take further action.
 * Another function called S1tapped() starts Timer32 to wait for debouncing time. Again, when the Timer32 is expired an interrupt is
 * triggered. Another global boolean is set to true in ISR for the Timer to tell the other functions, specifically S1tapped(), that the timer has
 * expired.
 */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Based on system clock of 3MHz and prescaler of 1, this is a 200ms wait
// TODO: change this number to see how the debouncing behavior changes (try 6000, 60000, 300000)
#define DEBOUNCE_WAIT 600000

// This function initializes all the peripherals
void initialize();

void TurnOn_Launchpad_LED1();
void TurnOff_Launchpad_LED1();
void Toggle_Launchpad_LED1();

void TurnOn_Launchpad_LED2Blue();
void TurnOff_Launchpad_LED2Blue();
void Toggle_Launchpad_LED2Blue();

// The global variables used by the ISRs

// A boolean variable that is true when a high-to-low transition is sensed on S1
volatile bool S1modified = false;

// A boolean variable that is true when Timer32 is expired
volatile bool TimerExpired = false;


// The ISR for port 1 (all of port 1, not any specific pin)
// Hold the control key on your keyboard and click on the name of this function to see where it takes you
// We did not choose the name of this function. Any time a port 1 interrupt happens this function is called automoatically.
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

// This is also an ISR, but we picked our own name.
// Since we picked the name, we have to register this function to become an official ISR
void Debounce_Over()
{
    // We use this global variable to communicate with the main
    TimerExpired = true;

    // We tell the Timer32 to remove the interrupt as we already handled it.
    Timer32_clearInterruptFlag(TIMER32_0_BASE);
}

bool S1tapped()
{
    if (S1modified)
    {
        // If a change on S1 is sensed, we start the debounce timer
        Timer32_setCount(TIMER32_0_BASE, DEBOUNCE_WAIT);
        Timer32_startTimer(TIMER32_0_BASE, true);

        // LE2 blue is on during the debouncing wait
        // This is only for debugging purpose and for you to get a sense that the debouncing has begun
        TurnOn_Launchpad_LED2Blue();

        // It is important to revert back this boolean variable. Otherwise, next loop
        // we will again start the timer.
        S1modified = false;

        // at this point we still don't say the buttons is tapped
        return false;
    }

    else if (TimerExpired)
    {
        // LE2 blue is on during the debouncing wait. We turn it off here.
        TurnOff_Launchpad_LED2Blue();

        // Again, since we took action for the expired timer we should revert back the boolean flag.
        TimerExpired = false;

        // Debounce wait is over and we are taking action
        return true;
    }

    // if none of the above the button is not tapped
    return false;
}

void TurnOn_Launchpad_LED1();
void TurnOff_Launchpad_LED1();
char SwitchStatus_Launchpad_Button1();

int main(void)
{

    initialize();

    while (1) {
        // Enters the Low Power Mode 0 - the processor is asleep and only responds to interrupts
        PCM_gotoLPM0();

        if (S1tapped())
            Toggle_Launchpad_LED1();
    }
}


// Initialization part is always device dependent and therefore does not change much with HAL
void initialize()
{

    // Stop watchdog timer
    // We do this at the beginning of all our programs for now.Later we learn more about it.
    WDT_A_hold(WDT_A_BASE);

    // Initializing LED1, which is on Pin 0 of Port P1 (from page 37 of the Launchpad User Guide)
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    // blue LED on Launchpad
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN2);

    // Initializing S1 (switch 1 or button 1),
    // which is on Pin1 of Port 1 (from page 37 of the Launchpad User Guide)
    GPIO_setAsInputPinWithPullUpResistor (GPIO_PORT_P1, GPIO_PIN1);

    // enable interrupt on port 1, pin 1
    GPIO_enableInterrupt(GPIO_PORT_P1,
                         GPIO_PIN1);

    // the interrupt is triggered on high to low transition (tapping)
    GPIO_interruptEdgeSelect(GPIO_PORT_P1,
                             GPIO_PIN1,
                             GPIO_HIGH_TO_LOW_TRANSITION);

    // enable the port 1 interrupt
    Interrupt_enableInterrupt(INT_PORT1);

    // Initialize the timers needed for debouncing
    Timer32_initModule(TIMER32_0_BASE, // There are two timers, we are using the one with the index 0
                       TIMER32_PRESCALER_1, // The prescaler value is 1; The clock is not divided before feeding the counter
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

    // register the Debounce_over() function as the ISR for Timer32_0
    Timer32_registerInterrupt(INT_T32_INT1, Debounce_Over);

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

void TurnOn_Launchpad_LED2Blue()
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
}

void TurnOff_Launchpad_LED2Blue()
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
}

void Toggle_Launchpad_LED2Blue()
{
    GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN2);
}



