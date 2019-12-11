#include "commons.h"
#include "cs_mdm.h"
#include "client_server.h"

string client_server_ip_port ;
string ipAddress;
int port;

class client_server_service_impl : public ClientServerService::Service {

        Status fileRevokePermissionRequestHandler (ServerContext* context,const  FilePermissionRevokeRequest* request, FilePermissionRevokeResponse* reply) override {

            std::cout << "\nGot permission invoke message ";
	    /*I dont think we need to implement READ permission invoke*/
	    if (request->type() == FilePermissionRevokeRequest::WRITE) {

		    cout<<"\n Start byte: "<<request->startbyte();
		    cout<<"\n End byte: "<<request->endbyte();
		    cout<<"\n Request id: "<<request->requestid();
		    cout<<"\n File name: "<<request->filename();

		    /*TODO based on this request, take a lock and modify the conflicting permission
		     * if current permission is write from  5-15 position and revoke permission ask for 4-20 then we can(double check more cases) reject the request. */
		    reply->set_requestid(request->requestid());
	    }


            return Status::OK;
    }


};

void start_client_server(){
	/* Start fileserver */
	client_server_service_impl cs_server;

	ServerBuilder builder;
	builder.AddListeningPort(client_server_ip_port, grpc::InsecureServerCredentials());
	builder.RegisterService(&cs_server);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Client Server listening on " << client_server_ip_port << std::endl;
	server->Wait();




}

