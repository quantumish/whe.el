#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "wheelmesg.h"

#define PORT 8070
#define PATH "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-6"
#define BUFSIZE 512

int main() {
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));	
	addr.sin_family = AF_INET; // Specify address family.
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY just 0.0.0.0, machine IP address
	addr.sin_port = htons(PORT); // Specify port.
	bind(s, (struct sockaddr *)&addr, sizeof(addr));
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	int recvlen;
	unsigned char buf[BUFSIZE];

	while (1 == 1) {
		recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		struct wheel_cmd cmd = *(struct wheel_cmd*)buf;
		int fd;
		char val[5] = {0};
		switch (cmd.type) {
		case AUTOCENTER:		   
			fd = open(PATH "/enable_autocenter", O_WRONLY);
			write(fd, cmd.value.autocenter ? "y" : "n", 1);
			break;
		case AUTOCENTER_FORCE:
			fd = open(PATH "/autocenter", O_WRONLY);
			sprintf(val, "%d", cmd.value.autocenter_force);
			write(fd, val, 3);
			break;
		case GAIN:
			fd = open(PATH "/gain", O_WRONLY);
			sprintf(val, "%d", cmd.value.gain);
			write(fd, val, 3);
			break;
		case RANGE:
			fd = open(PATH "/range", O_WRONLY);
			sprintf(val, "%d", cmd.value.range);
			write(fd, val, 4);
			break;
		}
	}
	
}
