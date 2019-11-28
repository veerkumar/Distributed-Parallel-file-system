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

class meta_data_manager_client {
	private:
		std::unique_ptr<MetaDataManagerService::Stub> stub_;
	public:
	//	meta_data_manager_client((std::shared_ptr<Channel> channel): stub_(MetaDataManagerService::NewStub(channel)));
		meta_data_manager_client(std::shared_ptr<Channel> channel): stub_(MetaDataManagerService::NewStub(channel)){};
		file_access_response_t* file_access_request_handler( file_access_request_t *c_req);

};

extern meta_data_manager_client *mdm_service;

int create_new_file (const char *filename, int stripe_width);
void write_file_to_server(cache_block* cb,  int start, int end, string server_ip);
