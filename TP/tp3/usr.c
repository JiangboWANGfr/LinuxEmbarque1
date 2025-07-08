#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd,fd1,fd2;
	char data_speed[100] = {0};
	char data_dir[100] = {0};
	char data_pattern[100] = {0};
	char data_to_write[] = "pattern=3";

    fd = open("/proc/ensea/speed", O_RDONLY);

    if (fd < 0) {
        perror("Error opening file speed");
        exit(EXIT_FAILURE);
    }

    if (read(fd, &data_speed, sizeof(data_speed)) < 0) {
        perror("Error read  file");
        close(fd);
        exit(EXIT_FAILURE);
    }		
    printf("%s\r\n",data_speed);
    close(fd);
	usleep(200000);  // Sleep for 200ms
	
    fd1 = open("/dev/ensea_leds", O_RDWR);
    if (fd1 < 0) {
        perror("Error opening file /dev/led");
        exit(EXIT_FAILURE);
    }
    if (write(fd1, data_to_write, sizeof(data_to_write)) < 0) {
        perror("Error writing to file ");
        close(fd1);
        exit(EXIT_FAILURE);
    }
	if (read(fd1, &data_pattern, sizeof(data_pattern)) < 0) {
        perror("Error read  file");
        close(fd1);
        exit(EXIT_FAILURE);
    }		
    printf("%s\r\n",data_pattern);
    close(fd1);
	usleep(200000);  // Sleep for 200ms
	
	fd2 = open("/proc/ensea/dir", O_RDONLY);

    if (fd2 < 0) {
        perror("Error opening file dir");
        exit(EXIT_FAILURE);
    }

    if (read(fd2, &data_dir, sizeof(data_dir)) < 0) {
        perror("Error read  file");
        close(fd2);
        exit(EXIT_FAILURE);
    }		
    printf("%s\r\n",data_dir);
    close(fd2);
	
    return 0;
}

