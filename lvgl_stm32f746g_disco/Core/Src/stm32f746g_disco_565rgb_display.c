
#include "stm32f746g_disco_565rgb_display.h"
#include "lv_conf.h"
#include "lvgl.h"
#include "stm32746g_discovery_lcd.h"
#include <string.h>

static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void DMA2D_Config();
static void DMA2D_Init(uint32_t Mode, uint32_t OutputMemoryAdd, uint32_t OutputOffset, uint32_t PixelPerLine, uint32_t NumberOfLine);
static void DMA2D_Run();
static void DMA2D_TransferComplete(DMA2D_HandleTypeDef *han);
static void DMA2D_TransferError(DMA2D_HandleTypeDef *han);

DMA2D_HandleTypeDef hdma2d;
static lv_display_t *display;

static uint16_t buf1[STM32F746G_DISCO_SCREEN_WIDTH * 68];
static uint16_t buf2[STM32F746G_DISCO_SCREEN_WIDTH * 68];
__attribute__((section(".SD_RAM"))) static uint16_t frame_buffer[STM32F746G_DISCO_SCREEN_WIDTH][STM32F746G_DISCO_SCREEN_HEIGHT];


void display_init(void)
{
    /* Hardware Initialization */
    BSP_LCD_Init();

    BSP_LCD_LayerDefaultInit(0, (uint32_t)&frame_buffer);
    BSP_LCD_SelectLayer(0);

    //LCD_SetPixelFormat(DMA2D_RGB565);
    LTDC_Layer1 -> CFBLR = LTDC_Layer1 -> CFBLR / 2;
    LTDC_Layer1 -> PFCR = DMA2D_RGB565;
    LTDC->SRCR = LTDC_SRCR_IMR;

    // clear the frame buffer
    memset(frame_buffer, 0, STM32F746G_DISCO_SCREEN_WIDTH * STM32F746G_DISCO_SCREEN_HEIGHT * 2);

    // configure default DMA params
    DMA2D_Config();

    BSP_LCD_DisplayOn();

    display = lv_display_create(STM32F746G_DISCO_SCREEN_WIDTH, STM32F746G_DISCO_SCREEN_HEIGHT);
    lv_display_set_buffers(display, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_cb);
}

static void flush_cb(lv_display_t * disp, const lv_area_t *area, uint8_t * px_map)
{
	lv_area_t copy = *area;

	if(copy.x1 < 0) {
		copy.x1 = 0;
	}

	if(copy.x2 > STM32F746G_DISCO_SCREEN_WIDTH - 1) {
		copy.x2 = STM32F746G_DISCO_SCREEN_WIDTH - 1;
	}

	if(copy.y1 < 0) {
		copy.y1 = 0;
	}

	if(copy.y2 > STM32F746G_DISCO_SCREEN_HEIGHT - 1) {
		copy.y2 = STM32F746G_DISCO_SCREEN_HEIGHT - 1;
	}

	if(copy.x1 > copy.x2 || copy.y1 > copy.y2) {
		return;
	}

    uint32_t width = (copy.x2 - copy.x1 + 1);
    uint32_t height = (copy.y2 - copy.y1 + 1);
    uint16_t *read = (uint16_t *)px_map;
    uint16_t *write = (uint16_t *)frame_buffer;
    write += copy.y1 * STM32F746G_DISCO_SCREEN_WIDTH;
    write += copy.x1;

    read += (copy.y1 - area->y1) * STM32F746G_DISCO_SCREEN_WIDTH;
    read += (copy.x1 - area->x1);

    DMA2D_Init(DMA2D_M2M, (uint32_t)write, STM32F746G_DISCO_SCREEN_WIDTH - width, width, height);

    DMA2D->FGPFCCR = (0xff000000) | DMA2D_RGB565;  // foreground format conversion
    DMA2D->FGMAR = (uint32_t)read;            // foreground memory address
    DMA2D->FGOR = (area->x2 - area->x1 - copy.x2 + copy.x1);  // foreground offset
    DMA2D_Run();
}

static void DMA2D_Config()
{
    /* Configure DMA2D */
	hdma2d.Instance = DMA2D;
	//HAL_DMA2D_DeInit(&hdma2d);
    hdma2d.Init.ColorMode = DMA2D_RGB565;

    /* Configures the color mode of the output image */
    DMA2D->OPFCCR &= ~(uint32_t)DMA2D_OPFCCR_CM;
    DMA2D->OPFCCR |= DMA2D_RGB565;

    HAL_DMA2D_MspInit(&hdma2d);
    hdma2d.XferCpltCallback = DMA2D_TransferComplete;
    hdma2d.XferErrorCallback = DMA2D_TransferError;
}

static void DMA2D_Init(uint32_t Mode, uint32_t OutputMemoryAdd, uint32_t OutputOffset, uint32_t PixelPerLine, uint32_t NumberOfLine)
{
	uint32_t pixline  = 0;

	hdma2d.Instance = DMA2D;

    /* Configures the DMA2D operation mode */
    DMA2D->CR &= (uint32_t)DMA2D_CR_MODE_Msk;
    DMA2D->CR |= Mode | DMA2D_IT_TC|DMA2D_IT_TE;

    /* Configures the output memory address */
    DMA2D->OMAR = OutputMemoryAdd;

    /* Configure  the line Offset */
    DMA2D->OOR &= ~(uint32_t)DMA2D_OOR_LO;
    DMA2D->OOR |= (OutputOffset);

    /* Configure the number of line and pixel per line */
    pixline = PixelPerLine << 16;
    DMA2D->NLR &= ~(DMA2D_NLR_NL | DMA2D_NLR_PL);
    DMA2D->NLR |= ((NumberOfLine) | (pixline));
}

static void DMA2D_Run()
{
    SCB_CleanInvalidateDCache();

    /* tell the engine to start */
    DMA2D->CR |= (uint32_t)DMA2D_CR_START;
}

static void DMA2D_TransferComplete(DMA2D_HandleTypeDef *han)
{
    lv_disp_flush_ready(display);
}

static void DMA2D_TransferError(DMA2D_HandleTypeDef *han)
{

}
