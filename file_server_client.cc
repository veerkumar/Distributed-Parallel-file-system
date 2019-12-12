#include "commons.h"
#include "config.h"
#include "c_mdm.h"
#include "file_server_client.h"

file_server_client *fs_service;
map<string, file_server_client*> fs_connections;

void create_connection_with_server(string ip_port) {
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": Creating new connection with file server :"<<ip_port;
#endif
       fs_connections[ip_port] =  new file_server_client(grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
 }

void 
print_response(fs_read_write_response_t *c_response) {
	cout<<"\n"<<c_response->request_id;
	cout<<"\n"<<c_response->code;
	cout<<"\n"<<c_response->data;
	cout<<"\n";
}
void 
print_request(fs_read_write_request_t *c_req) {
	cout<<"\n"<<c_req->start_byte;
	cout<<"\n"<<c_req->end_byte;
	cout<<"\n"<<c_req->file_name;
	cout<<"\n"<<c_req->req_ipaddr_port;
}

fs_read_write_response_t* 
extract_response_from_payload(FileReadWriteResponse Response) {
	fs_read_write_response_t *c_response = new fs_read_write_response_t;
	 if(Response.reqstatus() == FileReadWriteResponse::OK) {
                 c_response->code = OK;
         }
         if(Response.reqstatus() == FileReadWriteResponse::ERROR) {
                 c_response->code = ERROR;
         }
         c_response->request_id = Response.requestid();
	 c_response->data = new char[Response.data().size()+1] ;
	 strncpy((c_response->data),Response.data().c_str(),Response.data().size());
	 c_response->size = Response.data().size();
	 return c_response;
}

request_type
get_c_type(FileReadWriteRequest::RequestType type) {
	switch (type) {
		case FileReadWriteRequest::READ:
			return READ;
		case FileReadWriteRequest::WRITE:
			return WRITE;
		case FileReadWriteRequest::DELETE:
			return DELETE;
		default:
			cout<<"get_ctype: wrong request type";	
	}

}

FileReadWriteRequest::RequestType
fs_get_grpc_type (request_type type) {
	switch (type) {
		case READ:
			return FileReadWriteRequest::READ;
		case WRITE:
			return FileReadWriteRequest::WRITE;
		case DELETE:
			return FileReadWriteRequest::DELETE;
		default:
			cout<<"get_ctype: wrong request type";
	}
}


void 
make_req_payload (FileReadWriteRequest *payload, 
		fs_read_write_request_t *req) {

	payload->set_type(fs_get_grpc_type(req->type));
	payload->set_startbyte(req->start_byte);
	payload->set_endbyte(req->end_byte);
	payload->set_requestid(req->request_id);
	payload->set_filename(req->file_name);
	payload->set_reqipaddrport(req->req_ipaddr_port);
	payload->set_data(req->data, req->end_byte-req->start_byte);
	payload->set_stripwidth(req->strip_width);
}


fs_read_write_response_t* file_server_client::read_write_request_handler(fs_read_write_request_t *c_req) {
	FileReadWriteRequest ReqPayload;
	FileReadWriteResponse Response;
	ClientContext Context;

	fs_read_write_response_t *c_response = NULL;
	make_req_payload(&ReqPayload, c_req);

	print_request(c_req);

	// The actual RPC.
	Status status = stub_->fileReadWriteRequestHandler(&Context, ReqPayload, &Response);

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
int file_server_client::fs_write_file_to_server( cache_block *cb, int start, int end, string file_server) {
	fs_read_write_request_t *c_req = new fs_read_write_request_t;
	fs_read_write_response_t *c_response = NULL;
	//char *data = new char

	c_req->start_byte  = start;
	c_req->end_byte = end;
	c_req->request_id = get_random_number();
	c_req->file_name = cb->file_name;
	c_req->req_ipaddr_port = client_server_ip_port;
	c_req->type = WRITE;
	c_req->strip_width = file_dir[cb->file_name]->stripe_width;
	c_req->data =  new char[end-start+1];
	memcpy(c_req->data, cb->data, end-start+1);
	
	c_response = (fs_connections[file_server])->read_write_request_handler(c_req);

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
	
	//TODO	handle the Response;		
	}
	delete(c_req);
	delete(c_response);
	return 1;
}
int file_server_client:: fs_read_file_to_server(string file_name, char *buf, int start, int end, string file_server) {
	fs_read_write_request_t *c_req = new fs_read_write_request_t;
        fs_read_write_response_t *c_response = NULL;
        //char *data = new char

        c_req->start_byte  = start;
        c_req->end_byte = end;
        c_req->request_id = get_random_number();
        c_req->file_name = file_name;
        c_req->req_ipaddr_port = client_server_ip_port;
        c_req->type = READ;
	c_req->strip_width = file_dir[file_name]->stripe_width;

        c_response = (fs_connections[file_server])->read_write_request_handler(c_req);

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

	     memcpy(buf, c_response->data, start-end+1);
        }
        delete(c_req);
        delete(c_response);
        return start-end+1;
}
