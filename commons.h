#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <stdint.h>

#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
//#include <grpcpp/security/credentials.h>

using namespace std;

typedef struct file_access_request_ {
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

