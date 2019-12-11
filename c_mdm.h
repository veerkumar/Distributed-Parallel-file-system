#include "cache_manager.h"
#include "meta_data_manager_services.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using MetaDataManager::FileAccessRequest;
using MetaDataManager::FileAccessResponse;
using MetaDataManager::MetaDataManagerService;


using MetaDataManager::RegisterServiceRequest;
using MetaDataManager::RegisterServiceResponse;
using MetaDataManager::UpdateLastModifiedServiceRequest;
using MetaDataManager::UpdateLastModifiedServiceResponse;

class meta_data_manager_client {
	private:
		std::unique_ptr<MetaDataManagerService::Stub> stub_;
	public:
		meta_data_manager_client(std::shared_ptr<Channel> channel): stub_(MetaDataManagerService::NewStub(channel)){};
		file_access_response_t* file_access_request_handler( file_access_request_t *c_req);
		 register_service_response_t* register_service_handler( register_service_request_t *c_req);

};

extern meta_data_manager_client *mdm_service;

int mm_create_new_file(const char *filename, int stripe_width) ;
int mm_open_file(const char *filename, const char mode);
 int mm_get_read_permission (int fdis, size_t nbyte, off_t offset);
 int mm_get_write_permission (int fdis, size_t nbyte, off_t offset);
int mm_delete_file(const char *filename);
int mm_get_fstat(string filename, struct pfs_stat *buf);

extern string client_server_ip_port ;
extern map<string,file_info_store*> file_dir;

extern map<int, string> fdis_to_filename_map;

extern map<string,pair<int,int>> token_map;


FileAccessRequest::RequestType
get_grpc_type (request_type type); 

request_type
get_c_type(FileAccessRequest::RequestType type);
