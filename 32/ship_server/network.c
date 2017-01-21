/*
	Not sure what IPData is, if its just an IP address or something  more.
	Gets it from ship.ini
*/
void convertIPString (char* IPData, unsigned IPLen, int fromConfig, unsigned char* IPStore )
{
	unsigned p,p2=0,p3=0;			// These are counters, i think
	char convert_buffer[5];

	for (p=0; p<IPLen; p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p];
		else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2>3)
				{
					if (fromConfig)
						printf ("ship.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Press [ENTER] to quit...");
					gets(buff);
					exit (1);
				}
			}
			else
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				if (p2 != 3)
				{
					if (fromConfig)
						printf ("ship.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Press [ENTER] to quit...");
					gets(buff);
					exit (1);
				}
				break;
			}
		}
	}
}

/*
	Mask... maybe that IP mask thing (like 255.255.255.0) that filters
	out IPs.  Not sure why this is needed.  But also gotten from ship.ini.
*/
void convertMask (char* IPData, unsigned IPLen, unsigned short* IPStore )
{
	unsigned p,p2=0,p3=0;
	char convert_buffer[5];

	for (p=0; p<IPLen; p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p]; else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				if (convert_buffer[0] == 42)
					IPStore[p2] = 0x8000;
				else
					IPStore[p2] = atoi (&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2>3)
				{
					printf ("Bad mask encountered in masks.txt...\n");
					memset (&IPStore[0], 0, 8 );
					break;
				}
			}
			else
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				if (p2 != 3)
				{
					printf ("Bad mask encountered in masks.txt...\n");
					memset (&IPStore[0], 0, 8 );
					break;
				}
				break;
			}
		}
	}
}

void tcp_listen (int sockfd)
{
	if (listen(sockfd, 10) < 0)
	{
		debug_perror ("Could not listen for connection");
		debug_perror ("Press [ENTER] to quit...");
		gets(buff);
		exit(1);
	}
}

int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len )
{
	int fd;

	if ((fd = accept (sockfd, client_addr, addr_len)) < 0)
		debug_perror ("Could not accept connection");

	return (fd);
}

int tcp_sock_connect(char* dest_addr, int port)
{
	int fd;
	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if( fd < 0 )
		debug_perror("Could not create socket");
	else
	{

		memset (&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr (dest_addr);
		sa.sin_port = htons((unsigned short) port);

		if (connect(fd, (struct sockaddr*) &sa, sizeof(sa)) < 0)
		{
			debug_perror("Could not make TCP connection");
			return -1;
		}
	}
	return(fd);
}

int tcp_sock_open(struct in_addr ip, int port)
{
	int fd, turn_on_option_flag = 1, rcSockopt;

	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if( fd < 0 ){
		debug_perror ("Could not create socket");
		debug_perror ("Press [ENTER] to quit...");
		gets(buff);
		exit(1);
	} 

	sa.sin_family = AF_INET;
	memcpy((void *)&sa.sin_addr, (void *)&ip, sizeof(struct in_addr));
	sa.sin_port = htons((unsigned short) port);

	/* Reuse port */

	rcSockopt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &turn_on_option_flag, sizeof(turn_on_option_flag));

	/* bind() the socket to the interface */
	if (bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0){
		debug_perror("Could not bind to port");
		debug_perror("Press [ENTER] to quit...");
		gets(buff);
		exit(1);
	}

	return(fd);
}
