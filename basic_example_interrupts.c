// This application illustrates a simple use of interrupts for both GPIO and Timer32
// A transition on LB1 turns LL1 on for about 2 seconds

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Based on system clock of 3MHz and prescaler of 1, this is a 2000ms wait
#define TIMER_WAIT 6000000

// This function initializes all the peripherals
void initialize();

void TurnOn_LL1();
void TurnOff_LL1();
void TurnOn_LLG();
void TurnOff_LLG();

// The global variables used by the ISRs

// A boolean variable that is true when a high-to-low transition is sensed on S1
volatile bool S1modifiedFlag = false;

// A boolean variable that is true when Timer32 is expired
volatile bool TimerExpiredFlag = false;


// The ISR for port 1 (all of port 1, not any specific pin)
// Hold the control key on your keyboard and click on the name of this function to see where it takes you
// We did not choose the name of this function. Any time a port 1 interrupt happens this function is called automoatically.
void PORT1_IRQHandler() {

    // In our case on only S1 (attached to Pin1) can provide interrupt.
    // However, this is not true for all programs, so in general, you have to check
    // to make sure what pin created the interrupt
    if (GPIO_getInterruptStatus(GPIO_PORT_P1,
                                GPIO_PIN1))
        S1modifiedFlag = true;

    // The very critical step to make sure once we leave ISR, we don't come back to ISR again.
    // This tells the GPIO, the CPU has heard the interrupt and it should clear it.
    GPIO_clearInterruptFlag(GPIO_PORT_P1,
                            GPIO_PIN1);
}

// This is also an ISR, but we picked our own name.
// Since we picked the name, we have to register this function to become an official ISR
void TimerExpired()
{
    // We use this global variable to communicate with the main
    TimerExpiredFlag = true;

    // We tell the Timer32 to remove the interrupt as we already handled it.
    Timer32_clearInterruptFlag(TIMER32_0_BASE);
}


int main(void)
{

    initialize();

    while (1) {
        // Enters the Low Power Mode 0 - the processor is asleep and only responds to interrupts
        // LLG signifies asleep processor. During runtime, it appears the processor is always asleep
        // Debugging shows that the processor does wake up, but we blink and we miss it!
        TurnOn_LLG();
        PCM_gotoLPM0();
        TurnOff_LLG();

        if (S1modifiedFlag) {
            TurnOn_LL1();

            // This is important. Otherwise, the next time we enter the loop, we think there has been an interrupt
            S1modifiedFlag = false;

            // start the 2-second timer. The timer is configured to give interrupts
            Timer32_setCount(TIMER32_0_BASE, TIMER_WAIT);
            Timer32_startTimer(TIMER32_0_BASE, true);
        }

        if (TimerExpiredFlag) {
            TurnOff_LL1();

            // This is important. Otherwise, we will think the timer recently expired.
            TimerExpiredFlag = false;
        }

    }
}

void initLEDs() {

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN1);

    TurnOff_LL1();
    TurnOff_LLG();
}

void initLB1() {
    // Initializing S1 (switch 1 or button 1),
    // which is on Pin1 of Port 1 (from page 37 of the Launchpad User Guide)
    GPIO_setAsInputPinWithPullUpResistor (GPIO_PORT_P1, GPIO_PIN1);

    GPIO_clearInterruptFlag(GPIO_PORT_P1,
                            GPIO_PIN1);

    // enable interrupt on port 1, pin 1
    GPIO_enableInterrupt(GPIO_PORT_P1,
                         GPIO_PIN1);

    // the interrupt is triggered on high to low transition (tapping)
    GPIO_interruptEdgeSelect(GPIO_PORT_P1,
                             GPIO_PIN1,
                             GPIO_HIGH_TO_LOW_TRANSITION);

    // enable the port 1 interrupt
    Interrupt_enableInterrupt(INT_PORT1);

}

void initTimer(){
    // Initialize the timers needed for debouncing
    Timer32_initModule(TIMER32_0_BASE, // There are two timers, we are using the one with the index 0
                       TIMER32_PRESCALER_1, // The prescaler value is 1; The clock is not divided before feeding the counter
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

    // register the TimerExpired() function as the ISR for Timer32_0
    Timer32_registerInterrupt(INT_T32_INT1, TimerExpired);

    Timer32_clearInterruptFlag(TIMER32_0_BASE);

    Interrupt_enableInterrupt(INT_T32_INT1);

}

void initialize()
{

    WDT_A_hold(WDT_A_BASE);

    initLEDs();
    initLB1();
    initTimer();
}

void TurnOn_LL1()
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
}
void TurnOff_LL1()
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
}

// LLG
void TurnOn_LLG()
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
}

void TurnOff_LLG()
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
}






