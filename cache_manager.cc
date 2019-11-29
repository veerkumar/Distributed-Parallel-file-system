#include "commons.h"
#include "cache_manager.h"
#include "config.h"
using namespace std;

cache_manager *c_m;

cache_block:: cache_block() {
	dirty = false;
	file_name = "";
	start_index = 0;
	end_index = 0;
	data = new char[CLIENT_CACHE_SIZE*MEGA];
}
cache_block::~cache_block(){
	if(data != NULL) {
		delete data;
		dirty_range.clear();
	}
}

bool cache_block::clean_cache_block(){
	if (dirty == false) {
		file_name.erase();
		start_index = 0;
		end_index = 0;
		delete data;
		data = NULL;
	} else {
#ifdef DEBUG_FLAG
		cout<<"This block is dirty name: " << file_name;
#endif
		return false;
	}
	return true;
}


cache::cache(){
	slots = new cache_block*[ROW];
	for(int i =0; i<ROW;i++) {
		slots[i] = new cache_block[COLUMN];
	}     
}

cache_manager::cache_manager() {
	if (obj_cache == NULL) {
		obj_cache =  new cache;
		/* add newly allocated in the free list */
		for(int i=0; i<ROW;i++) {
			for(int j=0; j<COLUMN;j++) {
				free_list.push_back(&obj_cache->slots[i][j]);
			}
		}
	}
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

bool cache_manager::add_to_dirty_list_l (cache_block *cb) {
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
	mutx_allocated_list.unlock();
	return true;
}
/*
bool cache_manager::rm_from_allocated_list_l (cache_block *cb) {
	return true;
}*/

bool cache_manager::rm_from_allocated_list_l ( vector<cache_block*>::reverse_iterator& it) {
	mutx_allocated_list.lock();
//	allocated_list.erase(it);
	mutx_allocated_list.unlock();
	return true;
}
bool cache_manager::rm_from_allocated_list_l ( vector<cache_block*>::iterator& it) {
	mutx_allocated_list.lock();
	allocated_list.erase(it);
	mutx_allocated_list.unlock();
	return true;
}

bool cache_manager::add_to_map_fname_chunks_l (cache_block *cb) {
	return true;
}

bool cache_manager::rm_from_map_fname_chunks_l (string file_name, cache_block* cb) {
	return true;	
}
