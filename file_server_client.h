#include "file_server_services.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using FileServer::FileReadWriteRequest;
using FileServer::FileReadWriteResponse;
using FileServer::FileServerService;


class file_server_client {

        private:
                std::unique_ptr<FileServerService::Stub> stub_;
        public:
		map<string, file_server_client*> fs_connections;
                file_server_client(std::shared_ptr<Channel> channel): stub_(FileServerService::NewStub(channel)){};
                fs_read_write_response_t* read_write_request_handler(fs_read_write_request_t *c_req);
		void create_connection_with_server(string ip_port);
		int fs_write_file_to_server(cache_block *cb, int start, int end, string file_server);
		int fs_read_file_to_server(string file_name, char *buf, int start, int end, string file_server);
};

extern file_server_client *fs_service;
FileReadWriteRequest::RequestType 
fs_get_grpc_type (request_type type);

request_type
get_c_type(FileReadWriteRequest::RequestType type);

