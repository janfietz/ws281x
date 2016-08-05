
/**
 * @file    ws281x.h
 * @brief   WS281X Driver macros and structures.
 *
 * Driver for WS281x driven LED Strips. Driver uses a timer channel to generate datastream for LED strip. It can be used on
 * STM32 platforms.
 *
 * @addtogroup WS281X
 * @{
 */

#ifndef _WS281X_H_
#define _WS281X_H_

#include "ch.h"
#include "hal.h"

#if HAL_USE_WS281X || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/
#define WS2811_BIT_PWM_WIDTH 30 //2.5 us
#define WS2811_ZERO_PWM_WIDTH 6 //0.5 us
#define WS2811_ONE_PWM_WIDTH 15 //1.2 us

#define WS2812_BIT_PWM_WIDTH 15 //1.25 us
#define WS2812_ZERO_PWM_WIDTH 4 //0.35 us
#define WS2812_ONE_PWM_WIDTH 9 //0.7 us

#define WS281X_RGB 1
#define WS281X_GRB 2

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/
/**
 * @brief   WS281x interrupt priority level setting.
 */
#if !defined(WS281X_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define WS281X_IRQ_PRIORITY         10
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/
struct ws281xLEDSetting
{
    uint8_t colorComponentOrder;
};

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  WS281X_UNINIT = 0,                   /**< Not initialized.                   */
  WS281X_STOP = 1,                     /**< Stopped.                           */
  WS281X_READY = 2,                    /**< Ready.                             */
  WS281X_ACTIVE = 3,                   /**< Active.                            */
} ws281xState_t;
/**
 * @brief   Type of a structure representing an ws281xDriver driver.
 */
typedef struct ws281xDriver ws281xDriver;
/**
 * @brief   Driver configuration structure.
 */
typedef struct {
    /*
    * count of LED in strip
    */
    uint16_t ledCount;
    /*
    * LED settings
    */
    struct ws281xLEDSetting* ledSettings;
    /*
     * Timer settings
     */
    PWMConfig pwmConfig;
    /*
    * Timer
    */
    PWMDriver *pwmd;
    /*
    * Timer channel to use for data output
    */
    pwmchannel_t pwmChannel;
    /*
    * Timer compare counter setting for signaling a Zero
    */
    uint16_t pwmZeroWidth;
    /*
    * Timer compare counter setting for signaling a One
    */
    uint16_t pwmOneWidth;
    /*
    * DMA Stream to write compare counter values to channel register.
    * Channel has to be connected to used timer channel.
    */
    stm32_dma_stream_t *dmastp;
    /*
     * DMA Channel
     */
    uint32_t dmaChannel;

} ws281xConfig;


/**
 * @brief   Structure representing an ws281x driver.
 */
struct ws281xDriver {
  /**
   * @brief   Driver state.
   */
	ws281xState_t                state;
  /**
   * @brief   Current configuration data.
   */
  const ws281xConfig           *config;
  /* End of the mandatory fields.*/
  uint32_t frameCount;
  uint16_t *framebuffer;
  uint16_t *framebuffer02;
  bool transmitBuffer02;
  bool updateframebuffer;
  bool transactionActive;
  binary_semaphore_t bsemUpdateFrame;
};
/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void ws281xInit(void);
  void ws281xObjectInit(ws281xDriver *ws281xp);
  void ws281xStart(ws281xDriver *ws281xp, const ws281xConfig *config);
  void ws281xStop(ws281xDriver *ws281xp);
  void ws281xSetColor(ws281xDriver *ws281xp, int ledNum,
          uint8_t red, uint8_t green, uint8_t blue);
  void ws281xUpdate(ws281xDriver *ws281xp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WS281X */

#endif /* _WS281X_H_ */

/** @} */
