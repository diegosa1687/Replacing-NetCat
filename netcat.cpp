#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

using namespace std;

void usage () {
    cout << "BHP Net Tool - C++ version" << endl << endl;
    cout << "Usage: ./netcat -t target_host -p port" << endl;
    cout << "   -l --listen                - listen on [host]:[port] for incoming connections" << endl;
    cout << "   -e --execute               - execute the given file upon receiving a connection" << endl;
    cout << "   -c --command               - initialize a command shell" << endl;
    cout << "   -u --upload=destination    - upon receiving connection upload a file and write to [destination]" << endl << endl;
    cout << "Examples:" << endl;
    cout << "   ./netcat -t 192.168.0.1 -p 5555 -l -c" << endl;
    cout << "   ./netcat -t 192.168.0.1 -p 5555 -l -u=C:\\target.exe" << endl;
    cout << "   ./netcat -t 192.168.0.1 -p 5555 -l -e=\"cat /etc/passwd\"" << endl;
    cout << "   echo 'ABCDEFGH' | ./netcat -t 192.168.11.12 -p 135" << endl;

    exit(0);
}

string run_command (string command) {
    string result = "";
    char buffer[BUFFER_SIZE];
    command += " 2>&1";

    // Abrir um fluxo de leitura para o comando usando popen()
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "[*] Exception! Command error.";

    // Ler a saída do comando linha por linha
    while (!feof(pipe))
        if (fgets(buffer, BUFFER_SIZE, pipe) != NULL)
            result += buffer;

    fflush(stdout);
    // Fechar o fluxo
    pclose(pipe);

    return (result.empty() ? "\r" : result);
}

void client_sender (string target, int port) {
    int recv_len = 1;
    char buffer[BUFFER_SIZE];
    string response;

    // criação do socket
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        cerr << "[*] Exception! Socket creating error." << endl;
        exit(1);
    }

    // configurações do endereço do servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(target.c_str());

    // conexão ao servidor
    if (connect(client, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "[*] Exception! Error to connect." << endl;
        close(client);
        exit(1);
    }

    while (true) {
        cout << "<BHP:#> ";
        cin.getline(buffer, BUFFER_SIZE);

        // enviando comando
        if (send(client, buffer, strlen(buffer), 0) == -1) {
            cerr << "[*] Exception! Error to send." << endl;
            break;
        }

        if (strcmp(buffer, "exit") == 0) break;

        memset(buffer, 0, sizeof(buffer)); // limpa o buffer

        // recebendo resposta
        response = "";
        while (recv_len) {
            if (recv(client, buffer, sizeof(buffer), 0) == -1) {
                cerr << "[*] Exception! Error to receive." << endl;
                break;
            }

            recv_len = strlen(buffer);
            response += buffer;

            if (recv_len < 4096) break;
        }

        cout << response << endl;
        memset(buffer, 0, sizeof(buffer)); // limpa o buffer
        
    }

    close(client);
}

void server_loop (string target, int port) {
    char buffer[BUFFER_SIZE];
    string response;

    // criação do socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        std::cerr << "[*] Exception! Socket creating error." << std::endl;
        exit(1);
    }

    // configurações do endereço do servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // vincula o socket ao endereço do servidor
    if (bind(server, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "[*] Exception! Link address and port error." << endl;
        close(server);
        exit(1);
    }

    // coloca o socket em modo de escuta
    if (listen(server, 1) == -1) {
        cerr << "[*] Exception! Listen error." << endl;
        close(server);
        exit(1);
    }

    while (true) {
        int client_socket = accept(server, nullptr, nullptr);
        if (client_socket == -1) {
            cerr << "[*] Exception! Accept connection error." << endl;
            break;
        }

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
                cerr << "[*] Exception! Error to receive." << endl;
                break;
            }

            if (strcmp(buffer, "exit") == 0) break;

            response = run_command(buffer);

            if (send(client_socket, response.c_str(), response.length(), 0) == -1) {
                cerr << "[*] Exception! Error to send." << endl;
                break;
            }
        }
    }
}

int main (int argc, char* argv[]) {
    int opt, port = 0;
    bool listen = false, command = false;
    string execute, upload_destination, target;

    if (argc <= 1) usage();

    while ((opt = getopt(argc, argv, "hle:t:p:cu:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                break;
            case 'l':
                listen = true;
                break;
            case 'e':
                execute = optarg;
                break;
            case 'c':
                command = true;
                break;
            case 'u':
                upload_destination = optarg;
                break;
            case 't':
                target = optarg;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            case '?':
                usage();
                break;
            case ':':
                usage();
                break;
        }
    }

    if (!listen && !target.empty() && 0 < port <= 65535) client_sender(target, port);

    if (listen) server_loop(target, port);

    return 0;
}