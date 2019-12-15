#include "cache_manager.h"
#include "meta_data_manager_services.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using MetaDataManager::FileAccessRequest;
using MetaDataManager::FileAccessResponse;
using MetaDataManager::MetaDataManagerService;


using MetaDataManager::RegisterServiceRequest;
using MetaDataManager::RegisterServiceResponse;
using MetaDataManager::UpdateLastModifiedServiceRequest;
using MetaDataManager::UpdateLastModifiedServiceResponse;

using ClientServer::FilePermissionRevokeRequest;
using ClientServer::FilePermissionRevokeResponse;
using ClientServer::ClientServerService;


struct mm_permission{
	uint32_t start_byte;
	uint32_t end_byte;
	char access_type;
	string client_ipaddr_port;
};


mutex fileListLock;
mutex mut_client_list;
typedef struct file_list_{
	pthread_mutex_t fileLock;
	string name;
	uint32_t fileID;
	uint32_t size;
	long int creation_time;
	long int modification_time;
	uint32_t stripe_width;
	vector<mm_permission> access_permissions;
	vector <string> server_name;
}file_list_t;

vector<file_list_t> fileList;

class revoke_client {
        private:
                 std::unique_ptr<ClientServerService::Stub> stub_;
         public:
                 revoke_client(std::shared_ptr<Channel> channel): stub_(ClientServerService::NewStub(channel)){};

        revoke_access_response_t* send_revoke_request (revoke_access_request_t *c_req);


};

 class meta_data_manager {
 	public:
                 vector<string> server_list;
		 map<string,revoke_client*> client_connection;
                 //map<string,vector<string>> file_to_server_dist_map;
                 //map<string,vector<permission_info>> map_file_to_server_dist;
                 //map<string,string> token_to_client_map;  /* will fetch clinet info */
                 mutex mut_server_list;
                 //mutex mut_file_to_server_dist_map;
                 
                 //bool add_server_to_file_to_server_dist_map(string file_name, string server);
                 
                 bool add_server_to_server_list(string server) ;
 };

extern meta_data_manager *m_m;



