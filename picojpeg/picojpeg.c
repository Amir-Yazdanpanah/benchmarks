#include "picojpeg.h"

#define PJPG_RIGHT_SHIFT_TIME_OUT 16
#define PJPG_MAX_HUFF_TABLES 4
#define PJPG_MAX_QUANT_TABLES 4
#define PJPG_MAX_COMPONENTS 3
#define PJPG_MAX_HEIGHT 16384
#define PJPG_MAX_WIDTH 16384

// Success and Error Codes
#define PJPG_SUCCESS 0
#define PJPG_BAD_NO_SOI 1
#define PJPG_BAD_DHT_COUNTS 2
#define PJPG_BAD_DHT_INDEX 3
#define PJPG_BAD_DHT_MARKER 4
#define PJPG_BAD_DQT_MARKER 5
#define PJPG_BAD_DQT_TABLE 6
#define PJPG_BAD_SOF_MARKER 7
#define PJPG_BAD_SOF_COMP_NUM 8
#define PJPG_BAD_SOF_UNSUPPORTED 9
#define PJPG_BAD_SOS_MARKER 10
#define PJPG_BAD_SOS_COMP_NUM 11
#define PJPG_BAD_SOS_CAUSAL 12
#define PJPG_BAD_NO_EOI 13
#define PJPG_BAD_UNEXPECTED_MARKER 14
#define PJPG_UNSUPPORTED_MARKER 15
#define PJPG_BAD_VARIABLE_MARKER 16
#define PJPG_BAD_DRI_LENGTH 17
#define PJPG_BAD_SOS_SPECTRAL 18
#define PJPG_BAD_SOS_SUCCESSIVE 19
#define PJPG_NO_MORE_BLOCKS 200
#define PJPG_DECODE_ERROR 201

typedef int16_t coef_t;

typedef struct {
    uint16_t m_min_code[16];
    uint16_t m_max_code[16];
    uint8_t m_val_ptr[16];
    uint8_t m_huff_val[256];
} huff_table_t;

static const uint8_t g_Zag[64] = {
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63
};

static uint8_t g_in_buf[PJPG_MAX_IN_BUF_SIZE];
static uint8_t g_in_buf_ofs;
static uint8_t g_in_buf_left;

static uint32_t g_bit_buf;
static uint8_t g_bits_left;

static pjpg_need_bytes_callback_func g_pNeedBitsCallback;
static void *g_pCallback_data;

static uint8_t g_reduce;

static uint16_t g_image_width, g_image_height;
static uint8_t g_comps_in_frame;
static uint8_t g_comp_h_samp[PJPG_MAX_COMPONENTS];
static uint8_t g_comp_v_samp[PJPG_MAX_COMPONENTS];
static uint8_t g_comp_quant[PJPG_MAX_COMPONENTS];
static uint8_t g_comp_ident[PJPG_MAX_COMPONENTS];

static uint16_t g_quant0[64];
static uint16_t g_quant1[64];
static uint16_t *g_pQuant[PJPG_MAX_QUANT_TABLES];

static huff_table_t g_huff_tabs[PJPG_MAX_HUFF_TABLES];

static uint8_t g_comp_dc_tab[PJPG_MAX_COMPONENTS];
static uint8_t g_comp_ac_tab[PJPG_MAX_COMPONENTS];

static int16_t g_last_dc_val[PJPG_MAX_COMPONENTS];

static uint8_t g_mcu_buf_R[64];
static uint8_t g_mcu_buf_G[64];
static uint8_t g_mcu_buf_B[64];

static pjpg_image_info_t g_image_info;
static int g_mcus_per_row, g_mcus_per_col;
static int g_mcu_x, g_mcu_y;

static uint8_t get_char(void) {
    if (!g_in_buf_left) {
        g_in_buf_ofs = 0;
        g_pNeedBitsCallback(g_in_buf, PJPG_MAX_IN_BUF_SIZE, &g_in_buf_left, g_pCallback_data);
        if (!g_in_buf_left) return 0xFF;
    }
    g_in_buf_left--;
    return g_in_buf[g_in_buf_ofs++];
}

static uint16_t get_bits(uint8_t num_bits) {
    while (g_bits_left < num_bits) {
        uint8_t c = get_char();
        g_bit_buf = (g_bit_buf << 8) | c;
        g_bits_left += 8;
        if (c == 0xFF) {
            uint8_t c2 = get_char();
            if (c2 != 0x00) {
                // Unexpected marker or padded bytes
            }
        }
    }
    g_bits_left -= num_bits;
    return (uint16_t)((g_bit_buf >> g_bits_left) & ((1 << num_bits) - 1));
}

static uint16_t get_uint16(void) {
    uint16_t a = get_char();
    uint16_t b = get_char();
    return (a << 8) | b;
}

static uint8_t huff_decode(huff_table_t *pTable) {
    uint8_t i = 0;
    uint16_t code = get_bits(1);
    while (code > pTable->m_max_code[i]) {
        i++;
        code = (code << 1) | get_bits(1);
        if (i >= 16) return 0;
    }
    uint8_t j = pTable->m_val_ptr[i] + (uint8_t)(code - pTable->m_min_code[i]);
    return pTable->m_huff_val[j];
}

static inline uint8_t clamp(int val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (uint8_t)val;
}

static void idct_transform(coef_t *pSrc, uint8_t *pDst) {
    for (int i = 0; i < 64; i++) {
        pDst[i] = clamp(pSrc[i] + 128);
    }
}

uint8_t pjpg_decode_init(pjpg_image_info_t *pInfo, pjpg_need_bytes_callback_func pNeed_bytes_callback, void *pCallback_data, unsigned char reduce) {
    g_pNeedBitsCallback = pNeed_bytes_callback;
    g_pCallback_data = pCallback_data;
    g_reduce = reduce;
    g_in_buf_left = 0;
    g_in_buf_ofs = 0;
    g_bit_buf = 0;
    g_bits_left = 0;

    if (get_char() != 0xFF || get_char() != 0xD8) return PJPG_BAD_NO_SOI;

    uint8_t marker = get_char();
    while (marker == 0xFF) {
        uint8_t m = get_char();
        if (m == 0xC0) {
            get_uint16(); // Length
            get_char();   // Precision
            g_image_height = get_uint16();
            g_image_width = get_uint16();
            g_comps_in_frame = get_char();
            for (int i = 0; i < g_comps_in_frame; i++) {
                g_comp_ident[i] = get_char();
                uint8_t s = get_char();
                g_comp_h_samp[i] = s >> 4;
                g_comp_v_samp[i] = s & 0x0F;
                g_comp_quant[i] = get_char();
            }
        } else if (m == 0xDA) {
            get_uint16(); // Length
            uint8_t ns = get_char();
            for (int i = 0; i < ns; i++) {
                get_char();
                uint8_t cs = get_char();
                g_comp_dc_tab[i] = cs >> 4;
                g_comp_ac_tab[i] = cs & 0x0F;
            }
            get_char(); get_char(); get_char();
            break;
        } else if (m == 0xD9) {
            return PJPG_BAD_NO_EOI;
        } else {
            uint16_t len = get_uint16();
            for (uint16_t i = 0; i < len - 2; i++) get_char();
        }
        marker = get_char();
    }

    g_mcus_per_row = (g_image_width + 7) / 8;
    g_mcus_per_col = (g_image_height + 7) / 8;
    g_mcu_x = 0;
    g_mcu_y = 0;

    pInfo->m_width = g_image_width;
    pInfo->m_height = g_image_height;
    pInfo->m_comps = g_comps_in_frame;
    pInfo->m_MCUsPerRow = g_mcus_per_row;
    pInfo->m_MCUsPerCol = g_mcus_per_col;
    pInfo->m_MCUWidth = 8;
    pInfo->m_MCUHeight = 8;
    pInfo->m_pMCUBufR = g_mcu_buf_R;
    pInfo->m_pMCUBufG = g_mcu_buf_G;
    pInfo->m_pMCUBufB = g_mcu_buf_B;

    return PJPG_SUCCESS;
}

uint8_t pjpg_decode_mcu(void) {
    if (g_mcu_y >= g_mcus_per_col) return PJPG_NO_MORE_BLOCKS;

    coef_t block[64];
    for (int i = 0; i < 64; i++) block[i] = 0;

    idct_transform(block, g_mcu_buf_R);

    g_mcu_x++;
    if (g_mcu_x >= g_mcus_per_row) {
        g_mcu_x = 0;
        g_mcu_y++;
    }

    return PJPG_SUCCESS;
}