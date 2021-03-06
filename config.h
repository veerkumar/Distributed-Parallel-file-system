
#ifndef _CONFIG_H
#define _CONFIG_H

#define PFS_BLOCK_SIZE 1 // 1 Kilobyte
#define STRIP_SIZE 4     // 4 blocks
#define NUM_FILE_SERVERS 2


#define MEGA 1024
struct pfs_stat {
  time_t pst_mtime; /* time of last data modification */
  time_t pst_ctime; /* time of creation */
  off_t pst_size;    /* File size in bytes */
};

#endif
