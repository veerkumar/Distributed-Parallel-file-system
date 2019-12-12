#include "commons.h"
#include "fs_mdm.h"
#include "config.h"


#define INTERFACE "ens33"


string ipAddress;
int port;
string file_server_ip_port;

 meta_data_manager_client *mdm_service;

void  
set_ipaddr_port() {
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr->sa_family == AF_INET) {
				if(strcmp(temp_addr->ifa_name, INTERFACE)==0){
					ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
#ifdef DEBUG_FLAG				
					cout<<"\n IPaddress"<<ipAddress;
#endif
				}
			}
			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	/* initialize random seed: */
	srand (time(NULL));

	/* generate secret number between 5000 to 65000: */
	port = rand() % 60000 + 5000;
	file_server_ip_port.append(ipAddress);
	file_server_ip_port.append(":");
	file_server_ip_port.append(to_string(port));
	//return file_server_ip_port;
}


int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}

void 
print_response(register_service_response_t *c_response) {
	cout<<"\n"<<c_response->code;
	cout<<"\n";
}

void 
print_request(register_service_request_t *c_req) {
	cout<<"\n"<<c_req->type;
	cout<<"\n"<<c_req->ip_port;
}

register_service_response_t* 
extract_response_from_payload(RegisterServiceResponse Response) {
	register_service_response_t *c_response = new register_service_response_t;
	if(Response.code() == RegisterServiceResponse::OK) {
		c_response->code = OK;
	}
	if(Response.code() == RegisterServiceResponse::ERROR) {
		c_response->code = ERROR;
	}
	return c_response;
}

void 
make_req_payload (RegisterServiceRequest *payload, 
		register_service_request_t *req) {
	if(req->type == FILE_SERVER) {
		payload->set_type(RegisterServiceRequest::FILESERVER);
	}
	payload->set_ipport(req->ip_port);
}

/*Class: which hold function which we will use to call me services*/
		

register_service_response_t* meta_data_manager_client::register_service_handler( register_service_request_t *c_req) {
	RegisterServiceRequest ReqPayload;
	RegisterServiceResponse Response;
	ClientContext Context;

	register_service_response_t *c_response = NULL;
	make_req_payload(&ReqPayload, c_req);

	print_request(c_req);

	// The actual RPC.
	Status status = stub_->registerServiceHandler(&Context, ReqPayload, &Response);

	// Act upon its status.
	if (status.ok()) {
		c_response = extract_response_from_payload(Response);
		return c_response;
	} else {
		std::cout << status.error_code() << ": " << status.error_message()
			<< std::endl;
		return 0;
	}
}

void meta_data_manager_client::update_last_modified_time (string file_name,int size) {
	UpdateLastModifiedServiceRequest Req;
	UpdateLastModifiedServiceResponse Response;
	ClientContext Context;

	Req.set_filename(file_name);
	Req.set_time(time(0));  // epoch time
	Req.set_newbytewrote(size);  // epoch time
	Status status = stub_->updateLastModifiedServiceHandler(&Context, Req, &Response);
	// Act upon its status.
	if (status.ok()) {
		cout<<"Last modified update return code: "<< Response.code();
		return ;
	} else {
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return ;
	}
}

class file_server_service_impl : public FileServerService::Service {
        
	Status fileReadWriteRequestHandler (ServerContext* context,const  FileReadWriteRequest* request, FileReadWriteResponse* reply) override {

#ifdef DEBUG_FLAG
	cout<<"\n\n"<<__func__<<" FS received request";

#endif
	    FILE *fid;
	    char *buffer;
	    size_t result;
	    int found=0;
	    string name;
	    if (request->type() == FileReadWriteRequest::READ) {
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" READ from file server";
	cout<<"\n"<<__func__<<" Request IP Address ="<< request->reqipaddrport();
        cout<<"\n"<<__func__<<" Request Start Byte = "<< request->startbyte();
        cout<<"\n"<<__func__<<" Request End Byte = "<< request->endbyte();
        cout<<"\n"<<__func__<<" Request Request ID ="<< request->requestid();
        cout<<"\n"<<__func__<<" Request File Name ="<< request->filename();
#endif
		for(auto it=fileManager.begin();it!=fileManager.end();it++){
			if((*it).first.compare(request->filename())==0){
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" StripWidth ="<< (*it).second;
#endif
				uint32_t fileNumber=((request->startbyte()/(STRIP_SIZE*1024*PFS_BLOCK_SIZE))%(*it).second)/NUM_FILE_SERVERS;
				name= (*it).first +"."+file_server_ip_port+"."+ std::to_string(fileNumber);
				fid=fopen(name.c_str(),"r");
				reply->set_requestid(request->requestid());
			     	reply->set_reqstatus(FileReadWriteResponse::OK);
				buffer = (char*) malloc (sizeof(char)*(request->endbyte()-request->startbyte()+1));
				uint32_t i=request->startbyte()/(STRIP_SIZE*1024*PFS_BLOCK_SIZE);
				uint32_t offset=(((i-i%7)/(*it).second)*((*it).second-1)*(STRIP_SIZE*1024*PFS_BLOCK_SIZE))+(i%(*it).second)*4096;
				uint32_t startByte=request->startbyte()-offset;
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Reading File =" << name;
	cout<<"\n"<<__func__<<" Offset ="<< startByte;
#endif
			  	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
				fseek(fid,startByte,SEEK_SET);
			  	// copy the file into the buffer:
			  	result = fread (buffer,1,request->endbyte()-request->startbyte()+1,fid);
			  	if (result != request->endbyte()-request->startbyte()+1) {fputs ("Reading error",stderr); exit (3);}
				fclose(fid);		    	
				reply->set_data(buffer);
				return Status::OK;
			}
		}
		reply->set_requestid(request->requestid());
		reply->set_reqstatus(FileReadWriteResponse::ERROR);
		return Status::CANCELLED;
             	
	    } else if (request->type() == FileReadWriteRequest::WRITE) {
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Write to file server";
	cout<<"\n"<<__func__<<" Request IP Address ="<< request->reqipaddrport();
        cout<<"\n"<<__func__<<" Request Start Byte = "<< request->startbyte();
        cout<<"\n"<<__func__<<" Request End Byte = "<< request->endbyte();
        cout<<"\n"<<__func__<<" Request Request ID ="<< request->requestid();
        cout<<"\n"<<__func__<<" Request File Name ="<< request->filename();
#endif
		    uint32_t fileNumber=((request->startbyte()/(STRIP_SIZE*1024*PFS_BLOCK_SIZE))%request->stripwidth())/NUM_FILE_SERVERS;
		    name= request->filename() +"."+file_server_ip_port+"."+ std::to_string(fileNumber);
		    fid=fopen(name.c_str(),"w");
		    reply->set_requestid(request->requestid());
		    reply->set_reqstatus(FileReadWriteResponse::OK);
		    uint32_t i=request->startbyte()/(STRIP_SIZE*1024*PFS_BLOCK_SIZE);
		    uint32_t offset=(((i-i%7)/request->stripwidth())*(request->stripwidth()-1)*(STRIP_SIZE*1024*PFS_BLOCK_SIZE))+(i%request->stripwidth())*4096;
		    uint32_t startByte=request->startbyte()-offset;
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" Writing File =" << name;
	cout<<"\n"<<__func__<<" Offset ="<< startByte;
#endif
		    fseek(fid,startByte,SEEK_SET);
		    fwrite ((void *)&(request->data()),1,request->endbyte()-request->startbyte()+1,fid);
		    fclose(fid);		    	
				
		    
		    for(auto it=fileManager.begin();it!=fileManager.end();it++){
			if((*it).first.compare(request->filename())==0){
				found=1;
				continue;
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" File is available " << name;
#endif
			}
		    }
		    if(found==0){
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" File is new " << name;
#endif
			fileManager.push_back(make_pair(request->filename(),request->stripwidth()));
		    }
		    mdm_service->update_last_modified_time(request->filename(),request->endbyte());
	    }else {
#ifdef DEBUG_FLAG
	cout<<"\n"<<__func__<<" DELETE from file server";
	cout<<"\n"<<__func__<<" Request IP Address ="<< request->reqipaddrport();
        cout<<"\n"<<__func__<<" Request Request ID ="<< request->requestid();
        cout<<"\n"<<__func__<<" Request File Name ="<< request->filename();
#endif
		   
		string del="exec rm -r ./"+request->filename()+"."+file_server_ip_port+"*";
		system(del.c_str());
	    }

            return Status::OK;
    }


};


void initialization(){
	/*Create file server ip and port*/
	set_ipaddr_port();
	
	/* Start fileserver */
	file_server_service_impl fs_server;

	ServerBuilder builder;
	builder.AddListeningPort(file_server_ip_port, grpc::InsecureServerCredentials());	
	builder.RegisterService(&fs_server);
	std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "File Server listening on " << file_server_ip_port << std::endl;


	/* Register fileserver service with metadata manager */
	mdm_service = new meta_data_manager_client (grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

	register_service_request_t *c_req = new register_service_request_t;
	register_service_response_t *c_response = NULL;

	c_req->type  = FILE_SERVER;
	c_req->ip_port = file_server_ip_port;

	c_response = mdm_service->register_service_handler(c_req);
	cout<<"Response recieved";
	print_response(c_response);

    /* Call wont return, it will wait for server to shutdown */
     server->Wait();
	return ;
}

int main(int argc, char** argv) {
	initialization();
	
	return 0;
}
