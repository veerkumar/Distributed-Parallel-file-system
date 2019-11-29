#include "commons.h"
#include "pfs.h"
#include "c_mdm.h"

meta_data_manager_client *mdm_service;
thread thread_flusher;
thread thread_harvester;
string server_ip_port = "localhost:50051";

/* TODO: Need to make thread safe 
	 BLOCK_SIZE here is not correct, need to use PFS_BLOCK_SIZE to find correct server address, now assumming both are equal
*/
void harvest_block(cache_block *cb) {
	pair<int, int> range;
	int temp_start = 0, temp_end = 0, server_index = 0 ;
	if(cb->dirty == true) {
		//No need to syncronize this as we are reading and file recepe wont change
		vector<string> file_recep = c_m->map_file_recep[cb->file_name];
		/*TODO: one case to handle, while doing this if another thread append a element */                 
		for (auto dr_it = cb->dirty_range.begin();
				dr_it != cb->dirty_range.end();
				dr_it++) {
			range = *dr_it;
			temp_start = range.first;
			while(1) {
				server_index = temp_start/BLOCK_SIZE;
				temp_end = (range.second >=
						((server_index+1)*BLOCK_SIZE))?((server_index+1)*BLOCK_SIZE - 1):range.second;       
				write_file_to_server(cb,
						temp_start,
						temp_end,
						file_recep[server_index]);
				if(range.second > temp_end) {
					/*it means we need to write to more then one fileserver*/                                          
					temp_start = temp_end + 1;
				} else {
					break;
				}
			}

		}
		cb->dirty = false;
	}

}

void harvester(){
	pair<int,int> range;
	int server_index = 0;
	int temp_start = 0, temp_end = 0;
	/* TODO: sanity check
		 Since this loop is latency expensive we avoinding to lock it, 
		we will remove element later*/
	for(auto it = c_m->dirty_list.begin(); it != c_m->dirty_list.end(); it++) {
		cache_block *cb = *it;
		harvest_block(cb);
	}
       
	/* Lock the queue and remove non-dirty block */ 
	c_m->mutx_dirty_list.lock();
	for(auto it = c_m->dirty_list.begin(); it != c_m->dirty_list.end(); ) {
		cache_block *cb = *it;
		if(cb->dirty == false) {
			c_m->dirty_list.erase(it); 
		}
	}	
	c_m->mutx_dirty_list.unlock();

}
void harvester_process() {
#ifdef DEBUG_FLAG
		cout<<"Harvester process created";
#endif
	while(1) {
		std::this_thread::sleep_for(std::chrono::seconds(30));
#ifdef DEBUG_FLAG
		cout<<"Harvester_process: flusher process Timeout";
#endif
		harvester();
	}
}
/* Create flusher thread, If cache is FLUSH_PERCENT% filled
	   Start removing least recently used cache block 
	   Condition: 
		while removing 
			if Dirty: harvest it and remove it from dirty queue and 
				  remove from allocated queue and add in free list. 
			if not dirty: Just empty "data" and append in the free list
*/
/* TODO: Currently fetched from back of allocated list, will need to fetch from prority queue 
*/
void flusher() {
#ifdef DEBUG_FLAG
	cout<<"fluser: starting executing";
#endif
	int filled_per = 0; 
	int reduce_to_sz = 0;	
	int current_sz = 0;
	current_sz = c_m->allocated_list.size();
	filled_per = (current_sz/(ROW*COLUMN))*100;
	cache_block *cb = NULL;
	if(filled_per >= FLUSH_HIGH_MARK ) { 
#ifdef DEBUG_FLAG
		cout<<"flusher: Reached high water mark";
#endif
		reduce_to_sz = (FLUSH_LOW_MARK/100)*(ROW*COLUMN);	
		
		for (auto it = c_m->allocated_list.rbegin();
			 it != c_m->allocated_list.rend();
			 ++it) {
			cb = *it;
			if(cb->dirty == false) {
#ifdef DEBUG_FLAG_VERBOSE
				cout<<"fluser: Non-dirty filename:"<<cb->file_name;
#endif
				cb->clean_cache_block();
				c_m->add_to_back_free_list_l (cb);
				c_m->rm_from_allocated_list_l(it);
			} else {
#ifdef DEBUG_FLAG_VERBOSE
				cout<<"fluser: Dirty block filename:"<<cb->file_name;
#endif
				harvest_block(cb);
				/* Remove from the dirty list */
				cb->clean_cache_block();
				c_m->add_to_back_free_list_l (cb);
				c_m->rm_from_allocated_list_l (it);
			}
		}
	}
}

void flusher_process(){
#ifdef DEBUG_FLAG
		cout<<"Fluser process created";
#endif
	while(1) {
		std::this_thread::sleep_for(std::chrono::seconds(FLUSHER_TIMEOUT));
#ifdef DEBUG_FLAG
		cout<<"flusher_process: flusher process Timeout";
#endif
		flusher();
	}
}
void
initialize (int argc, char *argv[]) {

	/* Intialize cache */
	c_m = new cache_manager();


	/* Create harvester thread, which will read from 
	   dirty queue and write to file server */
	thread_harvester = std::thread(harvester_process);

	/* Create flusher thread, If cache is FLUSH_PERCENT% filled */
	thread_flusher = std::thread(flusher_process);
	mdm_service = new meta_data_manager_client (grpc::CreateChannel(server_ip_port, grpc::InsecureChannelCredentials()));
	

}
int pfs_create(const char *filename, int stripe_width) { 
	create_new_file(filename, stripe_width);
	return 2;	
}

int pfs_open(const char *filename, const char mode){return 1;}

size_t pfs_read(int filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit){return 1;}

size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){return 1;}

int pfs_close(int filedes){
	thread_harvester.join();
	thread_flusher.join();
	return 1;
}

int pfs_delete(const char *filename) {return 1;}

int pfs_fstat(int filedes, struct pfs_stat *buf) {return 1;} 

