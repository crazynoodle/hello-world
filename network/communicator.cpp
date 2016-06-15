#include "../network/communicator.h"

void Communicator::sigPipeIgnore(struct sigaction& action) {
	action.sa_handler = SIG_IGN;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);
    
}
bool Communicator::getLocalIP(char* local_ip) {
    
    /*char host_name[128];
    struct hostent *host_ent;
    gethostname(host_name, sizeof(host_name));
    host_ent = gethostbyname(host_name);
    const char* first_ip = inet_ntoa(*(struct in_addr*)(host_ent->h_addr_list[0]));
    memcpy(local_ip, first_ip, 16);
    return true;*/
    //支持局域网
    int inet_sock;  
    struct ifreq ifr;  
    char ip[32];  
  
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, "eth0");  
    ioctl(inet_sock, SIOCGIFADDR, &ifr);  
    strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  
    memcpy(local_ip, ip, 16);
    return true;
}
