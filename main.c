#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define FALSE 0
#define TRUE 1
#define EXIT 2

#define SOC_PATH "../socket"
#define MAX_CONNECTIONS 1
#define BUFFER_SIZE 1024

#define CMD_TIME "get_time\n"
#define CMD_EXIT "exit\n"


int create_soc(int* m_socket) {
    printf("Creating socket...\n");
    *m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_socket < 0) {
        perror("Server: Create");
        return FALSE;
    }
    return TRUE;
}

int bind_soc(const int* m_socket, struct sockaddr_un *m_addr) {
    printf("Binding socket...\n");
    unlink(SOC_PATH);
    if (bind(*m_socket, (struct sockaddr*)(m_addr), sizeof(*m_addr)) < 0) {
        perror("Server: Bind");
        return FALSE;
    }
    return TRUE;
}

int listen_soc(const int* m_socket) {
    printf("Listening socket...\n");
    if (listen(*m_socket, MAX_CONNECTIONS) == -1) {
        perror("Server: Listen");
        return FALSE;
    }
    return TRUE;
}

int accept_soc(int* m_socket, struct sockaddr_un *m_addr) {
    printf("Accepting socket...\n");
    int len;
    *m_socket = accept(*m_socket, (struct sockaddr*)(m_addr), (socklen_t*)(&len));
    if (*m_socket == -1) {
        perror("Server: Accept");
        return FALSE;
    }
    return TRUE;
}

int establishConnection(int* m_socket) {
    struct sockaddr_un m_addr;
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    strcpy(m_addr.sun_path, SOC_PATH);
    memcpy(m_addr.sun_path, SOC_PATH, sizeof(m_addr.sun_path));

    if (create_soc(m_socket) == FALSE) {
        return FALSE;
    }
    if (bind_soc(m_socket, &m_addr) == FALSE) {
        return FALSE;
    }
    if (listen_soc(m_socket) == FALSE) {
        return FALSE;
    }
    if (accept_soc(m_socket, &m_addr) == FALSE) {
        return FALSE;
    }

    printf("Connection established!\n");
    return TRUE;
}

int receive(int* m_socket, char* buf) {
    int res = recv(*m_socket, buf, BUFFER_SIZE, 0);

    if (res == -1 || buf[0] == 0) {
        close(*m_socket);
        printf("Connection lost. Trying to reconnect...\n");

        if (establishConnection(m_socket) == FALSE) {
            perror("Establishing connection: ");
            return FALSE;
        }
        return FALSE;
    }
    return TRUE;
}

int response(const int* m_socket, char* buf) {
    int res = -1;

    if (strcmp(buf, CMD_TIME) == 0) {
        time_t raw_time;
        struct tm *info;
        time(&raw_time);
        info = localtime(&raw_time);
        res = send(*m_socket, asctime(info), BUFFER_SIZE, MSG_NOSIGNAL);
    }
    else if (strcmp(buf, CMD_EXIT) == 0) {
        printf("Exit command received\n");
        return EXIT;
    }
    else {
        res = send(*m_socket, "ERROR: No such command\n", BUFFER_SIZE, MSG_NOSIGNAL);
    }

    if (res < 0) {
        return FALSE;
    }
    return TRUE;
}


int main() {
    int m_socket = -1;
    if (establishConnection(&m_socket) == FALSE) {
        perror("Establishing connection: ");
        return 1;
    }

    char buf [BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);

    while (TRUE) {
        if (receive(&m_socket, &buf) == TRUE) {
            int res = response(&m_socket, &buf);
            if (res == FALSE) {
                perror("Sending answer to client: ");
                return FALSE;
            }
            else if (res == EXIT) {
                break;
            }
        }
    }

    close(m_socket);
    return TRUE;
}
