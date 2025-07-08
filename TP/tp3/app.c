#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char data_to_write[] = "value=63"; 

    fd = open("/proc/ensea/speed", O_WRONLY);

    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (write(fd, data_to_write, sizeof(data_to_write)) < 0) {
        perror("Error writing to file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    return 0;
}
