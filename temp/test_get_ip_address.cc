#include "../commons.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int port;
string getIPAddress(){
    string ipAddress="Unable to get IP Address";
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while(temp_addr != NULL) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if(strcmp(temp_addr->ifa_name, "ens33")==0){
                    ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
               	cout<<"\n"<<ipAddress;
		 }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
	srand (time(NULL));
        
        /* generate secret number between 5000 to 65000: */
        port = rand() % 60000 + 5000;
        ipAddress.append(":");
        ipAddress.append(to_string(port));
	cout<<"\n";
    return ipAddress;
}

int main(){

cout<<" " << getIPAddress() <<"\n";
//auto start = std::chrono::system_clock::now();
cout<<time(0)<<"\n";
}
