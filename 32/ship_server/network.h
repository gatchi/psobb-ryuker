#define TCP_BUFFER_SIZE 4096

/*
	The ol' PSO packets.
	Generates text from them and then puts them in a buffer.
*/
void packet_to_text ( unsigned char* buf, int len );

/*
	Prints the packet to terminal but not without
	generating text from the packet first.
*/
void display_packet ( unsigned char* buf, int len );

/*
	Not sure what IPData is, if its just an IP address or something  more.
	Gets it from ship.ini
*/
void convertIPString (char* IPData, unsigned IPLen, int fromConfig, unsigned char* IPStore );

/*
	Mask... maybe that IP mask thing (like 255.255.255.0) that filters
	out IPs.  Not sure why this is needed.  But also gotten from ship.ini.
*/
void convertMask (char* IPData, unsigned IPLen, unsigned short* IPStore );

void tcp_listen (int sockfd);
int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len );
int tcp_sock_connect(char* dest_addr, int port);
int tcp_sock_open(struct in_addr ip, int port);
