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



enum permission_type {
        READ_PERMISSION = 0,
        WRITE_PERMISSION = 1
};


struct permission_info {
        int start_byte;
        int end_byte;
        permission_type type;
        vector<string> token;   /* It will be used to revoke a permission,
                                   vector as More the one reader can be accessing the file
                                   */
};

 class meta_data_manager {
 	public:
                 vector<string> server_list;
                 map<string,vector<string>> file_to_server_dist_map;
                 map<string,vector<permission_info>> map_file_to_server_dist;
                 map<string,string> token_to_client_map;  /* will fetch clinet info */
                 mutex mut_server_list;
                 mutex mut_file_to_server_dist_map;
                 
                 bool add_server_to_file_to_server_dist_map(string file_name, string server);
                 
                 bool add_server_to_server_list(string server) ;
 };

extern meta_data_manager *m_m;



