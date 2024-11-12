#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_NAME_LENGTH 50

int socket_descriptor;
bool pseudonyme_envoye = false;
char pseudonyme[MAX_NAME_LENGTH]; // Stockage du pseudonyme du client

void *ecouterServeur(void *arg) {
    for (;;) {
        char buffer_msg[1024];
        int longueur;
        memset(buffer_msg, 0, sizeof(buffer_msg));
        longueur = read(socket_descriptor, buffer_msg, sizeof(buffer_msg));
        if (longueur > 0) {
            buffer_msg[longueur] = '\0';
            // Vérifier si le message reçu est une liste de clients connectés
            if (strncmp(buffer_msg, "Liste des clients connectés :", 30) == 0) {
                printf("\033[1;32mListe des clients connectés :\n%s\033[0m\n", buffer_msg + 30);
            } else {
                printf("%s", buffer_msg);
            }
        }
    }
    return NULL;
}

void on_send_button_clicked(const char *message) {
    if (strcmp(message, ".exit") == 0) {
        if ((write(socket_descriptor, message, strlen(message))) < 0) {
            perror("erreur : impossible d'écrire le message destiné au serveur.");
            exit(1);
        }
        close(socket_descriptor); // Fermez la connexion
        exit(0); // Quittez le programme
    }

    if (!pseudonyme_envoye) {
        printf("Erreur : Le pseudonyme n'a pas été envoyé au serveur.\n");
        return;
    }
    

    if (message[0] == '/') {
        // Mettre le message en rouge et ignorer le "/"
        char formatted_message[1024];
        sprintf(formatted_message, "\033[1;31m%s\033[0m", message + 1); // Ignorer le premier caractère "/"
        if ((write(socket_descriptor, formatted_message, strlen(formatted_message))) < 0) {
            perror("erreur : impossible d'écrire le message destiné au serveur.");
            exit(1);
        }
    } else if (message[0] == '!') {
        // Si le message commence par "!", extraire le pseudonyme et le message à envoyer
        char *dest_pseudonyme = strtok(message + 1, " ");
        char *message_body = strtok(NULL, "\0");
        char formatted_message[1024];
        sprintf(formatted_message, "!%s %s", dest_pseudonyme, message_body);
        if ((write(socket_descriptor, formatted_message, strlen(formatted_message))) < 0) {
            perror("erreur : impossible d'écrire le message destiné au serveur.");
            exit(1);
        }
    } else {
        if ((write(socket_descriptor, message, strlen(message))) < 0) {
            perror("erreur : impossible d'écrire le message destiné au serveur.");
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <adresse-serveur>\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    struct addrinfo hints, *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "5001", &hints, &result) != 0) {
        perror("erreur : impossible de trouver le serveur à partir de son adresse.");
        exit(1);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        socket_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_descriptor == -1)
            continue;

        if (connect(socket_descriptor, rp->ai_addr, rp->ai_addrlen) != -1)
            break;

        close(socket_descriptor);
    }

    if (rp == NULL) {
        perror("erreur : impossible de se connecter au serveur.");
        exit(1);
    }

    freeaddrinfo(result);

    printf("Entrez votre pseudonyme : ");
    fgets(pseudonyme, sizeof(pseudonyme), stdin);
    pseudonyme[strcspn(pseudonyme, "\n")] = '\0'; // Supprimer le caractère de retour à la ligne

    // Envoyer le pseudonyme au serveur
    if (write(socket_descriptor, pseudonyme, strlen(pseudonyme)) == -1) {
        perror("erreur : impossible d'envoyer le pseudonyme au serveur.");
        close(socket_descriptor);
        exit(1);
    }

    pseudonyme_envoye = true; // Marquer le pseudonyme comme envoyé

    pthread_t thread_ecoute;
    pthread_create(&thread_ecoute, NULL, ecouterServeur, NULL);

    char message[1024];
    while (fgets(message, sizeof(message), stdin) != NULL) {
        message[strcspn(message, "\n")] = '\0'; // Remove newline character
        on_send_button_clicked(message);
    }

    close(socket_descriptor);

    return 0;
}
