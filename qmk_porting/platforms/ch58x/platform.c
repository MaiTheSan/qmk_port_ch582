#include "platform_deps.h"
#include "quantum_keycodes.h"

volatile uint8_t kbd_protocol_type = 0;
volatile uint8_t GPIOTigFlag;
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = { 0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02 };
#endif

/* we have already set the correct _putchar() function 
 * disable this to avoid further modification */
void print_set_sendchar(sendchar_func_t func)
{
}

#ifndef PLF_DEBUG
/* platform uart log output is disabled
 * so we fake a _putchar() to avoid link error */
void _putchar(char character)
{
}
#endif

void platform_setup()
{
#if (defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(Fsys);
    DelayMs(5);
    PowerMonitor(ENABLE, HALevel_2V1);
#ifdef DEBUG
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
#ifdef LSE_FREQ
    sys_safe_access_enable();
    SAFEOPERATE;
    R8_CK32K_CONFIG |= RB_CLK_OSC32K_XT | RB_CLK_INT32K_PON | RB_CLK_XT32K_PON;
    sys_safe_access_disable();
#else
    sys_safe_access_enable();
    SAFEOPERATE;
    R8_CK32K_CONFIG &= ~(RB_CLK_OSC32K_XT | RB_CLK_XT32K_PON);
    R8_CK32K_CONFIG |= RB_CLK_INT32K_PON;
    sys_safe_access_disable();
    Calibration_LSI(Level_64);
#endif
    // TODO: set some GPIO for mode decision

    PRINT("Chip start, %s\n", VER_LIB);

    if (KC_VENDOR_BT1 < SAFE_RANGE) {
        // keycode violation
        PRINT("Error: overlap detected between QMK and Vendor defined keycodes!\n");
        DelayMs(1000);
        SYS_ResetExecute();
    }

#if 0
    PRINT("EEPROM dump: \n");
    for (uint8_t i = 0; i < 8; i++) {
        PRINT("Page: %d\n", i);
        uint8_t eeprom_dump[0x1000] = {};
        EEPROM_READ(i * 0x1000, eeprom_dump, 0x1000);
        for (uint16_t j = 0; j < 0x1000; j++) {
            PRINT("0x%02x ", eeprom_dump[j]);
            DelayUs(20);
        }
        PRINT("\n\n");
        DelayMs(1);
    }
    PRINT("End of EEPROM dump.\n\n");
#endif
}

#ifdef BLE_ENABLE

void platform_setup_ble()
{
    uint32_t pin_a = GPIO_Pin_All, pin_b = GPIO_Pin_All;

#ifdef PLF_DEBUG
    pin_a &= ~bTXD1;
    pin_a &= ~bRXD1;
#endif
#ifdef LSE_FREQ
    pin_a &= ~bX32KI;
    pin_a &= ~bX32KO;
#endif
#ifdef WS2812_DRIVER_SPI
    pin_a &= ~bMOSI;
#elif defined WS2812_DRIVER_PWM
#if WS2812_PWM_DRIVER == 1
    pin_a &= ~bTMR1;
#elif WS2812_PWM_DRIVER == 2
    pin_a &= ~bTMR2;
#endif
#endif
    pin_b &= ~bUDP;
    pin_b &= ~bUDM;
    pin_b &= ~bU2DP;
    pin_b &= ~bU2DM;
    GPIOA_ModeCfg(pin_a, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(pin_b, GPIO_ModeIN_PU);

    // clock gate for unused peripherals
    sys_safe_access_enable();
    R8_SLP_CLK_OFF0 |= RB_SLP_CLK_UART3 | RB_SLP_CLK_UART2 | RB_SLP_CLK_UART0;
    sys_safe_access_enable();
#ifndef PLF_DEBUG
    R8_SLP_CLK_OFF0 |= RB_SLP_CLK_UART1;
#endif
    sys_safe_access_enable();
    R8_SLP_CLK_OFF0 |= RB_SLP_CLK_TMR3 | RB_SLP_CLK_TMR0;
    sys_safe_access_enable();
    R8_SLP_CLK_OFF1 |= RB_SLP_CLK_USB | RB_SLP_CLK_USB2 | RB_SLP_CLK_I2C;
    sys_safe_access_enable();
    R8_SLP_CLK_OFF1 |= RB_SLP_CLK_PWMX;
#if WS2812_PWM_DRIVER != 1
    sys_safe_access_enable();
    R8_SLP_CLK_OFF0 |= RB_SLP_CLK_TMR1;
#endif
#if WS2812_PWM_DRIVER != 2
    sys_safe_access_enable();
    R8_SLP_CLK_OFF0 |= RB_SLP_CLK_TMR2;
#endif
#ifndef WS2812_DRIVER_SPI
    sys_safe_access_enable();
    R8_SLP_CLK_OFF1 |= RB_SLP_CLK_SPI1 | RB_SLP_CLK_SPI0;
#endif
    sys_safe_access_disable();

    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
}

#endif