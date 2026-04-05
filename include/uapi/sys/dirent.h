#pragma once

#include <uapi/types.h>

#define DIRENT_ALIGN(len) (((len) + 7) & ~7)

#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4  // 디렉토리
#define DT_BLK      6
#define DT_REG      8  // 일반 파일
#define DT_LNK      10
#define DT_SOCK     12

struct dirent {
    uint64_t d_ino;       // Inode 번호
    off_t    d_off;       // 다음 엔트리 오프셋
    uint16_t d_reclen;    // 구조체 길이
    uint8_t  d_type;      // 파일 타입
    char     d_name[256]; // 파일 이름
};