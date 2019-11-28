#include "commons.h"
#include "client_to_meta_manager.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using MetaDataManager::FileAccessRequest;
using MetaDataManager::FileAccessResponse;
using MetaDataManager::MetaDataManagerService;


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

