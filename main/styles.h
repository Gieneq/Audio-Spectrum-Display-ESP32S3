#pragma once

#define DISPL_TOTAL_WIDTH   (320)
#define DISPL_TOTAL_HEIGHT  (240 - 20)

#define VIS_BARS_COUNT      21
#define VIS_ROWS_COUNT      19

#define VIS_BLOCK_WIDTH     11
#define VIS_BLOCK_HEIGHT    3

#define VIS_BAR_HGAP        3
#define VIS_BAR_VGAP        3

#define VIS_DISPLAY_WIDTH   (VIS_BAR_HGAP + VIS_BARS_COUNT * (VIS_BLOCK_WIDTH  + VIS_BAR_HGAP))
#define VIS_DISPLAY_HEIGHT  (VIS_BAR_VGAP + VIS_ROWS_COUNT * (VIS_BLOCK_HEIGHT + VIS_BAR_VGAP))

#if (VIS_DISPLAY_WIDTH > DISPL_TOTAL_WIDTH)
#error "asdads"
#endif

#if (VIS_DISPLAY_HEIGHT > DISPL_TOTAL_HEIGHT)
#error "asdads"
#endif

#define GREV(_val16) ((uint16_t)((_val16>>8) | (_val16<<8)))

#define VIS_BLOCK_OFF_COLOR     GREV(0x4a49U)
#define VIS_DISPLAY_BG_COLOR    GREV(0x2104U)