#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>

// --- Prototipos de tus archivos auxiliares ---
int connectTCP(const char *host, const char *service);

#define BUFSIZE 1024

bool use_passive_mode = true; 
long long restart_marker = 0;

typedef struct {
    int socket_fd;
    int server_listen_fd;
} DataConn;

// --- Prototipos Locales ---
void readInput(char *buf, size_t size);
void handleServerResponse(int sd, char *out_buf, size_t size);
void do_transfer(int control_sd, const char *command, const char *arg, long long offset);
DataConn establish_data_connection(int control_sd);
int send_port_command(int control_sd, int *listen_sock);
int parse_pasv_response(char *buf, char *ip_out, int *port_out);
const char* get_filename_from_path(const char* path);
void print_help();

// Manejador de señal para limpiar procesos hijos
void child_handler(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    char *srv_add;
    
    struct sigaction sa;
    sa.sa_handler = child_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    sigaction(SIGCHLD, &sa, NULL);

    switch (argc) {
        case 1: srv_add = "localhost"; break;
        case 2: srv_add = argv[1]; break;
        default:
            printf("Usage: %s <server>\n", argv[0]);
            return 1;
    }

    int sd = connectTCP(srv_add, "ftp");
    if (sd < 0) {
        perror("Error conectando al servidor");
        return 1;
    }
    printf("Conectado a %s (fd: %d)\n", srv_add, sd);
    
    char buf[BUFSIZE];
    handleServerResponse(sd, buf, sizeof(buf));

    // --- AUTENTICACIÓN ---
    while(1) {
        printf("USER: ");
        readInput(buf, sizeof(buf));
        dprintf(sd, "USER %s\r\n", buf);
        handleServerResponse(sd, buf, sizeof(buf));

        printf("PASSWORD: ");
        readInput(buf, sizeof(buf));
        dprintf(sd, "PASS %s\r\n", buf);
        handleServerResponse(sd, buf, sizeof(buf));

        if (strncmp(buf, "230", 3) == 0) break;
        printf("Login fallido.\n");
    }

    printf("\n--- Cliente FTP Concurrente ---\n");
    print_help();

    // --- BUCLE PRINCIPAL ---
    while (1) {
        printf("ftp> ");
        fflush(stdout); 
        readInput(buf, sizeof(buf));

        if (strlen(buf) == 0) continue;

        if (strncmp(buf, "quit", 4) == 0 || strncmp(buf, "exit", 4) == 0) {
            dprintf(sd, "QUIT\r\n");
            handleServerResponse(sd, buf, sizeof(buf));
            break;
        }
        else if (strncmp(buf, "help", 4) == 0) {
            print_help();
        }
        else if (strncmp(buf, "passive", 7) == 0) {
            use_passive_mode = !use_passive_mode;
            printf("Passive mode: %s.\n", use_passive_mode ? "on" : "off");
        }
        else if (strncmp(buf, "pwd", 3) == 0) {
            dprintf(sd, "PWD\r\n");
            handleServerResponse(sd, buf, sizeof(buf));
        }
        else if (strncmp(buf, "cd ", 3) == 0) {
            dprintf(sd, "CWD %s\r\n", buf + 3);
            handleServerResponse(sd, buf, sizeof(buf));
        }
        else if (strncmp(buf, "mkdir ", 6) == 0) {
            dprintf(sd, "MKD %s\r\n", buf + 6);
            handleServerResponse(sd, buf, sizeof(buf));
        }
        else if (strncmp(buf, "delete ", 7) == 0) {
            dprintf(sd, "DELE %s\r\n", buf + 7);
            handleServerResponse(sd, buf, sizeof(buf));
        }
        else if (strncmp(buf, "restart ", 8) == 0) {
            restart_marker = atoll(buf + 8);
            printf("Marcador REST establecido en byte %lld\n", restart_marker);
        }
        // --- LS / DIR (Listar archivos) ---
        else if (strncmp(buf, "ls", 2) == 0 || strncmp(buf, "dir", 3) == 0) {
            // Extraer argumentos opcionales (ej: "ls -l" o "ls carpeta")
            char *args = strchr(buf, ' ');
            if (args) args++; // Saltar el espacio
            else args = "";   // Sin argumentos
            
            do_transfer(sd, "LIST", args, 0);
        }
        // --- PUT ---
        else if (strncmp(buf, "put ", 4) == 0) {
            char *path = buf + 4;
            if (access(path, R_OK) != 0) {
                perror("Error: No se encuentra el archivo local");
            } else {
                do_transfer(sd, "STOR", path, restart_marker);
                restart_marker = 0;
            }
        }
        // --- GET ---
        else if (strncmp(buf, "get ", 4) == 0) {
            char *path = buf + 4;
            do_transfer(sd, "RETR", path, restart_marker);
            restart_marker = 0;
        }
        else {
            printf("Comando desconocido. Escriba 'help'.\n");
        }
    }

    close(sd);
    return 0;
}

void print_help() {
    printf("Comandos disponibles:\n");
    printf("  ls / dir         : Listar archivos del directorio remoto\n");
    printf("  get <archivo>    : Descargar archivo\n");
    printf("  put <archivo>    : Subir archivo\n");
    printf("  cd <ruta>        : Cambiar directorio remoto\n");
    printf("  mkdir <nombre>   : Crear directorio remoto\n");
    printf("  delete <archivo> : Borrar archivo remoto\n");
    printf("  pwd              : Ver directorio actual\n");
    printf("  restart <bytes>  : Reiniciar descarga desde byte X\n");
    printf("  passive          : Alternar modo Pasivo (on/off)\n");
    printf("  quit             : Salir\n");
}

const char* get_filename_from_path(const char* path) {
    const char *filename = strrchr(path, '/');
    if (filename) {
        return filename + 1; 
    }
    return path; 
}

// Función genérica para STOR, RETR y LIST
void do_transfer(int control_sd, const char *command, const char *arg, long long offset) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error fork"); return;
    }
    else if (pid > 0) {
        // Padre retorna inmediatamente
        return; 
    }

    // --- PROCESO HIJO ---
    DataConn dc = establish_data_connection(control_sd);
    if (dc.socket_fd < 0) {
        printf("\n[Hijo] Error conexión datos. Terminando.\n");
        exit(1);
    }

    char msg[BUFSIZE];

    // Enviar REST (solo para transferencias de archivos, no para LIST)
    if (offset > 0 && strcmp(command, "LIST") != 0) {
        snprintf(msg, sizeof(msg), "REST %lld\r\n", offset);
        send(control_sd, msg, strlen(msg), 0);
        ssize_t r = recv(control_sd, msg, sizeof(msg)-1, 0);
        if (r > 0) { msg[r] = 0; printf("\n[Hijo REST] %s", msg); }
    }

    // Construir comando: "STOR file", "RETR file" o "LIST [path]"
    if (strlen(arg) > 0)
        snprintf(msg, sizeof(msg), "%s %s\r\n", command, arg);
    else
        snprintf(msg, sizeof(msg), "%s\r\n", command);

    send(control_sd, msg, strlen(msg), 0);

    // Esperar respuesta 150 (Opening data connection)
    ssize_t r = recv(control_sd, msg, sizeof(msg)-1, 0);
    if (r > 0) {
        msg[r] = '\0';
        printf("\n[Hijo Control] %s", msg);
        if (msg[0] != '1') { // Si hay error (ej: 550)
            close(dc.socket_fd);
            exit(1);
        }
    }

    // --- TRANSFERENCIA DE DATOS ---
    
    if (strcmp(command, "STOR") == 0) {
        // SUBIDA
        FILE *fp = fopen(arg, "rb");
        if (fp) {
            if (offset > 0) fseek(fp, offset, SEEK_SET);
            char filebuf[BUFSIZE];
            size_t n;
            while ((n = fread(filebuf, 1, sizeof(filebuf), fp)) > 0) {
                if (send(dc.socket_fd, filebuf, n, 0) < 0) break;
            }
            fclose(fp);
        }
    } 
    else if (strcmp(command, "RETR") == 0) {
        // BAJADA
        const char *local_name = get_filename_from_path(arg);
        FILE *fp;
        if (offset > 0) {
            fp = fopen(local_name, "r+b"); 
            if (fp) fseek(fp, offset, SEEK_SET);
        } else {
            fp = fopen(local_name, "wb");
        }

        if (fp) {
            char filebuf[BUFSIZE];
            ssize_t n;
            while ((n = recv(dc.socket_fd, filebuf, sizeof(filebuf), 0)) > 0) {
                fwrite(filebuf, 1, n, fp);
            }
            fclose(fp);
        } else {
            printf("\n[Hijo Error] Fallo al crear archivo local: %s\n", local_name);
        }
    }
    else if (strcmp(command, "LIST") == 0) {
        // LISTADO (Imprimir a pantalla)
        char filebuf[BUFSIZE];
        ssize_t n;
        // Leemos del socket de datos y escribimos directamente a stdout
        while ((n = recv(dc.socket_fd, filebuf, sizeof(filebuf), 0)) > 0) {
            fwrite(filebuf, 1, n, stdout);
        }
    }

    close(dc.socket_fd);
    if (dc.server_listen_fd != -1) close(dc.server_listen_fd);

    // Esperar respuesta 226 (Transfer complete)
    r = recv(control_sd, msg, sizeof(msg)-1, 0);
    if (r > 0) {
        msg[r] = '\0';
        printf("\n[Hijo Control] %s", msg);
    }

    exit(0);
}

DataConn establish_data_connection(int control_sd) {
    DataConn dc = {-1, -1};
    char buf[BUFSIZE];

    if (use_passive_mode) {
        send(control_sd, "PASV\r\n", 6, 0);
        ssize_t r = recv(control_sd, buf, sizeof(buf)-1, 0);
        if (r <= 0) return dc;
        buf[r] = '\0';
        
        char ip[32];
        int port;
        if (parse_pasv_response(buf, ip, &port)) {
            char portStr[10];
            sprintf(portStr, "%d", port);
            dc.socket_fd = connectTCP(ip, portStr);
        }
    } 
    else {
        int listen_sd = -1;
        if (send_port_command(control_sd, &listen_sd) == 0) {
            dc.server_listen_fd = listen_sd;
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            dc.socket_fd = accept(listen_sd, (struct sockaddr*)&client_addr, &addr_len);
        }
    }
    return dc;
}

int send_port_command(int control_sd, int *listen_sock) {
    struct sockaddr_in local_addr, my_addr;
    socklen_t len = sizeof(local_addr);
    getsockname(control_sd, (struct sockaddr *)&local_addr, &len);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = 0;

    bind(s, (struct sockaddr *)&my_addr, sizeof(my_addr));
    listen(s, 1);

    len = sizeof(my_addr);
    getsockname(s, (struct sockaddr *)&my_addr, &len);
    *listen_sock = s;

    unsigned char *ip = (unsigned char *)&local_addr.sin_addr.s_addr;
    unsigned char *p = (unsigned char *)&my_addr.sin_port;

    char msg[BUFSIZE];
    snprintf(msg, sizeof(msg), "PORT %d,%d,%d,%d,%d,%d\r\n", 
             ip[0], ip[1], ip[2], ip[3], p[0], p[1]);
    
    send(control_sd, msg, strlen(msg), 0);

    char buf[BUFSIZE];
    ssize_t r = recv(control_sd, buf, sizeof(buf)-1, 0);
    if (r > 0 && buf[0] == '2') return 0;
    return -1;
}

int parse_pasv_response(char *buf, char *ip_out, int *port_out) {
    int h1, h2, h3, h4, p1, p2;
    char *start = strchr(buf, '(');
    if (start && sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
        sprintf(ip_out, "%d.%d.%d.%d", h1, h2, h3, h4);
        *port_out = p1 * 256 + p2;
        return 1;
    }
    return 0;
}

void readInput(char *buf, size_t size) {
    if (fgets(buf, size, stdin) != NULL) {
        buf[strcspn(buf, "\r\n")] = '\0';
    } else {
        buf[0] = '\0';
        if (ferror(stdin)) clearerr(stdin); 
    }
}

void handleServerResponse(int sd, char *out_buf, size_t size) {
    ssize_t r = recv(sd, out_buf, size - 1, 0);
    if (r > 0) {
        out_buf[r] = '\0';
        printf("%s", out_buf);
    }
}
