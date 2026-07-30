#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

// Configurazione del sistema distribuito
const int portaBase = 5000;
const char* hostSuccessivo = "127.0.0.1";
const int numNodi = 4;

// Struttura del token
struct Token {
    int bilancio;
    int conteggioTransizioni;
};

// Calcola la porta su cui un nodo deve ascoltare in base al suo ID
int getPorta(int idNodo) {
    return portaBase + idNodo;
}

// Stampa il separatore per la lettura dei log
void stampaSeparatore() {
    cout << "\n========================================\n" << endl;
}

// Configura il socket del server e lo mette in ascolto
int inizializzaServer(int porta) {
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("Errore socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configurazione dell'indirizzo del server
    sockaddr_in indirizzo{};
    indirizzo.sin_family = AF_INET;
    indirizzo.sin_addr.s_addr = INADDR_ANY;     // Ascolta su tutte le interfacce
    indirizzo.sin_port = htons(porta);

    // Associazione socket-porta
    if (::bind(server_fd, (sockaddr*)&indirizzo, sizeof(indirizzo)) < 0) {
        perror("Bind fallito"); exit(EXIT_FAILURE);
    }

    // Messa in ascolto del server
    if (::listen(server_fd, 3) < 0) {
        perror("Listen fallito"); exit(EXIT_FAILURE);
    }
    return server_fd;
}

// Aspetta e riceve un token dal nodo precedente
Token riceviToken(int server_fd) {
    sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    int nuovoSocket = ::accept(server_fd, (sockaddr*)&client, &addrlen);

    Token tokenRicevuto;
    ::read(nuovoSocket, &tokenRicevuto, sizeof(tokenRicevuto));
    ::close(nuovoSocket);
    return tokenRicevuto;
}

// Invia il token al nodo successore
void inviaSuccessivo(int idSuccessore, Token& token) {
    sockaddr_in indirizzoSuccessore{};
    indirizzoSuccessore.sin_family = AF_INET;
    indirizzoSuccessore.sin_port = htons(getPorta(idSuccessore));
    inet_pton(AF_INET, hostSuccessivo, &indirizzoSuccessore.sin_addr);

    // Connessione TCP verso il nodo successore
    int socketClient;
    while (true) {
        socketClient = ::socket(AF_INET, SOCK_STREAM, 0);
        if (::connect(socketClient, (sockaddr*)&indirizzoSuccessore, sizeof(indirizzoSuccessore)) == 0) break;

        cout << "ATM" << idSuccessore << " non disponibile. Riprovo..." << endl;
        ::close(socketClient);

        // Ritardo per la visualizzazione dei log
        this_thread::sleep_for(chrono::seconds(1));
    }
    ::send(socketClient, &token, sizeof(token), 0);
    ::close(socketClient);
}

// Sezione critica
void transazione(Token& token, int idCorrente, int operazione, int importo) {
    // Entrata nella sezione critica (solo del nodo che possiede il token)
    cout << "[SEZIONE CRITICA] ATM" << idCorrente << " ENTRA" << endl;

    // Caso DEPOSITO
    if (operazione == 1) {
        token.bilancio += importo;
        cout << "DEPOSITO: " << importo << "€" << endl;
    }

    // Caso PRELIEVO
    else if (operazione == 2 && token.bilancio >= importo) {
        token.bilancio -= importo;
        cout << "PRELIEVO: " << importo << "€" << endl;
    }

    else if (operazione == 2) {
        cout << "Fondi insufficienti!" << endl;
    }

    // Aggiornamento del contatore globale delle transazioni
    token.conteggioTransizioni++;
    cout << "Nuovo saldo: " << token.bilancio << "€" << endl;

    // Uscita dalla sezione critica
    cout << "[SEZIONE CRITICA] ATM" << idCorrente << " ESCE - Transazione completata." << endl;

    // Ritardo per la visualizzazione dei log
    this_thread::sleep_for(chrono::seconds(1));
}

// Transazioni determinate (esempio della traccia)
void eseguiLogica(int id, Token& token) {
    int op = 0, imp = 0;
    if (id == 2) { op = 2; imp = 200; }
    else if (id == 3) { op = 1; imp = 100; }
    else if (id == 4) { op = 2; imp = 500; }

    if (op != 0) transazione(token, id, op, imp);
    else cout << "Nessuna operazione per ATM" << id << endl;
}

// Funzione main
int main(int argc, char* argv[]) {
    // Controllo dei parametri da linea di comando: ogni nodo deve ricevere il proprio ID
    if (argc != 2) { cerr << "Uso: ./atm <id 1-4>" << endl; return -1; }
    int idCorrente = stoi(argv[1]);

    // Calcolo ID del nodo successore (per formare l'anello logico)
    int idSuccessore = (idCorrente % numNodi) + 1;

    // Inizializzazione del server e ricerca della porta del nodo successore
    int server_fd = inizializzaServer(getPorta(idCorrente));

    stampaSeparatore();
    cout << "ATM" << idCorrente << " online. In ascolto..." << endl;
    stampaSeparatore();

    // Log di avvio nodo (solo ATM1)
    if (idCorrente == 1) {
        cout << "Premi INVIO per iniettare il token..." << endl;
        cin.get();
        Token t = {1000, 0};
        inviaSuccessivo(idSuccessore, t);
    }

    // Ciclo TOKEN RING
    while (true) {
        // 1. Ricevi
        Token t = riceviToken(server_fd);
        stampaSeparatore();
        cout << "TOKEN RICEVUTO da ATM" << idCorrente << " (Saldo: " << t.bilancio << "€)" << endl;

        // 2. Elabora
        eseguiLogica(idCorrente, t);

        // 3. Invia
        cout << "Inoltro a ATM" << idSuccessore << "..." << endl;
        inviaSuccessivo(idSuccessore, t);
    }

    return 0;
}