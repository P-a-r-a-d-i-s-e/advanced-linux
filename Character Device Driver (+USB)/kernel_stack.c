#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <sys/ioctl.h>

#include <errno.h>

#define SET_SIZE _IOW('a', 'a', int * )
#define DEVICE_PATH "/dev/int_stack"

char *push = "push";
char *pop = "pop";
char *set_size = "set-size";
char *menu = "menu";
char *exit_app = "exit";

int option_to_int(char *opt) {
    if (strcmp(opt, push) == 0) {
        return 1;
    }
    if (strcmp(opt, pop) == 0) {
        return 2;
    }
    if (strcmp(opt, set_size) == 0) {
        return 3;
    }
    if (strcmp(opt, menu) == 0) {
        return 4;
    }
    if (strcmp(opt, exit_app) == 0) {
        return 5;
    }

    return -1;
}

void print_menu(void) {
    printf("****Please Enter the Option******\n");
    printf("        push N               \n");
    printf("        pop                 \n");
    printf("        set-size N              \n");
    printf("        menu              \n");
    printf("        exit                 \n");
    printf("*********************************\n");
}

int check_device_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

int main() {
    int fd;
    char buff[50];
    char option[10];
    int int_option;
    int value;
    int sscanf_result;

    while (1) {
        if (!check_device_exists(DEVICE_PATH)) {
            printf("error: USB key is not inserted\n");
            sleep(5);
        } else {
            fd = open(DEVICE_PATH, O_RDWR);
            if (fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
            }
            break;
        }
    }

    print_menu();

    while (1) {
        if (!fgets(buff, 50, stdin)) {
            printf("Fatal Error!\n");
            return 1;
        }
        sscanf_result = sscanf(buff, "%s %d", option, &value);
        int_option = option_to_int(option);

        if (!check_device_exists(DEVICE_PATH)) {
            printf("Device removed! Closing file descriptor...\n");
            close(fd);
            exit(1);
        }

        switch (int_option) {
        case 1:
            if (sscanf_result != 2) {
                printf("Please enter a value\n");
            } else {
                if (write(fd, &value, sizeof(int)) == -1 && errno == ERANGE) {
                    printf("ERROR: stack is full\n");
                    printf("%d\t#-ERANGE errno code\n", -errno);
                }
            }
            break;
        case 2:
            if (read(fd, &value, sizeof(int)) == -1) {
                printf("NULL\n");
            } else {
                printf("%d\n", value);
            }
            break;
        case 3:
            if (sscanf_result != 2) {
                printf("Please enter a value\n");
            } else {
                if (ioctl(fd, SET_SIZE, (int *) &value) == -1 && errno == EINVAL) {
                    printf("ERROR: size should be > %d \n", value);
                    printf("%d\t#-EINVAL errno code\n", -errno);
                }
            }
            break;
        case 4:
            print_menu();
            break;
        case 5:
            close(fd);
            exit(1);
            break;
        default:
            printf("Enter Valid option\n");
            break;
        }
    }
    close(fd);
}
