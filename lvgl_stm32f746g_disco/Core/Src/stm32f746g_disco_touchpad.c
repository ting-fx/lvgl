/**
 * @file indev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "stm32f746g_disco_565rgb_display.h"
#include "lvgl.h"

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_ts.h"


static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
static TS_StateTypeDef  TS_State;

void touchpad_init(void)
{
    BSP_TS_Init(STM32F746G_DISCO_SCREEN_WIDTH, STM32F746G_DISCO_SCREEN_HEIGHT);

    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    /* Read your touchpad */
    static int16_t last_x = 0;
    static int16_t last_y = 0;

    BSP_TS_GetState(&TS_State);
    if(TS_State.touchDetected) {
            data->point.x = TS_State.touchX[0];
            data->point.y = TS_State.touchY[0];
            last_x = data->point.x;
            last_y = data->point.y;
            data->state = LV_INDEV_STATE_PRESSED;
    } else {
            data->point.x = last_x;
            data->point.y = last_y;
            data->state = LV_INDEV_STATE_RELEASED;
    }
}
