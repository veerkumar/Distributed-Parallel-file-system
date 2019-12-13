#include "client_server_services.grpc.pb.h"

using grpc::Channel;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using ClientServer::FilePermissionRevokeRequest;
using ClientServer::FilePermissionRevokeResponse;
using ClientServer::ClientServerService;

#define INTERFACE "wlp3s0"
extern string client_server_ip_port ;
extern string ipAddress;
extern int port;

void start_client_server();
