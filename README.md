# ws281x
WS281x ChibiOS driver for STM32 Platforms

##Sample Configuration

Configuration is used on STM32F103 minimal development board.

```
#define LEDCOUNT 15

struct ws281xLEDSetting LED1[] =
{
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
        {WS281X_GRB},
};

static ws2811Config ws2811_cfg =
{
    LEDCOUNT,
    LED1,
    {
        12000000,
        WS2811_BIT_PWM_WIDTH,
        NULL,
        {
            { PWM_OUTPUT_ACTIVE_HIGH, NULL },
            { PWM_OUTPUT_ACTIVE_HIGH, NULL },
            { PWM_OUTPUT_ACTIVE_HIGH, NULL },
            { PWM_OUTPUT_ACTIVE_HIGH, NULL }
        },
        0,
        TIM_DIER_UDE | TIM_DIER_CC2DE | TIM_DIER_CC1DE,
    },
    &PWMD2, // Timer has to be 16bit only
    1,
    WS2811_ZERO_PWM_WIDTH,
    WS2811_ONE_PWM_WIDTH,
    STM32_DMA1_STREAM7,
    7,
};
```
