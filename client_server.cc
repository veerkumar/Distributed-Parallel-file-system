/*
 *
 * Author: Pramod Kumar(veer) & Soheil 
 *
 * Description:  Client server.cc
 * It is a thread in every client process  and always listen for metadatamanger permission revoke request. 
 *  It is thread safe
 *
 */
#include "commons.h"
#include "cs_mdm.h"
#include "c_mdm.h"

string ipAddress;
int port;
string client_server_ip_port ;



class client_server_service_impl : public ClientServerService::Service {

        Status fileRevokePermissionRequestHandler (ServerContext* context,const  FilePermissionRevokeRequest* request, FilePermissionRevokeResponse* reply) override {
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":";
            std::cout << "\n Got permission invoke message ";
	    cout<<"\n  Start byte: "<<request->startbyte();
	    cout<<"\n  End byte: "<<request->endbyte();
	    cout<<"\n  Request id: "<<request->requestid();
	    cout<<"\n  File name: "<<request->filename();
#endif
	    permission per1,per2;
	    int partial=0;
	    reply->set_requestid(request->requestid());
	    file_info_store *file = file_dir[request->filename()];
            

	    
	    if (request->type() == FilePermissionRevokeRequest::WRITE) {
		if(file->status!=CLOSED){
		    for(auto it = file->access_permission.begin(); it!=file->access_permission.end();it++) {
			if((*it).type.find("w")!=string::npos){
			    partial=1;
			    pair<int,int> s_e = (*it).start_end;
		            if(s_e.first==request->startbyte() && s_e.second==request->endbyte()){
				file->access_permission.erase(it);
			    }else if(s_e.first<request->startbyte() && s_e.second==request->endbyte()){
				file->access_permission.erase(it);
				per1.start_end=make_pair (s_e.first,request->startbyte()-1);
	    			per1.type="rw";
				file->access_permission.push_back(per1);	
			    }else if(s_e.first==request->startbyte() && s_e.second>request->endbyte()){
				file->access_permission.erase(it);
				per2.start_end=make_pair (request->endbyte()+1,s_e.second);
	    			per2.type="rw";
				file->access_permission.push_back(per2);
			    }else{
				file->access_permission.erase(it);
				per1.start_end=make_pair (s_e.first,request->startbyte()-1);
	    			per1.type="rw";
				file->access_permission.push_back(per1);
				per2.start_end=make_pair (request->endbyte()+1,s_e.second);
	    			per2.type="rw";
				file->access_permission.push_back(per2);
			    }
			    break;
		        }
                    }
		    if(partial)
		    	reply->set_code(FilePermissionRevokeResponse::PARTIAL);
		    else
		    	reply->set_code(FilePermissionRevokeResponse::WHOLE);
		}else{
		    reply->set_code(FilePermissionRevokeResponse::WHOLE);
		}
		   
	    }else if (request->type() == FilePermissionRevokeRequest::READ) {
		if(file->status!=CLOSED){
		    for(auto it = file->access_permission.begin(); it!=file->access_permission.end();it++) {
			if((*it).type.find("w")!=string::npos){
			    pair<int,int> s_e = (*it).start_end;
			    partial=1;
		            if(s_e.first==request->startbyte() && s_e.second==request->endbyte()){
				file->access_permission.erase(it);
			    }else if(s_e.first<request->startbyte() && s_e.second==request->endbyte()){
				file->access_permission.erase(it);
				per1.start_end=make_pair (s_e.first,request->startbyte()-1);
	    			per1.type="r";
				file->access_permission.push_back(per1);	
			    }else if(s_e.first==request->startbyte() && s_e.second>request->endbyte()){
				file->access_permission.erase(it);
				per2.start_end=make_pair (request->endbyte()+1,s_e.second);
	    			per2.type="r";
				file->access_permission.push_back(per2);
			    }else{
				file->access_permission.erase(it);
				per1.start_end=make_pair (s_e.first,request->startbyte()-1);
	    			per1.type="r";
				file->access_permission.push_back(per1);
				per2.start_end=make_pair (request->endbyte()+1,s_e.second);
	    			per2.type="r";
				file->access_permission.push_back(per2);
			    }
			    break;
		        }
                    }
		    if(partial)
		    	reply->set_code(FilePermissionRevokeResponse::PARTIAL);
		    else
		    	reply->set_code(FilePermissionRevokeResponse::WHOLE);
		}else{
		    reply->set_code(FilePermissionRevokeResponse::WHOLE);

		}   
	    }else{
		do{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}while(file->status==READING || file->status==WRITING);
		file_dir.erase(request->filename());
            }
            return Status::OK;
    }


};

void start_client_server(){
	/* Start fileserver */
	client_server_service_impl cs_server;
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": " <<client_server_ip_port;
#endif
	ServerBuilder builder;

	builder.AddListeningPort(client_server_ip_port, grpc::InsecureServerCredentials());
	builder.RegisterService(&cs_server);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout <<__func__<< "\nClient Server listening on " << client_server_ip_port << std::endl;
	server->Wait();

}

