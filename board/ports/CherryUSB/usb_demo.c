#include <rtthread.h>
#include <stdint.h>
#include <board.h>
#include <rtdevice.h>
#include <drv_gpio.h>
#include <usbh_core.h>
#include <dfs_romfs.h>
#include <dfs_fs.h>

#define USB_PWR_PIN GET_PIN(A, 15)

extern int cdc_acm_demo(void);
extern int cdc_acm_multi_demo(void);
extern int msc_ram_demo(void);

extern const struct romfs_dirent romfs_root;

void usb_demo_init(void)
{
#ifdef BSP_USING_USB_DEVICE
    // cdc_acm_demo();
    // cdc_acm_multi_demo();
    // msc_ram_demo();
#else
    rt_pin_mode(USB_PWR_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(USB_PWR_PIN, PIN_HIGH);

    dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root));
    usbh_initialize(0, USB_OTG_HS_PERIPH_BASE);
#endif

}
