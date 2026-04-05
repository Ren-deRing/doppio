#pragma once

#include <uapi/types.h>

#define S_IFMT   0170000  /* 파일 타입 비트 마스크 */

#define S_IFSOCK 0140000  /* 소켓 */
#define S_IFLNK  0120000  /* 심볼릭 링크 */
#define S_IFREG  0100000  /* 일반 파일 */
#define S_IFBLK  0060000  /* 블록 장치 */
#define S_IFDIR  0040000  /* 디렉토리 */
#define S_IFCHR  0020000  /* 문자 장치 */
#define S_IFIFO  0010000  /* FIFO */

#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)

struct stat {
    dev_t      st_dev;      /* 장치 ID */
    ino_t      st_ino;      /* 인덱스 노드 번호 */
    nlink_t    st_nlink;    /* 하드 링크 수 */
    mode_t     st_mode;     /* 파일 속성 */
    uid_t      st_uid;      /* 소유자 ID */
    gid_t      st_gid;      /* 그룹 ID */
    uint32_t   __pad0;      /* 패딩 */
    dev_t      st_rdev;     /* 특수 장치 ID */
    off_t      st_size;     /* 파일 크기 */
    blksize_t  st_blksize;  /* I/O 블록 크기 */
    blkcnt_t   st_blocks;   /* 블록 수 */

    struct timespec st_atim; /* 최종 접근 시간 */
    struct timespec st_mtim; /* 최종 수정 시간 */
    struct timespec st_ctim; /* 상태 변경 시간 */

    int64_t    __unused[3];
};

#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec