#include "../commons.h"
#define ROW 3
#define COLUMN 3
class cache_block {
    public:
        int num;
        string name;
        string permission;
        cache_block(){
         num = 2;
         name = "veer";
        }
        ~cache_block(){}
};
class cache {
         public:
	 //vector<vector<cache_block>> slots(3, vector<cache_block>(3));
	 //cache_block *slots[ROW][COLUMN];
 	cache_block **slots;
         cache(){
                // slots = new cache_block[ROW][COLUMN];
		slots = new cache_block*[ROW];
		for(int i =0; i<ROW;i++) {
			slots[i] = new cache_block[COLUMN];
		}
         }
 
 
 }; 


int main(){
	cache *obj_cache = new cache();
	vector<cache_block*> vec;
	for(int i=0; i<ROW;i++) {
		for(int j=0; j<COLUMN;j++) {
			vec.push_back(&obj_cache->slots[i][j]);
		}}
	auto it = vec.begin();
	it++;
	(*it)->num = 4;
	 it = vec.begin();
	vec.erase(it);

	for(int i=0; i<ROW;i++) {
		for(int j=0; j<COLUMN;j++) {
		cout<<" "<<	obj_cache->slots[i][j].num;
		}
		cout<<"\n";
	}
}
