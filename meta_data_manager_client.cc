#include "commons.h"
#include "c_mdm.h"


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
	c_response->request_id = Response.requestid();
//	c_response->req_status = Response.reqstatus();
	c_response->token= Response.token();
	return c_response;
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
make_req_payload (FileAccessRequest *payload, 
		file_access_request_t *req) {
	payload->set_startbyte(req->start_byte);
	payload->set_endbyte(req->end_byte);
	payload->set_requestid(req->request_id);
	payload->set_filename(req->file_name);
	payload->set_reqipaddrport(req->req_ipaddr_port);
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





int create_new_file(const char *filename, int stripe_width) {
	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = 0;
	c_req->end_byte = 10;
	c_req->request_id = get_random_number();
	c_req->file_name = "veer.c";
	c_req->req_ipaddr_port = "localhost:5001";
	
	c_response = mdm_service->file_access_request_handler(c_req);
	cout<<"Response recieved";
	print_response(c_response);

	return 0;
}
void write_file_to_server(cache_block* cb,  int start, int end, string server_ip){
return;
}

