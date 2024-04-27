#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DISPL_TOTAL_WIDTH   (320)
#define DISPL_TOTAL_HEIGHT  (240)

#define VIS_BARS_COUNT      21
#define VIS_ROWS_COUNT      19

#define VIS_BLOCK_WIDTH     11
#define VIS_BLOCK_HEIGHT    4

#define VIS_BAR_HGAP        2
#define VIS_BAR_VGAP        2

#define VIS_DISPLAY_WIDTH   (VIS_BAR_HGAP + VIS_BARS_COUNT * (VIS_BLOCK_WIDTH  + VIS_BAR_HGAP))
#define VIS_DISPLAY_HEIGHT  (VIS_BAR_VGAP + VIS_ROWS_COUNT * (VIS_BLOCK_HEIGHT + VIS_BAR_VGAP))

#define VIS_DISPLAY_BASE_WIDTH  (VIS_DISPLAY_WIDTH + 2 * 14)
#define VIS_DISPLAY_BASE_HEIGHT (VIS_BLOCK_HEIGHT * 3)

#if (VIS_DISPLAY_WIDTH > DISPL_TOTAL_WIDTH)
#error "asdads"
#endif

#if (VIS_DISPLAY_HEIGHT > DISPL_TOTAL_HEIGHT)
#error "asdads"
#endif

#define PANE_BOTTOM_HEIGHT      60

#define VIS_BASE_V_OFFSET            6
#define VIS_V_OFFSET                 4

#define VIS_BASE_X ((DISPL_TOTAL_WIDTH - VIS_DISPLAY_BASE_WIDTH) / 2)
#define VIS_BASE_Y (VIS_BASE_V_OFFSET + PANE_BOTTOM_HEIGHT)

#define VIS_X      ((DISPL_TOTAL_WIDTH - VIS_DISPLAY_WIDTH) / 2)
#define VIS_Y      (VIS_BASE_Y + VIS_DISPLAY_BASE_HEIGHT + VIS_V_OFFSET)

#define GREV(_val16) ((uint16_t)((_val16>>8) | (_val16<<8)))

/* Colors */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define GRAY        0X8430
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define BROWN       0XBC40
#define BRRED       0XFC07
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458

#define RAWCOLOR_BG                0x0000
#define RAWCOLOR_DISPL_BACK        0x1082
#define RAWCOLOR_DISPL_ACRYLIC     0x8C71
#define RAWCOLOR_BOT_PANE_BG       0x2945

#define VIS_PANE_BOTTOM_BG_COLOR    (0xFFFF - (GREV(RAWCOLOR_BOT_PANE_BG)))

#define VIS_DISPLAY_BG_COLOR        (0xFFFF - (GREV(RAWCOLOR_DISPL_BACK)))
#define VIS_DISPLAY_BASE_COLOR        (0xFFFF - (GREV(RAWCOLOR_DISPL_ACRYLIC)))

#define VIS_BLOCK_OFF_COLOR         (0xFFFF - (GREV(RED)))
#define VIS_BLOCK_ON_COLOR          (0xFFFF - (GREV(RAWCOLOR_DISPL_ACRYLIC)))

#ifdef __cplusplus
}
#endif