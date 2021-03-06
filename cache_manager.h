
/*
 *
 * Author: Pramod Kumar(veer)
 *
 * Description:  Cache_manager.h
 * 
 *
 */

#define CACHE_BLOCK_SZ 2048
#define ROW 32
#define COLUMN 32

extern map<string,file_info_store*> file_dir;

class cache_block {
	public:
		bool dirty;
		string file_name;
		int start_index;
		int end_index;
		vector<pair<int,int>> dirty_range;
		char* data;
		cache_block();
		~cache_block();
		bool clean_cache_block(cache_block *cb);

};
class cache {
	public: 
		mutex mutex_lru_list;
		list<cache_block*> lru_list;
		cache_block **slots;
		unordered_map<cache_block*, list<cache_block*>::iterator> lru_indexing;
			
		mutex mutex_lru_indexing;
		cache();
		void refer(cache_block* cb);
		void display_lru_list();
		void lru_item_delete(cache_block* cb);
};
class cache_manager {
	public:
		cache *obj_cache;
		mutex mutx_free_list;
		mutex mutx_allocated_list;
		mutex mutx_dirty_list;
		mutex mutx_map_fname_to_chunks;
		vector<cache_block*> free_list;
		vector<cache_block*> allocated_list;
		vector<cache_block*> dirty_list;
		map<string,vector<cache_block*>> map_fname_to_chunks;
		// Whenever a file is opened or written, update this list, it will be used in dirty_list har    verst
		map<string,vector<string>> map_file_recep;
		cache_manager();
		cache* get_cache_obj () ;
		cache_block* get_free_cache_block ();
		bool add_to_back_free_list_l (cache_block *cb);
		bool add_to_front_free_list_l (cache_block *cb) ;
		bool rm_from_free_list_l (cache_block *cb) ;
		bool rm_from_free_list_l ( vector<cache_block*>::reverse_iterator& it) ;
		bool rm_from_free_list_l ( vector<cache_block*>::iterator& it) ;
		bool add_to_dirty_list_l (cache_block *cb);
		bool rm_from_dirty_list_l (cache_block *cb) ;
		bool add_to_allocated_list_l (cache_block *cb) ;
		bool add_to_front_allocated_list_l (cache_block *cb) ;
		bool rm_from_allocated_list_l (cache_block *cb);
		bool rm_from_allocated_list_l ( vector<cache_block*>::reverse_iterator& it);
		bool rm_from_allocated_list_l ( vector<cache_block*>::iterator& it);
		bool add_to_map_fname_chunks_l (string file_name, cache_block *cb);
		bool rm_from_map_fname_chunks_l (string file_name, cache_block* cb);
		
		int read_file (string file_name, char *buf, int start,int end, int *cache_hit);
                bool write_file (string file_name, const void *buf, int start,int end, int *cache_hit);
 		bool clean_file (string file_name, string operation); 
};

extern cache_manager *c_m;

struct block_list {
        int count;
        vector<cache_block*> list;
};

