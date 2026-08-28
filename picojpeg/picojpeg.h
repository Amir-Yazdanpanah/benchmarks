#ifndef PICOJPEG_H
#define PICOJPEG_H

#include <stdint.h>

// Reduced to 128 to prevent 8-bit unsigned char overflow (256 -> 0) in callback
#define PJPG_MAX_IN_BUF_SIZE 128
#define PJPG_NO_MORE_BLOCKS 200

typedef enum {
    PJPG_SINGLE_STREAMING = 0,
    PJPG_DECODE_PROGRESSIVE
} pjpg_scan_type_t;

typedef struct {
    int m_width;
    int m_height;
    int m_comps;
    int m_MCUsPerRow;
    int m_MCUsPerCol;
    int m_scanType;
    int m_MCUWidth;
    int m_MCUHeight;
    unsigned char *m_pMCUBufR;
    unsigned char *m_pMCUBufG;
    unsigned char *m_pMCUBufB;
} pjpg_image_info_t;

typedef unsigned char (*pjpg_need_bytes_callback_func)(unsigned char *pBuf, unsigned char buf_size, unsigned char *pBytes_read, void *pCallback_data);

uint8_t pjpg_decode_init(pjpg_image_info_t *pInfo, pjpg_need_bytes_callback_func pNeed_bytes_callback, void *pCallback_data, unsigned char reduce);
uint8_t pjpg_decode_mcu(void);

#endif