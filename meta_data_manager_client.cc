#include "commons.h"
#include "client_to_meta_manager.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using MetaDataManager::FileAccessRequest;
using MetaDataManager::FileAccessResponse;
using MetaDataManager::MetaDataManagerService;

int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}

void 
print_response(file_access_response_t *c_response) {
	cout<<"\n"<<c_response->request_id;
	cout<<"\n"<<c_response->req_status;
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

file_access_response_t* 
extract_response_from_payload(FileAccessResponse Response) {
	file_access_response_t *c_response = new file_access_response_t;
	c_response->request_id = Response.requestid();
	c_response->req_status = Response.reqstatus();
	c_response->token= Response.token();
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

class meta_data_manager_client {
	public:
		meta_data_manager_client (std::shared_ptr<Channel> channel): stub_(MetaDataManagerService::NewStub(channel))
		 {}
		

		file_access_response_t* file_access_request_handler( file_access_request_t *c_req) {
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
	private:
		std::unique_ptr<MetaDataManagerService::Stub> stub_;


};



int main(int argc, char** argv) {
	meta_data_manager_client file_access_service(grpc::CreateChannel(
				"localhost:50051", grpc::InsecureChannelCredentials()));
	file_access_request_t *c_req = new file_access_request_t;
	file_access_response_t *c_response = NULL;

	c_req->start_byte  = 0;
	c_req->end_byte = 10;
	c_req->request_id = get_random_number();
	c_req->file_name = "veer.c";
	c_req->req_ipaddr_port = "localhost:5001";
	
	c_response = file_access_service.file_access_request_handler(c_req);
	cout<<"Response recieved";
	print_response(c_response);

	return 0;
}

