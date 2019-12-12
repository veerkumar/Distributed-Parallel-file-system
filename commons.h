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
	CREATE = 2,
	DELETE = 3,
	OPEN = 4,
	FSTAT = 5
};

enum return_code {
            OK = 0,
            ERROR = 1
};
enum file_status {
        NONE = 0,
        CREATED = 1,
        OPENED = 2,
        WRITING = 3,
        WRITTEN = 4,
        READING = 5,
        READ_ = 6,
        CLOSED = 7
};

enum service_type {
	CLIENT = 0,
	FILE_SERVER = 1
};

typedef struct file_access_request_ {
  request_type type;
  uint32_t start_byte;
  uint32_t end_byte;
  int request_id;
  string file_name;
  string req_ipaddr_port;
  uint32_t strip_width;
  uint64_t fdis ;
} file_access_request_t;

typedef struct  file_access_response_ {
  int request_id ;
  return_code code;
  string token;
  uint32_t start_byte;
  uint32_t end_byte;
  uint64_t fdis ; 
  uint32_t create_time;
  uint32_t last_modified_time;
  uint32_t file_size;
  int stripe_width;
  vector<string> server_list;
} file_access_response_t;

typedef struct fs_read_write_request_ {
  request_type type;
  uint32_t start_byte;
  uint32_t end_byte;
  int request_id;
  string file_name;
  string req_ipaddr_port;
  char *data;
  int strip_width;
} fs_read_write_request_t;

typedef struct  fs_read_write_response_ {
  int request_id ;
  return_code code;
  int size ;
  char *data;
} fs_read_write_response_t;


typedef struct register_service_request_ {
	service_type type;
	string ip_port;
} register_service_request_t;

typedef struct response_ {
	return_code code;
} register_service_response_t;

typedef struct permission_ {
	string type;
	pair<int,int> start_end;
} permission;

typedef struct file_store_ {
        string file_name;
        string global_permission; /*Permission with which this file was opened*/
	file_status status;
	uint32_t create_time;
	uint32_t last_modified_time;
	uint32_t file_size;
	int stripe_width;
	int fdis;
        vector<string> server_list;
	vector<permission> access_permission;

	file_store_(string filename, int stripeWidth) {
		file_name = filename;
		global_permission ="";
		status = CREATED;
		create_time = 0;
		last_modified_time = 0;
		file_size = 0;
		stripe_width = stripeWidth;
		fdis = 0;
	}

} file_info_store;

extern int get_random_number ();

/*
New Data Structs
*/

enum revoke_type {
	READ_REVOKE = 0,
	WRITE_REVOKE = 1,
	DELETE_REVOKE = 2
};

enum revoke_code {
            WHOLE = 0,
            PARTIAL = 1
};

typedef struct revoke_access_request_ {
  revoke_type type;
  uint32_t start_byte;
  uint32_t end_byte;
  int request_id;
  string file_name;
  string token;
} revoke_access_request_t;

typedef struct  revoke_access_response_ {
  int request_id ;
  revoke_code code;
} revoke_access_response_t;




typedef struct update_modified_access_request_ {
  string file_name;
  uint64_t modified_time;
  int last_address;
} update_modified_access_request_t;

typedef struct  update_modified_access_response_ {
  return_code code;
} update_modified_access_response_t;

