#ifndef PFS_H
#define PFS_H
#include "config.h"
#define HARVEST_TIME 30    // In second
/* Make room for new cache block */
#define FLUSH_HIGH_MARK 90   // kick flusher if is more then high mark %
#define FLUSH_LOW_MARK  50   // Keep doing it till it reach low mark %
#define FLUSHER_TIMEOUT 30   // x Seconds
using namespace std;

extern thread thread_flusher;
extern thread thread_harvester;
extern thread thread_client_server;
class meta_data_manager_client; 
extern meta_data_manager_client *mdm_service;
extern string server_ip_port;

extern map<string,file_info_store*> file_dir;

extern map<uint32_t, string> fdis_to_filename_map;

extern map<string,pair<int,int>> token_map;

void initialize(int argc, char *argv[]);
int pfs_create(const char *filename, int stripe_width);

int pfs_open(const char *filename, const char mode);

size_t pfs_read(uint32_t filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit);

size_t pfs_write(uint32_t filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit);

int pfs_close(uint32_t filedes);

int pfs_delete(const char *filename);

int pfs_fstat(uint32_t filedes, struct pfs_stat *buf); // Check the config file for the definition of pfs_stat structure

#endif
