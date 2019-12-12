#include "commons.h"
#include "pfs.h"
#include "c_mdm.h"
#include "cs_mdm.h"
#include "file_server_client.h"

meta_data_manager_client *mdm_service;
map<string,file_info_store*> file_dir;

map<uint32_t, string> fdis_to_filename_map;

map<string,pair<int,int>> token_map;



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

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":  client server ip assigned : "<<client_server_ip_port;
#endif
}




/* TODO: Need to make thread safe 
	 BLOCK_SIZE here is not correct, need to use PFS_BLOCK_SIZE to find correct server address, now assumming both are equal
*/
void harvest_block(cache_block *cb) {
	pair<int, int> range;
	int temp_start = 0, temp_end = 0, server_index = 0 ;
	if(cb->dirty == true) {
		//No need to syncronize this as we are reading and file recepe wont change
		vector<string> file_recep = file_dir[cb->file_name]->server_list;
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
			       
				fs_service->fs_write_file_to_server(cb,
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
#ifdef DEBUG_FLAG
		cout<<"\n"<<__func__<<" Harvester process is running";
#endif
	pair<int,int> range;
	int server_index = 0;
	int temp_start = 0, temp_end = 0;
	/* TODO: sanity check
		 Since this loop is latency expensive we avoinding to lock it, 
		we will remove element later*/
#ifdef DEBUG_FLAG
		cout<<"\n"<<__func__<<" Dirty list size is = "<< c_m->dirty_list.size();
#endif

	for(auto it = c_m->dirty_list.begin(); it != c_m->dirty_list.end(); it++) {

#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Dirty Cache: start= "<<(*it)->start_index <<" end= "<<(*it)->end_index <<" filename= "<<(*it)->file_name <<" dirty= "<<(*it)->dirty;
	cout<<"\n";
#endif
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
		cout<<"\n Harvester process created";
#endif
	while(1) {
		std::this_thread::sleep_for(std::chrono::seconds(5));
#ifdef DEBUG_FLAG
		cout<<"\n Harvester_process: harvester process Timeout";
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
void flusher() {
#ifdef DEBUG_FLAG
	cout<<"\n\nfluser: starting executing";
#endif
	int filled_per = 0; 
	int reduce_to_sz = 0;	
	int current_sz = 0;
	current_sz = c_m->allocated_list.size();
	filled_per = (current_sz/(ROW*COLUMN))*100;
	cache_block *cb = NULL;
	if(filled_per >= FLUSH_HIGH_MARK ) { 
		reduce_to_sz = (FLUSH_LOW_MARK/100)*(ROW*COLUMN);

#ifdef DEBUG_FLAG
		cout<<"\n"<<__func__ <<": Reaching cache from: "<<current_sz<<" to: "<<reduce_to_sz;
#endif

		while( (c_m->allocated_list.size() - reduce_to_sz) <= 0) {
			cb = c_m->obj_cache->lru_list.back();	

			if(cb->dirty == false) {
#ifdef DEBUG_FLAG_VERBOSE
				cout<<"\n fluser: Non-dirty filename:"<<cb->file_name;
#endif
				c_m->add_to_back_free_list_l (cb);
				c_m->rm_from_allocated_list_l(cb);
			} else {
#ifdef DEBUG_FLAG_VERBOSE
				cout<<"\n fluser: Dirty block filename:"<<cb->file_name;
#endif
				harvest_block(cb);
				/* Remove from the dirty list */
				c_m->add_to_back_free_list_l (cb);
				c_m->rm_from_allocated_list_l (cb);
			}
			c_m->obj_cache->lru_item_delete(cb);
		}
	}
}

void flusher_process(){
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": process created";
#endif
	while(1) {
		std::this_thread::sleep_for(std::chrono::seconds(FLUSHER_TIMEOUT));
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": flusher process Timeout ";
#endif
		flusher();
	}
}

void 
register_client_and_start_client_server_process(){

#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": starting the client server";
#endif
	start_client_server(); // it will never return
	return;
}

void
initialize (int argc, char *argv[]) {
#ifdef DEBUG_FLAG
                 cout<<"\n \n"<<__func__ <<": initilaizating";
#endif
	set_ipaddr_port();
	/* Intialize cache */
	c_m = new cache_manager;


	/* Create harvester thread, which will read from 
	   dirty queue and write to file server */
	thread_harvester = std::thread(harvester_process);

	/* Create flusher thread, If cache is FLUSH_PERCENT% filled */
	thread_flusher = std::thread(flusher_process);
	mdm_service = new meta_data_manager_client (grpc::CreateChannel(server_ip_port, grpc::InsecureChannelCredentials()));
	
	/* generate port and fetch ip */
	thread_client_server = std::thread(register_client_and_start_client_server_process);	
	
	std::this_thread::sleep_for(std::chrono::seconds(1));

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": register the clinet and start the server";
#endif
	register_service_request_t *c_req = new register_service_request_t;
        register_service_response_t *c_response = NULL;

        c_req->type  = CLIENT;
        c_req->ip_port = client_server_ip_port;

        c_response = mdm_service->register_service_handler(c_req);

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": After client registeration ";
#endif

	return;
}

int pfs_create(const char *filename, int stripe_width) { 

	/* We just send the request and 
	 * MM will create a new file and 
	 * assign server list according to width */
#ifdef DEBUG_FLAG
                 cout<<"\n \n"<<__func__ <<": Sending file create request ";
#endif
	return mm_create_new_file(filename, stripe_width);
}

int pfs_open(const char *filename, const char mode){
	
		/*
		 * get server list 
		 *
		 * */
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Sending file open request ";
#endif
	return mm_open_file(filename, mode);
}

size_t pfs_read(uint32_t filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit){

	int start = 0, end = 0;
	
	bool need_permission = true;

	/*check if file was opened with read/write */
	file_info_store *file = file_dir[fdis_to_filename_map[filedes]];

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": checking file permission";
#endif	
	/*Iterate over all the permission and check if we already have the permission*/
  	for(auto it = file->access_permission.begin(); it!=file->access_permission.end();it++) {
		pair<int,int> s_e = (*it).start_end;
		if(s_e.first <= offset && s_e.second >= offset+nbyte) {
			need_permission = false;
			break;
		}
	}	
	if(need_permission) {
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Not sufficient permission, sending permission aquire request";
#endif
		if(mm_get_read_permission( filedes, nbyte, offset)<-1) {
			cout<<"\n Error while getting the permission";
			return 0;
		}
	}
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Permission accquired";
                 cout<<"\n"<<__func__ <<": Processing read on fildes: "<<filedes;
#endif

	/* read file from then cache using cache_manager */

      return c_m->read_file(fdis_to_filename_map[filedes], (char *)buf, offset, offset+nbyte-1, cache_hit);
}

size_t pfs_write(uint32_t filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){
	
         int start = 0, end = 0;
 
         bool need_permission = true;
 
         /*check if file was opened with read/write */
         file_info_store *file = file_dir[fdis_to_filename_map[filedes]];
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": "<<filedes << " : " << file->global_permission;
#endif


	 if(file->global_permission.find("w") == string::npos ) {

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": File is not open for writing";
#endif
	 	 *cache_hit = 0;
		 return -1;
	 }
 

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Checking permissions";
#endif
         /*Iterate over all the permission and check if we already have the permission*/
         for(auto it = file->access_permission.begin(); it!=file->access_permission.end();it++) {
                 pair<int,int> s_e = (*it).start_end;
                 if(s_e.first <= offset && s_e.second >= offset+nbyte) {
                         need_permission = false;
			 *cache_hit = 1;
                         break;
                 }
 		
 
         }
         if(need_permission) {
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Requesting permissions from MM";
#endif
		 *cache_hit = 0;
                 if(mm_get_write_permission(filedes, nbyte, offset)<-1) {
                         cout<<"\n Error while getting the permission";
                         return 0;
 
                 }
         }

#ifdef DEBUG_FLAG
                cout<<"\n"<<__func__ <<": Permission request accepted";
		cout<<"\n"<<__func__ <<": Permission request accepted";
#endif

	
 
         /*at this point we need to write in the cache and harvester will take */	
	
	 c_m->write_file(fdis_to_filename_map[filedes], buf, offset, offset+nbyte-1, cache_hit);
	 file_dir[fdis_to_filename_map[filedes]]->status == WRITTEN;
	
	return 1;
}

int pfs_close(uint32_t filedes){

	/* clean the cache for this file */
	c_m->clean_file(fdis_to_filename_map[filedes], "close");

	/* remove permission and remove the file from file directory*/

	file_info_store *file = file_dir[fdis_to_filename_map[filedes]];
	file->access_permission.clear();
	
	delete file;
	file_dir.erase(fdis_to_filename_map[filedes]);
	fdis_to_filename_map.erase(filedes);
	return 1;
}

int pfs_delete(const char *filename) {
	/* send file delete request to metadata manager */
	 mm_delete_file(filename);
	/* clean the cache for this file */
	c_m->clean_file(filename, "delete");

	file_info_store *file = file_dir[filename];
	file->access_permission.clear();

	delete file;
	file_dir.erase(filename);
	return 1;
}

int pfs_fstat(uint32_t filedes, struct pfs_stat *buf) {
		/*Add last modifieed in the messageresposen. */
	mm_get_fstat(fdis_to_filename_map[filedes], buf);	
	return 1;

} 
