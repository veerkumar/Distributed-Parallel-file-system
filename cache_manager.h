//Global object for cache;
#define BLOCK_SIZE 1000 //BYTE
#define ROW 200
#define COLUMN 200

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
		bool clean_cache_block();

};
class cache {
	public:
		cache_block **slots;
		cache();
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
		bool add_to_map_fname_chunks_l (cache_block *cb);
		bool rm_from_map_fname_chunks_l (string file_name, cache_block* cb);
		
		int read_file (string file_name, void *buf, int start,int end, int *cache_hit);
                bool write_file (string file_name, const void *buf, int start,int end, int *cache_hit);
 		bool clean_file (string file_name, string operation); 
};

extern cache_manager *c_m;

struct block_list {
        int count;
        vector<cache_block*> list;
};

