#include "commons.h"
#include "config.h"
#include "c_mdm.h"
#include "file_server_client.h"

int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}

void 
print_response(file_access_response_t *c_response) {
	cout<<"\n"<<c_response->request_id;
	cout<<"\n"<<c_response->code;
	cout<<"\n"<<c_response->token;
	cout<<"\n";
}
void 
print_request(file_access_request_t *c_req) {
	cout<<"\n"<<c_req->start_byte;
	cout<<"\n"<<c_req->end_byte;
	cout<<"\n"<<c_req->file_name;
	cout<<"\n"<<c_req->req_ipaddr_port;
}
void
print_request(register_service_request_t *c_req) {
        cout<<"\n"<<c_req->type;
        cout<<"\n"<<c_req->ip_port;
}


file_access_response_t* 
extract_response_from_payload(FileAccessResponse Response) {
	file_access_response_t *c_response = new file_access_response_t;
	 if(Response.code() == FileAccessResponse::OK) {
                 c_response->code = OK;
         }
         if(Response.code() == FileAccessResponse::ERROR) {
                 c_response->code = ERROR;
         }
         c_response->request_id = Response.requestid();
         c_response->token = Response.token();
         c_response->start_byte = Response.startbyte();
         c_response->end_byte = Response.endbyte();
         c_response->stripe_width = Response.stripwidth();
	 c_response->create_time=Response.createtime();
	 c_response->last_modified_time=Response.lastupdatetime();
	 c_response->file_size=Response.filesize();
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": server recevied size"<< Response.serverlist_size();
#endif
         for(int i = 0; i< Response.serverlist_size(); i++) {
                 c_response->server_list.push_back(Response.serverlist()[i]);
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":    "<< Response.serverlist()[i];
#endif

         }
	return c_response;
}

register_service_response_t*
extract_response_from_payload(RegisterServiceResponse Response) {
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": extring the register service response ";
#endif
	register_service_response_t *c_response = new register_service_response_t;
	if(Response.code() == RegisterServiceResponse::OK) {
		c_response->code = OK;
	}
	if(Response.code() == RegisterServiceResponse::ERROR) {
		c_response->code = ERROR;
	}

	return c_response;
}

request_type
get_c_type(FileAccessRequest::RequestType type) {
	switch (type) {
		case FileAccessRequest::READ:
			return READ;
		case FileAccessRequest::WRITE:
			return WRITE;
		case FileAccessRequest::CREATE:
			return CREATE;
		case FileAccessRequest::DELETE:
			return DELETE;
		case FileAccessRequest::OPEN:
			return OPEN;
		case FileAccessRequest::FSTAT:
			return FSTAT;
		default:
			cout<<"get_ctype: wrong request type";	
	}

}

FileAccessRequest::RequestType
get_grpc_type (request_type type) {
	switch (type) {
		case READ:
			return FileAccessRequest::READ;
		case WRITE:
			return FileAccessRequest::WRITE;
		case CREATE:
			return FileAccessRequest::CREATE;
		case DELETE:
			return FileAccessRequest::DELETE;
		case OPEN:
			return FileAccessRequest::OPEN;
		case FSTAT:
			return FileAccessRequest::FSTAT;
		default:
			cout<<"get_ctype: wrong request type";
	}
}


void 
make_req_payload (FileAccessRequest *payload, 
		file_access_request_t *req) {

	payload->set_type(get_grpc_type(req->type));
	payload->set_startbyte(req->start_byte);
	payload->set_endbyte(req->end_byte);
	payload->set_requestid(req->request_id);
	payload->set_filename(req->file_name);
	payload->set_reqipaddrport(req->req_ipaddr_port);
	payload->set_stripewidth(req->strip_width);
}

void
make_req_payload (RegisterServiceRequest *payload,
                register_service_request_t *req) {
        if(req->type == CLIENT) {
                payload->set_type(RegisterServiceRequest::CLIENT);
        }
        payload->set_ipport(req->ip_port);
}
		

file_access_response_t* meta_data_manager_client::file_access_request_handler( file_access_request_t *c_req) {
	FileAccessRequest ReqPayload;
	FileAccessResponse Response;
	ClientContext Context;

	file_access_response_t *c_response = NULL;
	make_req_payload(&ReqPayload, c_req);

	print_request(c_req);

	// The actual RPC.
	Status status = stub_->fileAccessRequestHandler(&Context, ReqPayload, &Response);

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

register_service_response_t* meta_data_manager_client::register_service_handler( register_service_request_t *c_req) {

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<":";
#endif
        RegisterServiceRequest ReqPayload;
        RegisterServiceResponse Response;
        ClientContext Context;

        register_service_response_t *c_response = NULL;
        make_req_payload(&ReqPayload, c_req);

//        print_request(c_req);

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

int mm_create_new_file(const char *filename, int stripe_width) {
#ifdef DEBUG_FLAG
                 cout<<__func__ <<": create file function called ";
#endif

	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = 0;
	c_req->end_byte = 0;
	c_req->request_id = get_random_number();
	c_req->file_name = filename;
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = CREATE;
	c_req->strip_width = stripe_width;

	
	c_response = mdm_service->file_access_request_handler(c_req);
#ifdef DEBUG_FLAG
	cout<<"\n Response recieved";
	print_response(c_response);
#endif
	if(c_response->code != OK) {
		cout<< "Error occured, possibly similar file exists";
		delete(c_req);
		delete(c_response);
		return -1;
	} else {
#ifdef DEBUG_FLAG
		cout<<"mM_create_new_file: file created successfully";
#endif	
		/*Create the file record in file*/
		file_info_store *new_file = new file_info_store(filename, stripe_width);
		file_dir[filename] = new_file;
	}
	delete(c_req);
	delete(c_response);
	return 1;
}


int mm_open_file(const char *filename, const char mode)
{
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": open file function";
#endif
	file_info_store *file = NULL;

	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = 0;
	c_req->end_byte = 0;
	c_req->request_id = get_random_number();
	c_req->file_name = filename;
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = OPEN;
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": sent the request";
#endif
	c_response = mdm_service->file_access_request_handler(c_req);
#ifdef DEBUG_FLAG
	cout<<"\n Response recieved";
	print_response(c_response);
#endif

	if(c_response->code != OK) {
		cout<< "Error occured, possibly file doesnt xists";
		delete(c_req);
		delete(c_response);
		return -1;
	} else {
		if(file_dir.find(filename) != file_dir.end()) {
			cout<<"\n file structre already exist";
			auto it = file_dir.find(filename);
			file = it->second;
		} else {
			 file =  new file_info_store(filename, c_response->stripe_width);
		 }
		 
		file->file_name = filename;
		file->global_permission = mode;
		file->status = OPENED;
		file->create_time = c_response->create_time;
		file->last_modified_time = c_response->last_modified_time;
		file->file_size = c_response->file_size;
		file->stripe_width = c_response->stripe_width;
		file->fdis= c_response->fdis;
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": file name : "<<filename;
                 cout<<"\n"<<__func__ <<": server list : "<< c_response->server_list.size();
#endif
		for(int i = 0; i < c_response->server_list.size(); i++) {
			file->server_list.push_back(c_response->server_list[i]);
			
			if(fs_connections.find(c_response->server_list[i]) == fs_connections.end()){
			 //Create a new connection with the file server
				create_connection_with_server(c_response->server_list[i]);
#ifdef DEBUG_FLAG
                 cout<<"\n                    "<< c_response->server_list[i];
#endif
			}
		}

		file_dir[filename] = file;
		fdis_to_filename_map[c_response->fdis] = filename;
	}
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": fdis" <<file->fdis;
                 cout<<"\n"<<__func__ <<": creation time" << file->create_time;
                 cout<<"\n"<<__func__ <<": access time" << file->last_modified_time;
                 cout<<"\n"<<__func__ <<": file status" << file->status;
                 cout<<"\n"<<__func__ <<": file stripe_width" << file->stripe_width;
#endif
	delete(c_req);
	delete(c_response);
	return file->fdis;
}

int mm_get_read_permission (uint32_t fdis, size_t nbyte, off_t offset) {

	if(file_dir.find(fdis_to_filename_map[fdis]) == file_dir.end()) {
		if((file_dir.find(fdis_to_filename_map[fdis])->second)->status < OPENED){
			cout<<"This file is not opened at all";
		}
		return -1;
	}

	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = offset;
	c_req->end_byte = offset + nbyte - 1; /*carefull on -1.. offset 0 and byte 10 then it should be 0 start and end 9*/
	c_req->request_id = get_random_number();
	c_req->file_name = fdis_to_filename_map[fdis];
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = READ;

#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": Permission request sent to MDM";
#endif
	c_response = mdm_service->file_access_request_handler(c_req);

#ifdef DEBUG_FLAG
	cout<<"\n Response recieved";
	print_response(c_response);
#endif

	if(c_response->code != OK) {
		cout<< "Error occured, possibly similar file exists";
		delete(c_req);
		delete(c_response);
		return -1;
	}
	else { 
		/* Store the token in map and add that token in file information*/
		permission access_permission;
		access_permission.type = "r";
		access_permission.start_end = make_pair(c_response->start_byte, c_response->end_byte);
		file_dir[fdis_to_filename_map[fdis]]->access_permission.push_back(access_permission);
		file_status status = file_dir[fdis_to_filename_map[fdis]]->status;
		file_dir[fdis_to_filename_map[fdis]]->status == READING;
#ifdef DEBUG_FLAG
		 for(auto it = file_dir[fdis_to_filename_map[fdis]]->access_permission.begin();it!=file_dir[fdis_to_filename_map[fdis]]->access_permission.end();it++) {
			 cout<<"\n"<<__func__ <<": permission" <<(*it).type;
		 	cout<< "\nstart" <<(*it).start_end.first <<"end"<<(*it).start_end.second;
		 }
                 cout<<"\n"<<__func__ <<": file status" << file_dir[fdis_to_filename_map[fdis]]->status;
#endif
		return 1; }
	return 1;
}

int mm_get_write_permission (uint32_t fdis, size_t nbyte, off_t offset) {

	if(file_dir.find(fdis_to_filename_map[fdis]) == file_dir.end()) {
		if((file_dir.find(fdis_to_filename_map[fdis])->second)->status < OPENED){
			cout<<"This file is not opened at all";
		}
		return -1;
	}

	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = offset;
	c_req->end_byte = offset + nbyte - 1; /*carefull on -1.. offset 0 and byte 10 then it should be 0 start and end 9*/
	c_req->request_id = get_random_number();
	c_req->file_name = fdis_to_filename_map[fdis];
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = WRITE;

	c_response = mdm_service->file_access_request_handler(c_req);

#ifdef DEBUG_FLAG
	cout<<"Response recieved";
	print_response(c_response);
#endif

	if(c_response->code != OK) {
		cout<< "Error occured, possibly similar file exists";
		delete(c_req);
		delete(c_response);
		return -1;
	}
	else { 
		/* Store the token in map and add that token in file information*/
		permission access_permission;
		access_permission.type = "rw";
		access_permission.start_end = make_pair(c_response->start_byte, c_response->end_byte);
		file_dir[fdis_to_filename_map[fdis]]->access_permission.push_back(access_permission);
		file_dir[fdis_to_filename_map[fdis]]->status == WRITING;
#ifdef DEBUG_FLAG
                  for(auto it = file_dir[fdis_to_filename_map[fdis]]->access_permission.begin();it!=file_dir[fdis_to_filename_map[fdis]]->access_permission.end();it++) {
                          cout<<"\n"<<__func__ <<": permission" <<(*it).type;
                         cout<< "\nstart" <<(*it).start_end.first <<"end"<<(*it).start_end.second;
                  }
                  cout<<"\n"<<__func__ <<": file status" << file_dir[fdis_to_filename_map[fdis]]->status;
		  cout<<"\n";
		   cout<<"\n";
			 cout<<"\n";
 #endif
		return 1;
	}
	return 1;
}

int mm_delete_file (const char *filename) {
      
      	file_access_request_t *c_req = new file_access_request_t;
        file_access_response_t *c_response = NULL;

        c_req->start_byte  = 0;
        c_req->end_byte = 0;
        c_req->request_id = get_random_number();
        c_req->file_name = filename;
        c_req->req_ipaddr_port = client_server_ip_port;
        c_req->type = DELETE;

        c_response = mdm_service->file_access_request_handler(c_req);
#ifdef DEBUG_FLAG
        cout<<"Response recieved";
        print_response(c_response);
#endif
        if(c_response->code != OK) {
                cout<< "Error occured, possibly similar file exists";
                delete(c_req);
                delete(c_response);
                return -1;
        } else {
#ifdef DEBUG_FLAG
		cout<< "File is delete request successfull";
#endif
        }
        delete(c_req);
        delete(c_response);
	//for(auto it= file_dir[filename]->server_list.begin();it!=file_dir[filename]->server_list.end();it++)
	//	fs_service->fs_delete_file_from_server(filename,*it);
	


        return 1;
}

int mm_get_fstat(string filename, struct pfs_stat *buf)
{
	if(file_dir.find(filename) != file_dir.end()) {
		 /*if((*(file_dir.find(filename))).status < OPEN) {
                 cout<<"\nThis file is not open";
                 return -1;
                 }*/
	}

	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = 0;
	c_req->end_byte = 0;
	c_req->request_id = get_random_number();
	c_req->file_name = filename;
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = FSTAT;

	c_response = mdm_service->file_access_request_handler(c_req);
#ifdef DEBUG_FLAG
	cout<<"Response recieved";
	print_response(c_response);
#endif

	if(c_response->code != OK) {
		cout<< "Error occured, possibly similar file exists";
		delete(c_req);
		delete(c_response);
		return -1;
	} else {
		file_info_store *file = NULL;

		buf->pst_mtime = c_response->last_modified_time;
		buf->pst_ctime = c_response->create_time;
		buf->pst_size = c_response->file_size;
#ifdef DEBUG_FLAG
                 cout<<"\n"<<__func__ <<": buf->pst_c  "<<buf->pst_ctime;
                 cout<<"\n"<<__func__ <<": last modified time "<<buf->pst_mtime;
                 cout<<"\n"<<__func__ <<":"<< buf->pst_size;
#endif
	}
	delete(c_req);
	delete(c_response);
	return 1;
}

