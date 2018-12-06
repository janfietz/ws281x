/**
 * @file    ws281x.c
 * @brief   WS281X Driver code.
 *
 * @addtogroup WS281X
 * @{
 */

#include "hal.h"
#include "ws281x.h"
#include <string.h>

#if HAL_USE_WS281X || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
static void ws281xSetupDMA(ws281xDriver *ws281xp)
{
    dmaStreamSetMemory0(ws281xp->config->dmastp, ws281xp->framebuffer);
    dmaStreamSetTransactionSize(ws281xp->config->dmastp, ws281xp->frameCount);
    dmaStreamSetMode(ws281xp->config->dmastp,
       STM32_DMA_CR_TCIE |
       STM32_DMA_CR_DIR_M2P |
       STM32_DMA_CR_MINC |
       STM32_DMA_CR_MSIZE_HWORD |
       STM32_DMA_CR_PSIZE_HWORD |
       STM32_DMA_CR_PL(3) |
       STM32_DMA_CR_CHSEL(ws281xp->config->dmaChannel));
}

static void ws281xStartTransaction(ws281xDriver *ws281xp)
{
    ws281xp->transactionActive = true;
    ws281xSetupDMA(ws281xp);

    /*
    * Load buffer into preload register
    */
    ws281xp->config->pwmd->tim->EGR |= STM32_TIM_EGR_UG;
    /* Wait until RESET of UG bit */
    while ((ws281xp->config->pwmd->tim->EGR & STM32_TIM_EGR_UG) > 0) {}

    dmaStreamEnable(ws281xp->config->dmastp);
    // enable counting
    ws281xp->config->pwmd->tim->CR1 |= STM32_TIM_CR1_CEN;
}

static void ws281xStopTransaction(ws281xDriver *ws281xp)
{
    // disable counting
    ws281xp->config->pwmd->tim->CR1 &= ~STM32_TIM_CR1_CEN;
    dmaStreamDisable(ws281xp->config->dmastp);
    ws281xp->transactionActive = false;
}


static void ws281xSetColorBits(uint8_t red, uint8_t green, uint8_t blue,
        const struct ws281xLEDSetting* ledSettings, uint16_t* frame,
        uint16_t zero, uint16_t one)
{
    /* create a temporary color to avoid checking the setting each bit */
    uint8_t colorComponents[] = {red, green, blue};

    if (ledSettings->colorComponentOrder == WS281X_RGB)
    {
        colorComponents[0] = green;
        colorComponents[1] = red;
    }

    int bit;
    uint16_t* frameByte = frame;
    for (bit = 0; bit < 8; bit++)
    {
        frameByte[0] = ((colorComponents[0] << bit) & 0b10000000 ? one : zero);
        frameByte[8] = ((colorComponents[1] << bit) & 0b10000000 ?  one : zero);
        frameByte[16] = ((colorComponents[2] << bit) & 0b10000000 ?  one : zero);
        frameByte++;
    }
}

static void ws281xUpdateFrameBuffer(ws281xDriver *ws281xp)
{
    memcpy(ws281xp->framebuffer, ws281xp->framebuffer02, ws281xp->frameCount * 2);
    ws281xp->updateframebuffer = false;
}

/**
 * @brief   Shared end-of-tx service routine.
 *
 * @param[in] i2sp      pointer to the @p ws281xDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void ws281x_update_interrupt(ws281xDriver *ws281xp, uint32_t flags)
{
    if ((flags & STM32_DMA_ISR_TCIF) != 0) {
        /* Transfer complete processing.*/
        osalSysLockFromISR();
        ws281xStopTransaction(ws281xp);

        if (ws281xp->updateframebuffer == true)
        {
            ws281xUpdateFrameBuffer(ws281xp);
            chBSemSignalI(&ws281xp->bsemUpdateFrame);

            ws281xStartTransaction(ws281xp);
        }
        osalSysUnlockFromISR();
    }
}


/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   WS281X Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void ws281xInit(void) {

}

/**
 * @brief   Initializes the standard part of a @p WS281XDriver structure.
 *
 * @param[out] ws281xp     pointer to the @p WS281XDriver object
 *
 * @init
 */
void ws281xObjectInit(ws281xDriver *ws281xp) {

    ws281xp->state = WS281X_STOP;
    ws281xp->config = NULL;
}

/**
 * @brief   Configures and activates the WS281X peripheral.
 *
 * @param[in] ws281xp      pointer to the @p WS281XDriver object
 * @param[in] config    pointer to the @p WS281XConfig object
 *
 * @api
 */

void ws281xStart(ws281xDriver *ws281xp, const ws281xConfig *config) {

    osalDbgCheck((ws281xp != NULL) && (config != NULL));

    osalSysLock();
    osalDbgAssert((ws281xp->state == WS281X_STOP) ||
        (ws281xp->state == WS281X_READY), "invalid state");
    ws281xp->config = config;

    osalSysUnlock();

    ws281xp->frameCount = (ws281xp->config->ledCount * 24) + 50;

    ws281xp->framebuffer = chHeapAlloc(NULL, ws281xp->frameCount * 2);
    ws281xp->framebuffer02 = chHeapAlloc(NULL, ws281xp->frameCount * 2);
    ws281xp->updateframebuffer = false;

    /*
     * initialize frame buffer with black
     */
    uint32_t j;
    for (j = 0; j < ((ws281xp->config->ledCount) * 24); j++)
    {
        ws281xp->framebuffer[j] = ws281xp->config->pwmZeroWidth;
        ws281xp->framebuffer02[j] = ws281xp->config->pwmZeroWidth;
    }

    for (; j < ws281xp->frameCount; j++)
    {
        ws281xp->framebuffer[j] = 0;
        ws281xp->framebuffer02[j] = 0;
    }

    chBSemObjectInit(&ws281xp->bsemUpdateFrame, TRUE);
    chBSemReset(&ws281xp->bsemUpdateFrame, TRUE);

    dmaStreamAllocate(ws281xp->config->dmastp, WS281X_IRQ_PRIORITY,
        (stm32_dmaisr_t)ws281x_update_interrupt,
        (void*)ws281xp);
    dmaStreamSetPeripheral(ws281xp->config->dmastp,
        ws281xp->config->pwmd->tim->CCR + ws281xp->config->pwmChannel);
    dmaStreamSetMemory0(ws281xp->config->dmastp, ws281xp->framebuffer);

    ws281xSetupDMA(ws281xp);
    pwmStart(ws281xp->config->pwmd, &ws281xp->config->pwmConfig);

    /* stop counting */
    ws281xp->config->pwmd->tim->CR1 &= ~STM32_TIM_CR1_CEN;
    ws281xp->config->pwmd->tim->CR1  &= ~STM32_TIM_CR1_URS;

    osalSysLock();
    ws281xp->state = WS281X_ACTIVE;
    osalSysUnlock();
}

/**
 * @brief   Deactivates the WS281X peripheral.
 *
 * @param[in] ws281xp      pointer to the @p WS281XDriver object
 *
 * @api
 */
void ws281xStop(ws281xDriver *ws281xp) {

    osalDbgCheck(ws281xp != NULL);

    osalSysLock();
    osalDbgAssert((ws281xp->state == WS281X_STOP) ||
        (ws281xp->state == WS281X_READY),
            "invalid state");

    ws281xp->state = WS281X_STOP;
    osalSysUnlock();
}

void ws281xSetColor(ws281xDriver *ws281xp, int ledNum,
    uint8_t red, uint8_t green, uint8_t blue)
{
    osalDbgCheck(ws281xp != NULL);
    osalDbgCheck(ledNum < ws281xp->config->ledCount);

    osalSysLock();
    osalDbgAssert((ws281xp->state == WS281X_READY) ||
        (ws281xp->state == WS281X_ACTIVE), "invalid state");

    uint16_t* buffer = ws281xp->framebuffer02;
    osalSysUnlock();

    uint16_t* frame = buffer + (24 * ledNum);
    ws281xSetColorBits(red, green, blue, &ws281xp->config->ledSettings[ledNum], frame,
            ws281xp->config->pwmZeroWidth, ws281xp->config->pwmOneWidth);
}

void ws281xUpdate(ws281xDriver *ws281xp)
{
    osalDbgCheck(ws281xp != NULL);
    osalSysLock();
    if ((ws281xp->state != WS281X_READY) && (ws281xp->state != WS281X_ACTIVE))
    {
        osalSysUnlock();
        return;
    }

    if (ws281xp->updateframebuffer == true)
    {
        // wait for finishing previous update
        chBSemWaitS(&ws281xp->bsemUpdateFrame);
    }
    ws281xp->updateframebuffer = true;

    if (ws281xp->transactionActive == false)
    {
        ws281xUpdateFrameBuffer(ws281xp);
        osalSysUnlock();
        ws281xStartTransaction(ws281xp);
    }
    else
    {
        osalSysUnlock();
    }
}

#endif /* HAL_USE_WS281X */

/** @} */
