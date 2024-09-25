#include <rtthread.h>
#include <stdint.h>
#include <board.h>
#include <usbh_core.h>

extern int cdc_acm_demo(void);
extern int cdc_acm_multi_demo(void);

void usb_demo_init(void)
{
    // cdc_acm_demo();
    cdc_acm_multi_demo();

// #ifdef BSP_USING_USB_HOST
//     usbh_initialize(0, USB_OTG_HS_PERIPH_BASE);
// #endif

}
