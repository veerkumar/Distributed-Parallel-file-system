#include "commons.h"
#include "meta_data_manager_services.grpc.pb.h"
#include "client_server_services.grpc.pb.h"
#include "m_mdm.h"

meta_data_manager *m_m;

/*bool meta_data_manager::add_server_to_file_to_server_dist_map(string file_name, string server){

	mut_file_to_server_dist_map.lock();
	file_to_server_dist_map[file_name].push_back(server);
	mut_file_to_server_dist_map.unlock();
	return true;
}*/

bool meta_data_manager::add_server_to_server_list(string server) {
	mut_server_list.lock();
	server_list.push_back(server);	
	mut_server_list.unlock();	
}

int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}

revoke_access_response_t* 
extract_response_from_payload(FilePermissionRevokeResponse Response) {
	revoke_access_response_t *c_response = new revoke_access_response_t;
	c_response->request_id = Response.requestid();
	c_response->token= Response.token();
	c_response->end_byte= Response.endbyte();
	c_response->start_byte= Response.startbyte();
	if(Response.code() == FilePermissionRevokeResponse::OK) {
                c_response->code = OK;
        }
	else
		c_response->code = ERROR;
	return c_response;
}


revoke_access_response_t* revoke_client::send_revoke_request (revoke_access_request_t *c_req) {

		FilePermissionRevokeResponse Response;
		FilePermissionRevokeRequest Req ;
		ClientContext Context;

		revoke_access_response_t *c_response = NULL;
		if(c_req->type==READ_REVOKE)
			Req.set_type(FilePermissionRevokeRequest::READ);
		else if(c_req->type==WRITE_REVOKE)
			Req.set_type(FilePermissionRevokeRequest::WRITE);
		else
			Req.set_type(FilePermissionRevokeRequest::DELETE);
		Req.set_startbyte(c_req->start_byte);
		Req.set_endbyte(c_req->end_byte);
		Req.set_requestid(c_req->request_id);
		Req.set_filename(c_req->file_name);
		Req.set_token(c_req->token);
	
		/*Make you payload(i.e response from c_data structe)*/
		Status status = stub_->fileRevokePermissionRequestHandler(&Context, Req, &Response);
		if (status.ok()) {
			cout<<"revoke request return code: "<< Response.code();
			c_response = extract_response_from_payload(Response);
			return c_response;
		} else {
			std::cout << status.error_code() << ": " << status.error_message() << std::endl;
			return 0;
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
	int i = 0;
	std::vector<file_list_t>::iterator it; 
	std::vector<permission>::iterator it2;
	std::vector<string>::iterator it3;
	vector<permission>permission_del;
	vector<permission>permission_ins;
	vector<int>index;
	if (request->type() == FileAccessRequest::CREATE) {
	    reply->set_code(FileAccessResponse::OK);
	    for(it=fileList.begin();it != fileList.end(); it++){
    
                if(it->name.compare(request->filename())==0){
                    reply->set_code(FileAccessResponse::ERROR);
		    return Status::CANCELLED; 
		}
	    }
	    file_list_t n1;
	    n1.name=request->filename();
	    n1.fileID=get_random_number();
	    n1.size=0;
	    n1.stripe_width=request->stripewidth();
	    n1.creation_time=static_cast<long int> (time(NULL));
	    n1.modification_time=static_cast<long int> (time(NULL));
	    for(i=0,it3=m_m->server_list.begin();i<request->stripewidth(),it3!=m_m->server_list.end();i++,it3++){
		n1.server_name.push_back(*it3);
	    }
	    fileList.push_back(n1);
        }
        else if (request->type() == FileAccessRequest::DELETE) {
	    revoke_access_request_t *c_req = new revoke_access_request_t;
	    revoke_access_response_t *c_response = NULL;
	    i=0;
	    for(it=fileList.begin();it != fileList.end(); it++){
    		
                if(it->name.compare(request->filename())==0){
                    for(it2=it->access_permissions.begin();it2 != it->access_permissions.end(); it2++){
			c_req->type=DELETE_REVOKE;
			c_req->start_byte=it2->start_byte;
		        c_req->end_byte=it2->end_byte;
		        c_req->request_id=get_random_number();
		        c_req->file_name=request->filename();
			c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
		    }
		    fileList.erase(fileList.begin()+i);  
		    break;
		}
		i++;
	    }
        }
        else if (request->type() == FileAccessRequest::WRITE) {
	    revoke_access_request_t *c_req = new revoke_access_request_t;
	    revoke_access_response_t *c_response = NULL;
            for(it=fileList.begin();it != fileList.end(); it++){
    
                if(it->name.compare(request->filename())==0){
                    i=0;
		    if(it->access_permissions.size()==0){
			permission_ins.push_back({0,it->size,'w',request->reqipaddrport()});
		    }
		    else{
		            for(it2=it->access_permissions.begin();it2 != it->access_permissions.end(); it2++){
		                if(request->reqipaddrport()==it2->client_ipaddr_port){
		                    if(it2->access_type=='r'){
		                        i++;        
		                        continue;
		                    }
		                }
		                index.push_back(i);
		                i++;
		                if(it2->start_byte>=request->startbyte() && it2->end_byte<=request->endbyte()){
				    if(it2->access_type=='r')
		                	c_req->type=READ_REVOKE;
				    else
		                        c_req->type=WRITE_REVOKE;
				    c_req->start_byte=it2->start_byte;
				    c_req->end_byte=it2->end_byte;
				    c_req->request_id=get_random_number();
				    c_req->file_name=request->filename();
				    c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
				    permission_del.push_back(*it2);
				    
		            }else if(it2->start_byte<request->startbyte() && it2->end_byte<=request->endbyte()){

				if(it2->access_type=='r')
				    c_req->type=READ_REVOKE;
				else
		                    c_req->type=WRITE_REVOKE;
				c_req->start_byte=request->startbyte();
				c_req->end_byte=it2->end_byte;
				c_req->request_id=get_random_number();
				c_req->file_name=request->filename();
				c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
				permission_del.push_back(*it2);
		                permission_ins.push_back({it2->start_byte,request->startbyte()-1,it2->access_type,it2->client_ipaddr_port});

		            }else if(it2->start_byte>=request->startbyte() && it2->end_byte>request->endbyte()){
		                			
				if(it2->access_type=='r')
		                	c_req->type=READ_REVOKE;
				else
		                	c_req->type=WRITE_REVOKE;
				c_req->start_byte=it2->start_byte;
				c_req->end_byte=request->endbyte();
				c_req->request_id=get_random_number();
				c_req->file_name=request->filename();
				c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
				
		                permission_del.push_back(*it2);
		                permission_ins.push_back({request->endbyte()+1,it2->end_byte,it2->access_type,it2->client_ipaddr_port});

		            }else if(it2->start_byte<request->startbyte() && it2->end_byte>request->endbyte()){
		                if(it2->access_type=='r')
		                    c_req->type=READ_REVOKE;
				else
		                    c_req->type=WRITE_REVOKE;
				c_req->start_byte=request->startbyte();
				c_req->end_byte=request->endbyte();
				c_req->request_id=get_random_number();
				c_req->file_name=request->filename();
				c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
			        
		                permission_del.push_back(*it2);
		                permission_ins.push_back({it2->start_byte,request->startbyte()-1,it2->access_type,it2->client_ipaddr_port});
		                permission_ins.push_back({request->endbyte()+1,it2->end_byte,it2->access_type,it2->client_ipaddr_port});
		            }
		       }
		       permission_ins.push_back({request->startbyte(),request->endbyte(),'w',request->reqipaddrport()});
		    }
		    cout << "DEL:";
		    i=0;
		    for(it2=permission_del.begin();it2 != permission_del.end(); it2++){
			it->access_permissions.erase(it->access_permissions.begin()+index[i]-i);
			i++;
			
			std::cout << it2->start_byte << ' ' << it2->end_byte <<' ';
		    }
		    cout << "\nINS:";

		    for(it2=permission_ins.begin();it2 != permission_ins.end(); it2++){
			it->access_permissions.push_back(*it2);
			std::cout << it2->start_byte << ' ' << it2->end_byte <<' ';
		    }
		}
            }
        }
        else if (request->type() == FileAccessRequest::READ) {
	    revoke_access_request_t *c_req = new revoke_access_request_t;
	    revoke_access_response_t *c_response = NULL;
            for(it=fileList.begin();it != fileList.end(); it++){
    
                if(it->name.compare(request->filename())==0){
                    i=0;
		    if(it->access_permissions.size()==0){
			permission_ins.push_back({0,it->size,'r',request->reqipaddrport()});
		    }
		    else{
		            for(it2=it->access_permissions.begin();it2 != it->access_permissions.end(); it2++){
		                if(request->reqipaddrport()==it2->client_ipaddr_port){
		                    if(it2->access_type=='w'){
		                        i++;        
		                        continue;
		                    }
		                }
		                index.push_back(i);
		                i++;
		                if(it2->start_byte>=request->startbyte() && it2->end_byte<=request->endbyte()){
		                    if(it2->access_type=='w'){
		                        c_req->type=WRITE_REVOKE;
					c_req->start_byte=it2->start_byte;
					c_req->end_byte=it2->end_byte;
					c_req->request_id=get_random_number();
					c_req->file_name=request->filename();
					c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
					permission_del.push_back(*it2);
		                    }
		                }else if(it2->start_byte<request->startbyte() && it2->end_byte<=request->endbyte()){
		                    if(it2->access_type=='w'){
		                        c_req->type=WRITE_REVOKE;
					c_req->start_byte=request->startbyte();
					c_req->end_byte=it2->end_byte;
					c_req->request_id=get_random_number();
					c_req->file_name=request->filename();
					c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
					permission_del.push_back(*it2);
		                        permission_ins.push_back({it2->start_byte,request->startbyte()-1,it2->access_type,it2->client_ipaddr_port});
		                    }
		                }else if(it2->start_byte>=request->startbyte() && it2->end_byte>request->endbyte()){
		                    if(it2->access_type=='w'){
		                        c_req->type=WRITE_REVOKE;
					c_req->start_byte=it2->start_byte;
					c_req->end_byte=request->endbyte();
					c_req->request_id=get_random_number();
					c_req->file_name=request->filename();
					c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
					permission_del.push_back(*it2);
		                        permission_ins.push_back({request->endbyte()+1,it2->end_byte,it2->access_type,it2->client_ipaddr_port});
		                    }
		                } else if(it2->start_byte<request->startbyte() && it2->end_byte>request->endbyte()){
		                    if(it2->access_type=='w'){
		                        c_req->type=WRITE_REVOKE;
					c_req->start_byte=request->startbyte();
					c_req->end_byte=request->endbyte();
					c_req->request_id=get_random_number();
					c_req->file_name=request->filename();
					c_response=m_m->client_connection[it2->client_ipaddr_port]->send_revoke_request(c_req);
					
		                        permission_del.push_back(*it2);
		                        permission_ins.push_back({it2->start_byte,request->startbyte()-1,it2->access_type,it2->client_ipaddr_port});
		                        permission_ins.push_back({request->endbyte()+1,it2->end_byte,it2->access_type,it2->client_ipaddr_port});
		                    }
		                }
		            }
		            permission_ins.push_back({request->startbyte(),request->endbyte(),'r',request->reqipaddrport()});
			}
			cout << "DEL:";
		    	i=0;
		    	for(it2=permission_del.begin();it2 != permission_del.end(); it2++){
			    it->access_permissions.erase(it->access_permissions.begin()+index[i]-i);
			    i++;
			
			    std::cout << it2->start_byte << ' ' << it2->end_byte <<' ';
		        }
		        cout << "\nINS:";

		        for(it2=permission_ins.begin();it2 != permission_ins.end(); it2++){
			    it->access_permissions.push_back(*it2);
			    std::cout << it2->start_byte << ' ' << it2->end_byte <<' ';
		        }
		     }
		
		}
        }
        else if (request->type() == FileAccessRequest::OPEN) {
	    reply->set_code(FileAccessResponse::ERROR);
	    for(it=fileList.begin();it != fileList.end(); it++){
		if(it->name.compare(request->filename())==0){
		    reply->set_code(FileAccessResponse::OK);
		    reply->set_createtime(it->creation_time);
		    reply->set_lastupdatetime(it->modification_time);
		    reply->set_filesize(it->size);
		    for(auto it2=it->server_name.begin();it2 != it->server_name.end(); it2++)
			reply->add_serverlist(*it2);
		    
		}
	    } 
        }
        else if (request->type() == FileAccessRequest::FSTAT) {
	    reply->set_code(FileAccessResponse::ERROR);
	    for(it=fileList.begin();it != fileList.end(); it++){
		if(it->name.compare(request->filename())==0){
		    reply->set_code(FileAccessResponse::OK);
		    reply->set_createtime(it->creation_time);
		    reply->set_lastupdatetime(it->modification_time);
		    reply->set_filesize(it->size);
		    for(auto it2=it->server_name.begin();it2 != it->server_name.end(); it2++)
			reply->add_serverlist(*it2);
		    
		} 
	    }   
        }
        
        return Status::OK;
    }

    bool create_connection_with_client(string ip_port) {
	m_m->client_connection[ip_port] =  new revoke_client(grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
    }

     Status registerServiceHandler(ServerContext* context,const  RegisterServiceRequest* request, RegisterServiceResponse* reply) override {

	std::cout << "\nGot the message ";

	cout<<"\n"<<request->type();
	cout<<"\n"<<request->ipport();

	if(request->type() == RegisterServiceRequest::CLIENT) {
	   //need lock
	   create_connection_with_client(request->ipport());
	   reply->set_code(RegisterServiceResponse::OK);
	}
	else if(request->type() == RegisterServiceRequest::FILESERVER) {
	    m_m->add_server_to_server_list(request->ipport());
	    reply->set_code(RegisterServiceResponse::OK);
	}
	return Status::OK;
    }


    Status updateLastModifiedServiceHandler(ServerContext* context,const  UpdateLastModifiedServiceRequest* request, UpdateLastModifiedServiceResponse *reply) override {
	std::cout << "\nGot the message ";
	cout<<"\n"<<request->time();
	std::vector<file_list_t>::iterator it; 
	for(it=fileList.begin();it != fileList.end(); it++){
	    if(it->name.compare(request->filename())==0){
		if(it->modification_time<request->time())
			it->modification_time=request->time();
		if(it->size<request->newbytewrote())
			it->size=request->newbytewrote();
	    }
	}
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

