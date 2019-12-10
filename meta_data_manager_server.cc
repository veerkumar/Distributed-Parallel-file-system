#include "commons.h"
#include "meta_data_manager_services.grpc.pb.h"
#include "client_server_services.grpc.pb.h"
#include "m_mdm.h"

meta_data_manager *m_m;

bool meta_data_manager::add_server_to_file_to_server_dist_map(string file_name, string server){

	mut_file_to_server_dist_map.lock();
	file_to_server_dist_map[file_name].push_back(server);
	mut_file_to_server_dist_map.unlock();
	return true;
}

bool meta_data_manager::add_server_to_server_list(string server) {
	mut_server_list.lock();
	server_list.push_back(server);	
	mut_server_list.unlock();	
}


void 
revoke_client::send_revoke_request () {

		FilePermissionRevokeResponse Response;
		FilePermissionRevokeRequest Req ;
		ClientContext Context;

		/*Make you payload(i.e response from c_data structe)*/
		Status status = stub_->fileRevokePermissionRequestHandler(&Context, Req, &Response);
		if (status.ok()) {
			cout<<"revoke request return code: "<< Response.code()    ;
			return ;
		} else {
			std::cout << status.error_code() << ": " << status.error_message() << std::endl;
			return ;
		}
}



/* Add meta data manager function's(which will be called by remote clients) Here */

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
		reply->set_code(FileAccessResponse::OK);
		int i = 0;
		if (request->type() == FileAccessRequest::CREATE) {
			if(m_m->server_list.size() < request->stripwidth()) {
				cout<<"Error: stripwidth size is more then available server\n";
				reply->set_code(FileAccessResponse::ERROR);
			} else {
				 reply->set_code(FileAccessResponse::OK);
				 for(auto it = m_m->server_list.begin();
						 it!=m_m->server_list.end(), i<(request->stripwidth());
						 it++,i++) {
				 reply->add_serverlist(m_m->server_list[i]);
				 }
			}	
		
		}
		/*REMOVE_ME*/
		m_m->client_connection[request->reqipaddrport()]->send_revoke_request();
		return Status::OK;
	}

	bool create_connection_with_client(string ip_port) {

		m_m->client_connection[ip_port] =  new revoke_client(grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
	}

        Status registerServiceHandler(ServerContext* context,const  RegisterServiceRequest* request, RegisterServiceResponse* reply) override {

            std::cout << "\nGot the message ";

            cout<<"\n"<<request->type();
            cout<<"\n"<<request->ipport();
	    m_m->add_server_to_server_list(request->ipport());
            reply->set_code(RegisterServiceResponse::OK);

	    	if(request->type() == RegisterServiceRequest::CLIENT) {
			//need lock
			create_connection_with_client(request->ipport());
		}
            return Status::OK;
    }


         Status updateLastModifiedServiceHandler(ServerContext* context,const  UpdateLastModifiedServiceRequest* request, UpdateLastModifiedServiceResponse *reply) override {

        std::cout << "\nGot the message ";

        cout<<"\n"<<request->time();
        reply->set_code(UpdateLastModifiedServiceResponse::OK);
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
	m_m = new meta_data_manager;
	RunServer();

	return 0;
}

