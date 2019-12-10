#include "commons.h"
#include "pfs.h"
#include "c_mdm.h"
#include "cs_mdm.h"

meta_data_manager_client *mdm_service;
thread thread_flusher;
thread thread_harvester;
thread thread_client_server;
string server_ip_port = "localhost:50051";



void
set_ipaddr_port() {
        struct ifaddrs *interfaces = NULL;
        struct ifaddrs *temp_addr = NULL;
        int success = 0;
        // retrieve the current interfaces - returns 0 on success
        success = getifaddrs(&interfaces);
        if (success == 0) {
                // Loop through linked list of interfaces
                temp_addr = interfaces;
                while(temp_addr != NULL) {
                        if(temp_addr->ifa_addr->sa_family == AF_INET) {
                                if(strcmp(temp_addr->ifa_name, INTERFACE)==0){
                                        ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
#ifdef DEBUG_FLAG                               
                                        cout<<"\n IPaddress"<<ipAddress;
#endif
                                }
                        }
                        temp_addr = temp_addr->ifa_next;
                }
        }
        // Free memory
        freeifaddrs(interfaces);
        /* initialize random seed: */
        srand (time(NULL));

        /* generate secret number between 5000 to 65000: */
        port = rand() % 60000 + 5000;
        client_server_ip_port.append(ipAddress);
        client_server_ip_port.append(":");
        client_server_ip_port.append(to_string(port));
}




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
			 //TODO write_file_to_server function implemention is not done yet	
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
register_client_and_start_client_server_process(){
	register_service_request_t *c_req = new register_service_request_t;
        register_service_response_t *c_response = NULL;

        c_req->type  = CLIENT;
        c_req->ip_port = client_server_ip_port;

        c_response = mdm_service->register_service_handler(c_req);
        cout<<"Response recieved";
	start_client_server(); // it will never return
	return;
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
	
	/* generate port and fetch ip */
	set_ipaddr_port();
	thread_client_server = std::thread(register_client_and_start_client_server_process);	

	return;
}
int pfs_create(const char *filename, int stripe_width) { 
	create_new_file(filename, stripe_width);
	 /* Send request to MM 
	  * 
	  * return mm lists and add into  the file list along with the permission 
	  *
	  * Allocate data in the cache 
	  * run harvester imidiatly 
	  *  */
	return 2;	
}

int pfs_open(const char *filename, const char mode){
	
		/*
		 * get server list 
		 *
		 * */
	
	return 1;}

size_t pfs_read(int filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit){return 1;}

size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){return 1;}

int pfs_close(int filedes){
	thread_harvester.join();
	thread_flusher.join();
	return 1;
}

int pfs_delete(const char *filename) {return 1;}

int pfs_fstat(int filedes, struct pfs_stat *buf) {
		/*Add last modifieed in the messageresposen. */
	return 1;} 
