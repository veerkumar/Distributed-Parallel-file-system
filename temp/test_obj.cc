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
int main(){


	cache_block  *cb = new cache_block*();
	cache_block *cb_ = new cache_block();

	cout<<cb->num<<"\n";
	cout<<cb->num<<"\n";
	

}
