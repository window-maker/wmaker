

/*
 * Handle events for non-GUI based applications
 */

#include "WINGsP.h"



void
WHandleEvents()
{
    /* Check any expired timers */
    W_CheckTimerHandlers();

    /* We need to make sure that we have some input handler before calling
     * W_CheckIdleHandlers() in a while loop, because else the while loop
     * can run forever (if some idle handler reinitiates itself).
     */
    if (W_HaveInputHandlers()) {
        /* Do idle and timer stuff while there are no input events */
        /* Check again if there are still input handlers, because some idle
         * handler could have removed them */
        while (W_CheckIdleHandlers() && W_HaveInputHandlers() &&
               !W_HandleInputEvents(False, -1)) {
            /* dispatch timer events */
            W_CheckTimerHandlers();
        }
    } else {
        W_CheckIdleHandlers();
        /* dispatch timer events */
        W_CheckTimerHandlers();
    }

    W_HandleInputEvents(True, -1);

    /* Check any expired timers */
    W_CheckTimerHandlers();
}


