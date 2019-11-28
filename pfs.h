#include "config.h"
//#include "cache_manager.h"

#define HARVEST_TIME 30    // In second
/* Make room for new cache block */
#define FLUSH_HIGH_MARK 90   // kick flusher if is more then high mark %
#define FLUSH_LOW_MARK  50   // Keep doing it till it reach low mark %
#define FLUSHER_TIMEOUT 30   // x Seconds
using namespace std;

extern thread thread_flusher;
extern thread thread_harvester;
class meta_data_manager_client; 
extern meta_data_manager_client *mdm_service;
extern string server_ip_port;

void initialize(int argc, char *argv[]);
int pfs_create(const char *filename, int stripe_width);

int pfs_open(const char *filename, const char mode);

size_t pfs_read(int filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit);

size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit);

int pfs_close(int filedes);

int pfs_delete(const char *filename);

int pfs_fstat(int filedes, struct pfs_stat *buf); // Check the config file for the definition of pfs_stat structure
