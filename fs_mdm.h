#include "cache_manager.h"
#include "meta_data_manager_services.grpc.pb.h"
#include "file_server_services.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using MetaDataManager::RegisterServiceRequest;
using MetaDataManager::RegisterServiceResponse;
using MetaDataManager::UpdateLastModifiedServiceRequest;
using MetaDataManager::UpdateLastModifiedServiceResponse;
using MetaDataManager::MetaDataManagerService;

using FileServer::FileReadWriteRequest;
using FileServer::FileReadWriteResponse;
using FileServer::FileServerService;


mutex fs_write;

class meta_data_manager_client {
	private:
		std::unique_ptr<MetaDataManagerService::Stub> stub_;
	public:
		meta_data_manager_client(std::shared_ptr<Channel> channel): stub_(MetaDataManagerService::NewStub(channel)){};
		register_service_response_t* register_service_handler(register_service_request_t *c_req);
		void update_last_modified_time (string file_name,int size);
};

extern meta_data_manager_client *mdm_service;

vector<pair<string,int>> fileManager;


