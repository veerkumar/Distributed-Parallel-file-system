#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <stdint.h>
#include <time.h>
#include <mutex>
#include <fstream>
#include <utility>
#include <cstddef>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
//#include <grpcpp/security/credentials.h>

using namespace std;
enum request_type {
	READ = 0,
	WRITE = 1,
	CREATE = 2
};

enum return_code {
            OK = 0,
            ERROR = 1
}; 

typedef struct file_access_request_ {
  request_type type;
  uint32_t start_byte;
  uint32_t end_byte;
  int request_id;
  string req_ipaddr_port;
  string file_name;
} file_access_request_t;

typedef struct  file_access_response_ {
  int request_id ;
  string req_status;
  string token;
} file_access_response_t;

enum service_type {
	CLIENT = 0,
	FILE_SERVER = 1
};

typedef struct register_service_request_ {
	service_type type;
	string ip_port;
} register_service_request_t;

typedef struct response_ {
	return_code code;
} register_service_response_t;
