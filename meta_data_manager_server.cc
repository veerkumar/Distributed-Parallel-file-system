#include "commons.h"
#include "meta_data_manager_services.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using MetaDataManager::FileAccessRequest;
using MetaDataManager::FileAccessResponse;
using MetaDataManager::RegisterServiceRequest;
using MetaDataManager::RegisterServiceResponse;
using MetaDataManager::MetaDataManagerService;

map<string,vector<string>> map_file_to_server_dist;
vector<string> server_list;

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

class meta_data_manager_server {
	public:
	       	vector<string> server_list;
		map<string,vector<string>> file_to_server_dist_map;
		map<string,vector<permission_info>> map_file_to_server_dist;
		map<string,string> token_to_client_map;  /* will fetch clinet info */
		mutex mut_server_list;
		mutex mut_file_to_server_dist_map;
		
		bool add_server_to_file_to_server_dist_map(string file_name, string server){

			mut_file_to_server_dist_map.lock();
			file_to_server_dist_map[file_name].push_back(server);
			mut_file_to_server_dist_map.unlock();
			return true;
		}

		bool add_server_to_server_list(string server) {
			mut_server_list.lock();
			server_list.push_back(server);	
			mut_server_list.unlock();	
		}

};
class meta_data_manager_service_impl : public MetaDataManagerService::Service {
	Status fileAccessRequestHandler (ServerContext* context,const  FileAccessRequest* request,
			FileAccessResponse* reply) override {
		std::cout << "\nGot the message ";

		cout<<"\n"<<request->reqipaddrport(); 
		cout<<"\n"<<request->startbyte(); 
		cout<<"\n"<<request->endbyte();
		cout<<"\n"<<request->requestid(); 
		cout<<"\n"<<request->filename();
		cout<<"\n"<<request->reqipaddrport(); 
		reply->set_requestid(request->requestid());
		reply->set_reqstatus("OK");
		reply->set_token("121212121212");
		return Status::OK;
	}

        Status registerServiceHandler(ServerContext* context,const  RegisterServiceRequest* request, RegisterServiceResponse* reply) override {

            std::cout << "\nGot the message ";

            cout<<"\n"<<request->type();
            cout<<"\n"<<request->ipport();
            reply->set_code(RegisterServiceResponse::OK);
            return Status::OK;
    }


};
void RunServer() {

	std::string server_address("0.0.0.0:50051");
	meta_data_manager_service_impl service;

	    ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}

