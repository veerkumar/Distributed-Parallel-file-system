#include "commons.h"
#include "cache_manager.h"
#include "config.h"
#include "file_server_client.h"
using namespace std;

cache_manager *c_m;

cache_block:: cache_block() {
	mutex cb_lock;
	dirty = false;
	file_name = "";
	start_index = 0;
	end_index = 0;
	data = new char[CACHE_BLOCK_SZ];
}
cache_block::~cache_block(){
	if(data != NULL) {
		delete data;
		dirty_range.clear();
	}
}

bool cache_block::clean_cache_block(cache_block *cb){
	if (cb->dirty == false) {
		cb->file_name.erase();
		cb->start_index = 0;
		cb->end_index = 0;
	} else {
#ifdef DEBUG_FLAG
		cout<<"This block is dirty name: " << file_name;
#endif
		return false;
	}
	return true;
}


cache::cache(){
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Cache constructor is called";
#endif
	slots = new cache_block*[ROW];
	for(int i =0; i<ROW;i++) {
		slots[i] = new cache_block[COLUMN];
	}     
}

cache_manager::cache_manager() {
	//if (obj_cache == NULL) {
		obj_cache =  new cache;
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Cache Manager constructor is called";
#endif
		/* add newly allocated in the free list */
		for(int i=0; i<ROW;i++) {
			for(int j=0; j<COLUMN;j++) {
				free_list.push_back(&obj_cache->slots[i][j]);
			}
		}
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Free list size= "<<free_list.size();
#endif
	//}

/*#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Object is not NULL";
#endif*/
} 

cache* cache_manager:: get_cache_obj () {
	obj_cache =  new cache;
	return obj_cache;
}

bool cache_manager::add_to_back_free_list_l (cache_block *cb) {
	mutx_free_list.lock();
	free_list.push_back(cb);
	mutx_free_list.unlock();
	return true;
}
bool  cache_manager::add_to_front_free_list_l (cache_block *cb) {
	mutx_free_list.lock();
	free_list.insert(free_list.begin(),cb);
	mutx_free_list.unlock();
	return true;
}
bool cache_manager::rm_from_free_list_l (cache_block *cb) {
	return true;
}
/*As such there is not reverser_iterator defination for "erase"*/
bool cache_manager::rm_from_free_list_l ( vector<cache_block*>::reverse_iterator& it) {
	mutx_free_list.lock();
//	free_list.erase(it);
	mutx_free_list.unlock();
	return true;
}
bool cache_manager::rm_from_free_list_l ( vector<cache_block*>::iterator& it) {
	mutx_free_list.lock();
	free_list.erase(it);
	mutx_free_list.unlock();
	return true;
}

cache_block* cache_manager::get_free_cache_block () {
		cache_block *cb;
		mutx_free_list.lock();
		auto it = free_list.begin();
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Freelist size = "<< free_list.size();
	cout<<"\n";
#endif
		cb = *it;
		free_list.erase(it);
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Freelist size = "<< free_list.size();
#endif

		mutx_free_list.unlock();
	return cb;
}
bool cache_manager::add_to_dirty_list_l (cache_block *cb) {
	mutx_dirty_list.lock();
	dirty_list.push_back(cb);
	mutx_dirty_list.unlock();
	return true;
}

bool cache_manager::rm_from_dirty_list_l (cache_block *cb) {
	return true;
}

bool cache_manager::add_to_allocated_list_l (cache_block *cb) {

	return true;
}

bool cache_manager::add_to_front_allocated_list_l (cache_block *cb) {
	mutx_allocated_list.lock();
	allocated_list.insert(allocated_list.begin(),cb);
	add_to_map_fname_chunks_l(cb->file_name,cb);
	mutx_allocated_list.unlock();
	return true;
}

bool cache_manager::rm_from_allocated_list_l (cache_block *cb) {
	mutx_allocated_list.lock();
	auto it = find(allocated_list.begin(), allocated_list.end(),cb);
	allocated_list.erase(it);
	(*it)->clean_cache_block(cb);
	rm_from_map_fname_chunks_l(cb->file_name, cb);
	mutx_allocated_list.unlock();

	return true;
}

bool cache_manager::rm_from_allocated_list_l ( vector<cache_block*>::reverse_iterator& it) {
	mutx_allocated_list.lock();
//	allocated_list.erase(it);
	mutx_allocated_list.unlock();
	return true;
}
bool cache_manager::rm_from_allocated_list_l ( vector<cache_block*>::iterator& it) {
	cache_block *cb = *it;
	mutx_allocated_list.lock();
	allocated_list.erase(it);
	(*it)->clean_cache_block(cb);
	rm_from_map_fname_chunks_l(cb->file_name, cb);
	mutx_allocated_list.unlock();
	return true;
}

bool cache_manager::add_to_map_fname_chunks_l (string file_name, cache_block *cb) {
	mutx_map_fname_to_chunks.lock();
	map_fname_to_chunks[file_name].push_back(cb);
	mutx_map_fname_to_chunks.unlock();

	return true;
}

bool cache_manager::rm_from_map_fname_chunks_l (string file_name, cache_block* cb) {
	mutx_map_fname_to_chunks.lock();
	vector<cache_block*> cb_list = map_fname_to_chunks[file_name];
	cout<<"cache block to before size "<<cb_list.size();
	for(auto it =  cb_list.begin(); it!= cb_list.end(); it++) {
		if(*it == cb) {
			cb_list.erase(it);
		}
	}
	cout<<"cache block removed  current size "<<cb_list.size();
	mutx_map_fname_to_chunks.unlock();
	return true;	
}
int sort_by_start_file_chunk_vector(const pair<pair<int,int>,char*> &a, const pair<pair<int,int>,char*> &b){
	return (a.first.first < b.first.first) ;

}

int sort_vector_cache_block (const cache_block* a, const cache_block* b){
	return (a->start_index < b->start_index);
}


int sort_pair_one_first_element_comp (const pair<int,int> &a, const pair<int,int> &b) {
	return (a.first < b.first);

}
int collapse_dirty_list(cache_block *cb) {
	vector<pair<int,int>> vec = cb->dirty_range;
	sort(vec.begin(),vec.end(),sort_pair_one_first_element_comp);
	stack<pair<int,int>> s;

	// push the first interval to stack
	s.push(vec[0]);

	// Start from the next interval and merge if necessary
	for (int i = 1 ; i < vec.size(); i++)
	{
		// get interval from stack top
		pair<int, int> top = s.top();

		// if current interval is not overlapping with stack top,
		// push it to the stack
		if (top.second < vec[i].first)
			s.push(vec[i]);

		// Otherwise update the ending time of top if ending of current
		// interval is more
		else if (top.second < vec[i].second)
		{
			top.second = vec[i].second;
			s.pop();
			s.push(top);
		}
	}

	// Print contents of stack
	cout << "\n The Merged Intervals are: ";
	vec.erase(vec.begin(),vec.end());
	while (!s.empty())
	{
		pair<int,int> t = s.top();
#ifdef DEBUG_FLAG
		cout << "[" << t.first << "," << t.second << "] ";
#endif
		vec.insert(vec.begin(), make_pair(t.first, t.second));
		s.pop();
	}
}


void cache::refer(cache_block* cb) {

	if(lru_indexing.find(cb) != lru_indexing.end()) {

		lru_list.erase(lru_indexing[cb]);

		lru_list.push_front(cb);
		lru_indexing[cb] = lru_list.begin();
	} else {
		lru_list.push_front(cb);
		lru_indexing[cb] = lru_list.begin();

	}


}

void cache::lru_item_delete(cache_block* cb){

	if(lru_indexing.find(cb) != lru_indexing.end())  { 
			mutex_lru_indexing.lock();
			mutex_lru_list.lock();	
			lru_list.erase(lru_indexing[cb]);
			lru_indexing.erase(cb);
			mutex_lru_list.unlock();	
			mutex_lru_indexing.unlock();	
	}
}

void cache::display_lru_list(){
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":";
#endif
		for(auto it = lru_list.begin(); it!= lru_list.end();it++){
			cout<<"\n           file_name      : "<< (*it)->file_name;
			cout<<"\n           start_name     : "<< (*it)->start_index;
			cout<<"\n           end_index      : "<< (*it)->end_index;
			cout<<"\n           Dirty          : "<< (*it)->dirty;
			cout<<"\n           dirty list size: "<< (*it)->file_name;
			cout<<"\n";
		}
}

int cache_manager::read_file (string file_name, char *buf, int start,int end, int *cache_hit) {
	/*START and END  includes boundries*/
	bool all_chunks_available = false;
	int current_index = 0;
	int size = 0, server_index;
	int act_start = start, act_end= end;	
	int temp_start = 0, temp_end= end;	
	map<string,pair<int,int>> blocks_to_fetch;
	vector<pair<pair<int,int>,char*>> file_chunks;
	vector<pair<int,int>> missing_chunks; /**/
	char *temp_buf;
	vector<cache_block*> cb_list;
	sort(map_fname_to_chunks[file_name].begin(),map_fname_to_chunks[file_name].end(), sort_vector_cache_block);
	cb_list = map_fname_to_chunks[file_name];
	cache_block* cb;
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": Entered read file: Start: "<< start<< " end : "<< end << "cache_hit : " << *cache_hit;
#endif



	/* iterate over cache blocks of this file*/
	for(auto it  = cb_list.begin(); it!= cb_list.end() ;){
		cb = *it;
		if (cb->start_index > start) {
			/*this block is completely out of range*/
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":" << "partially not in the cache chunk_Start: "<< start << " chunk_end : "<< (cb->start_index>end?end:cb->start_index);
#endif


			missing_chunks.push_back(make_pair(start, cb->start_index>end?end:(cb->start_index-1)));
			start = cb->start_index>end?end:cb->start_index;
			if(start == end) {
#ifdef DEBUG_FLAG
				cout<<"\n1. finished checking blocks, breaking";
#endif
				break;
			}
			continue;
		} 
		/* some or whole block is available */
		if (start>=cb->start_index && start <= cb->end_index) {
			if(end <= cb->end_index) {
				temp_buf = new char[end-start+1]; // 0 to 199 is 200  but 199-0 is 199
				obj_cache->refer(cb);
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Current block start : "<< cb->start_index<< " end: "<< cb->end_index;
		 cout<< "		: Current pointer is at :" << start;
#endif
				memcpy(temp_buf, cb->data+(cb->start_index-start),end-start+1 );
				file_chunks.push_back(make_pair(make_pair(start,end),temp_buf));
				start = end;
#ifdef DEBUG_FLAG
				cout<<"\n2. finished checking blocks, breaking "<< start << " chunk_end : "<< end;
#endif
				break;

			} else {
				temp_buf = new char[cb->end_index-start+1]; 
				obj_cache->refer(cb);
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Current block start : "<< cb->start_index<< " end: "<< cb->end_index;
		 cout << "		 :  Current pointer is at :" << start;
#endif
				memcpy(temp_buf, cb->data+(cb->start_index-start),cb->end_index-start+1 );
				file_chunks.push_back(make_pair(make_pair(start,end),temp_buf));
				start = cb->end_index + 1;
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Read some part, it will next from other block. read till: " << start;
#endif
			}
		}
		it++;
	}
	if(end-start !=0) {
		/* remaining rainge is missing */
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Done reading Blocks in cache : Remaining, start: "<< start << " end: "<< end ;

#endif
		missing_chunks.push_back(make_pair(start, end));
	}
	/* send read request to file server */

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Printing All the missing blocks ";
#endif

	for(auto it = missing_chunks.begin(); it != missing_chunks.end(); it++) {
		all_chunks_available = false;
		pair<int,int> start_end;
		start_end = *it;
		temp_start = start_end.first;
		char *temp_buf = NULL;
		cache_block *cb = NULL;
		string server_name ;
#ifdef DEBUG_FLAG
                 cout<<"\n " <<"			"<< start_end.first <<" -- "<< start_end.second;
#endif
		while(1) {
			server_index = temp_start/FILE_SERVER_CHUNK_SZ;
			temp_end = (start_end.second >
					((server_index+1)*(FILE_SERVER_CHUNK_SZ))-1)?((server_index + 1)*(FILE_SERVER_CHUNK_SZ) - 1):start_end.second;
			
			temp_buf = new char[temp_end-temp_start +1];
#ifdef DEBUG_FLAG
                 cout<<"\n					Sending to server: " << server_index;
		 cout<<"\n						    start: " << temp_start << " end: "<< temp_end;
#endif
			server_name = (file_dir[file_name])->server_list[server_index];
			if(fs_service->fs_read_file_to_server(file_name, temp_buf,
					temp_start,
					temp_end,
					server_name)) {
				file_chunks.push_back(make_pair(make_pair(temp_start,temp_end),temp_buf));
				cb = get_free_cache_block();
				memcpy(cb->data, temp_buf, temp_end-temp_start+1);
				cb->start_index = temp_start;
				cb->end_index = temp_end;
				cb->file_name = file_name;
				cb->dirty = false;
				add_to_front_allocated_list_l(cb);
				obj_cache->refer(cb); /*For LRU*/
			} else {
				cout<<"\nerror while reading file from the file server";
			}
			if(start_end.second > temp_end) {
				/*it means we need to write to more then one fileserve    r*/
				temp_start = temp_end + 1;
			} else {
				break;
			}
		}
	}
	sort(file_chunks.begin(), file_chunks.end(), sort_by_start_file_chunk_vector);
	int current = 0;

#ifdef DEBUG_FLAG
                 cout<<"\n" <<" Got all required chunks, Here the list: ";
#endif


	for(auto it = file_chunks.begin(); it!= file_chunks.end(); it++) {
#ifdef DEBUG_FLAG
                 cout<<"\n 				"<< (*it).first.first <<" -- " << (*it).first.second;
#endif

		memcpy(buf+current, (*it).second,(*it).first.second - (*it).first.first + 1);
		current = current + ((*it).first.second - (*it).first.first) ;
		delete ((*it).second);
	}

	if(all_chunks_available == false) {
		cache_hit = 0;
	}
#ifdef DEBUG_FLAG
	cout<< "\n Read and returing  size: "<<current;
	cout<<"\n cache_hit               : "<<cache_hit;
	cout<< "\n FILE CONTENT: \n";
	cout<<buf;
#endif
	return current;
}


bool cache_manager::write_file (string file_name, const void *buf, int start,int end, int *cache_hit) {

	/* go through the cache, if available overwrite it otherwise create a new block and and add the remaining */
	bool all_chunks_available = false;
	int current_index = 0;
	int size = 0;
	int act_start = start, act_end= end;
	int temp_start = 0, temp_end= 0;
	map<string,pair<int,int>> blocks_to_fetch;
	vector<pair<pair<int,int>,char*>> file_chunks;
	vector<pair<int,int>> missing_chunks; /**/
	char *temp_buf = (char*) buf ;
	vector<cache_block*> cb_list;
	sort(map_fname_to_chunks[file_name].begin(),map_fname_to_chunks[file_name].end(), sort_vector_cache_block);
	cb_list = map_fname_to_chunks[file_name];
	cache_block* cb;
	int current_written_sz = 0;
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<< __func__ <<": Starting Processing";
#endif

	for(auto it  = cb_list.begin(); it!= cb_list.end() ;){
		cb = *it;
		if (cb->start_index > start) {
			/*this block is completely out of range*/
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": ";
#endif
			cb = get_free_cache_block();
			memcpy(cb->data, temp_buf+current_written_sz, cb->start_index>end?end:(cb->start_index-1));
			obj_cache->refer(cb); /*For LRU*/
			current_written_sz = current_written_sz + cb->start_index>end?end:(cb->start_index); // not putting -1 as it will be use to more 1 more position where next time it will write
			cb->start_index = start;
			cb->end_index = cb->start_index>end?end:(cb->start_index-1);
			cb->file_name = file_name;
			cb->dirty = true;
			cb->dirty_range.push_back(make_pair(start, cb->start_index>end?end:(cb->start_index-1)));
			add_to_front_allocated_list_l(cb);
			add_to_dirty_list_l(cb);
			 obj_cache->refer(cb); /*For LRU*/
			start = cb->start_index>end?end:cb->start_index;
			if(start == end) {
#ifdef DEBUG_FLAG
				cout<<"\n1. finished writting the blocks, breaking";
#endif
				break;
			}
			continue;
		}
		/* some or whole block is available */
		if (start>=cb->start_index && start <= cb->end_index) {
			if(end <= cb->end_index) {
				/*Over will happen compeletly  inside this block*/
				memcpy(cb->data+(cb->start_index-start), temp_buf+current_written_sz,end-start+1 );
				obj_cache->refer(cb); /*For LRU*/
				current_written_sz = current_written_sz + end-start+1;
				cb->dirty_range.push_back(make_pair(start, end));

				start = end;
#ifdef DEBUG_FLAG
				cout<<"\n2. finished checking blocks, breaking";
#endif
				break;

			} else {
				/* partiall write will happen here and overflow to next block */
				memcpy(cb->data+(cb->start_index-start),temp_buf+current_written_sz, cb->end_index-start+1 );
				obj_cache->refer(cb); /*For LRU*/
				current_written_sz = current_written_sz + cb->end_index-start+1;
				file_chunks.push_back(make_pair(make_pair(start,end),temp_buf));
				start = cb->end_index + 1;
			}
			collapse_dirty_list(cb);
			obj_cache->refer(cb); /*For LRU*/
		}
		it++;
	}
	int i = 0;
	if(start< end+1) {
		/*looks like append*/
		while( start <= end) {
#ifdef DEBUG_FLAG
			cout<<"\n"<<__func__<<" Getting Free Block From Cache iterationi: " << i++ << " , current size : "<<current_written_sz;
#endif
			cb = get_free_cache_block();

#ifdef DEBUG_FLAG
			cout<<"\n"<<__func__<<" Doing Memcpy";
#endif
			int size_to_copy = (CACHE_BLOCK_SZ)>end-start? (end-start+1): ((CACHE_BLOCK_SZ));
			cout <<"\n size of copy " <<size_to_copy;
			memcpy(cb->data, temp_buf+current_written_sz, CACHE_BLOCK_SZ>end-start?end-start+1:(CACHE_BLOCK_SZ));
#ifdef DEBUG_FLAG
			cout<<"\n"<<__func__<<" Memcpy is finished : data size "<< (CACHE_BLOCK_SZ>end-start?end-start+1:(CACHE_BLOCK_SZ));
			cout<<"\n";
#endif
			current_written_sz = current_written_sz +  CACHE_BLOCK_SZ>end-start?end-start+1:((CACHE_BLOCK_SZ));
			// not putting -1 as it will be use to more 1 more position where next time it will write
			cb->start_index = start;
			cb->end_index =  CACHE_BLOCK_SZ>end-start?end:((CACHE_BLOCK_SZ)+start-1);
			cb->file_name = file_name;
			cb->dirty = true;
			cb->dirty_range.push_back(make_pair(start, CACHE_BLOCK_SZ>end-start?end:((CACHE_BLOCK_SZ)+start-1)));
#ifdef DEBUG_FLAG
			cout<<"\n"<<__func__<<" Cache is added start= "<<cb->start_index <<" end= "<<cb->end_index <<" filename= "<<cb->file_name <<" dirty = "<<cb->dirty;
			cout<<"\n";
#endif

			add_to_front_allocated_list_l(cb);
			add_to_dirty_list_l(cb);
			obj_cache->refer(cb); /*For LRU*/
			start = CACHE_BLOCK_SZ>end-start?end+1:((CACHE_BLOCK_SZ)+start);

#ifdef DEBUG_FLAG
			cout<<"\n";
#endif
		}
	}
}

bool cache_manager::clean_file (string file_name, string operation) {
	/* harvest dirty blocks for this file and clean it from the cache*/
	if(operation == "close") {
	
	
	} else {
	
		/*delete, remove dirty block for this file and also remove it from the map */
	}
}
