#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


int main() {
    int fd ;
	char data_time[100] = {0};

    fd = open("/dev/ensea_afficheur", O_RDONLY);

    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (read(fd, &data_time, sizeof(data_time)) < 0) {
        perror("Error read  file");
        close(fd);
        exit(EXIT_FAILURE);
    }		
    printf("%s\r\n",data_time);
    close(fd);
}


