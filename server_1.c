#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 5001
#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 50

// Garantir qu'un seul thread accesde au fichier à un momment donné
pthread_mutex_t file_mutex;

struct Client {
    int socket;
    char pseudonyme[MAX_NAME_LENGTH];

};

struct Client clients[MAX_CLIENTS];
int nb_clients = 0;

#define MAX_CHANNEL_NAME_LENGTH 50
#define MAX_CHANNELS 15

struct Channel {
    char name[MAX_CHANNEL_NAME_LENGTH];
    struct Client *clients[MAX_CLIENTS];
    int nb_clients;
};

struct Channel channels[MAX_CHANNELS];
int nb_channels = 0;

void initClients() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].socket = -1;
    }
}
// Affiche les clients déjà connecté
void getConnectedClients(char *connectedClients) {
    strcpy(connectedClients, "Clients connectés : ");
    for (int i = 0; i < nb_clients; ++i) {
        strcat(connectedClients, clients[i].pseudonyme);
        strcat(connectedClients, ", ");
    }
}


void addClient(int socket, const char *pseudonyme) {
    if (nb_clients < MAX_CLIENTS) {
        // Vérifier si le pseudonyme est déjà utilisé
        for (int i = 0; i < nb_clients; ++i) {
            if (strcmp(clients[i].pseudonyme, pseudonyme) == 0) {
                // Le pseudonyme est déjà utilisé, envoyer un message d'erreur au client
                char error_message[1024];
                sprintf(error_message, "Le pseudonyme %s est déjà utilisé. Veuillez en choisir un autre.\n", pseudonyme);
                write(socket, error_message, strlen(error_message));
                close(socket); // Fermer la connexion avec le client
                return;
            }
        }
        
        // Ajouter le nouveau client
        clients[nb_clients].socket = socket;
        strcpy(clients[nb_clients].pseudonyme, pseudonyme);
        nb_clients++;

        // Envoyer un message de connexion à tous les clients existants
        char message[1024];
        sprintf(message, "Le client %s s'est connecté.\n", pseudonyme);
        for (int i = 0; i < nb_clients - 1; ++i) {
            write(clients[i].socket, message, strlen(message));
        }
    } else {
        // Trop de clients connectés, envoyer un message d'erreur au nouveau client
        char error_message[1024] = "Le serveur a atteint sa capacité maximale de clients. Réessayez plus tard.\n";
        write(socket, error_message, strlen(error_message));
        close(socket); // Fermer la connexion avec le client
    }
}


void removeClient(int socket) {
    char pseudonyme[MAX_NAME_LENGTH];
    for (int i = 0; i < nb_clients; ++i) {
        if (clients[i].socket == socket) {
            strcpy(pseudonyme, clients[i].pseudonyme);
            clients[i].socket = -1;

            // Mettre à jour la liste des clients connectés
            updateConnectedClients();

            break;
        }
    }
    // Envoyer un message de déconnexion à tous les clients restants
    char message[1024];
    sprintf(message, "Le client %s s'est déconnecté.\n", pseudonyme);
    for (int i = 0; i < nb_clients; ++i) {
        if (clients[i].socket != -1) {
            write(clients[i].socket, message, strlen(message));
        }
    }
}

void updateConnectedClients() {
    int index = 0;
    for (int i = 0; i < nb_clients; ++i) {
        if (clients[i].socket != -1) {
            clients[index] = clients[i];
            index++;
        }
    }
    nb_clients = index;
}
/*
bool isClientConnected(const char *pseudonyme) {
    for (int i = 0; i < nb_clients; ++i) {
        if (strcmp(clients[i].pseudonyme, pseudonyme) == 0) {
            return true;
        }
    }
    return false;
}*/

void joinChannel(int socket, const char *channel_name, const char *pseudonyme) {
    struct Client *client = NULL;

    // Rechercher le client dans la liste
    for (int i = 0; i < nb_clients; i++) {
        if (clients[i].socket == socket) {
            client = &clients[i];
            break;
        }
    }

    if (client == NULL) return;

    // Vérifier si le canal existe
    struct Channel *channel = NULL;
    for (int i = 0; i < nb_channels; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            channel = &channels[i];
            break;
        }
    }

    // Si le canal n'existe pas, le créer
    if (channel == NULL) {
        if (nb_channels < MAX_CHANNELS) {
            channel = &channels[nb_channels++];
            strcpy(channel->name, channel_name);
            channel->nb_clients = 0;
        } else {
            char error_message[1024];
            sprintf(error_message, "Le nombre maximum de canaux a été atteint.\n");
            write(socket, error_message, strlen(error_message));
            return;
        }
    }

    // Ajouter le client au canal
    if (channel->nb_clients < MAX_CLIENTS) {
        channel->clients[channel->nb_clients++] = client;
        char join_message[1024];
        sprintf(join_message, "Vous avez rejoint le canal %s.\n", channel_name);
        write(socket, join_message, strlen(join_message));
    } else {
        char error_message[1024];
        sprintf(error_message, "Le canal %s est plein.\n", channel_name);
        write(socket, error_message, strlen(error_message));
    }
}

void sendMessageToChannel(struct Channel *channel, const char *message, int sender_socket) {
    for (int i = 0; i < channel->nb_clients; i++) {
        if (channel->clients[i]->socket != sender_socket) {
            write(channel->clients[i]->socket, message, strlen(message));
        }
    }
}

void getAvailableChannels(char *channelList) {
    strcpy(channelList, "Canaux disponibles :\n");
    for (int i = 0; i < nb_channels; i++) {
        strcat(channelList, "- ");
        strcat(channelList, channels[i].name);
        strcat(channelList, "\n");
    }
}

void leaveChannel(int socket) {
    for (int i = 0; i < nb_channels; i++) {
        for (int j = 0; j < channels[i].nb_clients; j++) {
            if (channels[i].clients[j]->socket == socket) {
                // Retirer le client du canal
                for (int k = j; k < channels[i].nb_clients - 1; k++) {
                    channels[i].clients[k] = channels[i].clients[k + 1];
                }
                channels[i].nb_clients--;

                // Envoyer un message de confirmation au client
                char leave_message[1024];
                sprintf(leave_message, "Vous avez quitté le canal %s.\n", channels[i].name);
                write(socket, leave_message, strlen(leave_message));

                // Envoyer un message aux autres clients du canal
                char broadcast_message[1024];
                sprintf(broadcast_message, "Le client %s a quitté le canal.\n", channels[i].clients[j]->pseudonyme);
                for (int l = 0; l < channels[i].nb_clients; l++) {
                    write(channels[i].clients[l]->socket, broadcast_message, strlen(broadcast_message));
                }

                return;
            }
        }
    }

    // Si le client n'était pas dans un canal, envoyer un message d'erreur
    char error_message[1024] = "Vous n'êtes dans aucun canal.\n";
    write(socket, error_message, strlen(error_message));
}

void *handleClient(void *arg) {
    int sock = *(int *)arg;
    char buffer[1024];
    int bytes_received;
    char pseudonyme[MAX_NAME_LENGTH];
    bool pseudonyme_saisi = false;
    bool clientInChannel = false;
       
    while ((bytes_received = read(sock, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_received] = '\0';

        if (!pseudonyme_saisi) {
            strcpy(pseudonyme, buffer);
            addClient(sock, pseudonyme);
            printf("Le client %s s'est connecté.\n", pseudonyme);
            // Message de bienvenue au client qui vient de se connecter
            char bienvenue[1024];
            sprintf(bienvenue, "Bienvenue sur le chat, %s !\n", pseudonyme);
            write(sock, bienvenue, strlen(bienvenue));
            // Sauvegarder le message dans l'historique avec nom et timestamp
            save_message(pseudonyme, buffer);
            printf(". SAVE \n");
            // Envoi de la liste des clients connectés au client qui vient de se connecter
            char liste_clients[1024] = "Clients connectés :\n";
            for (int i = 0; i < nb_clients; ++i) {
                strcat(liste_clients, clients[i].pseudonyme);
                strcat(liste_clients, "\n");
            }
            write(sock, liste_clients, strlen(liste_clients));
            pseudonyme_saisi = true;
            continue;
        }
        if (strcmp(buffer, ".help") == 0) {
            // Envoyer la liste des commandes disponibles au client
            char command_list[1024] = "\033[34m Commandes disponibles :\033[0m \n";
            strcat(command_list, "\033[34m Message privé : \033[0m !pseudo message\n");
            strcat(command_list, "\033[34m Liste des personnes connectées : \033[0m .liste\n");
            strcat(command_list, "\033[34m Quitter le tchat : \033[0m .exit\n");
            strcat(command_list, "\033[34m Rejoindre un canal : \033[0m .join 'nom du channel'\n");
            strcat(command_list, "\033[34m Afficher tous les canaux disponibles : \033[0m .channels\n");
            strcat(command_list, "\033[34m Liste des personnes connectées dans son channel : \033[0m .chanListe\n");
            strcat(command_list, "\033[34m Quitter le canal : \033[0m .leave\n");
            strcat(command_list, "\033[34m Écrire en rouge : \033[0m /message\n");
            write(sock, command_list, strlen(command_list));
        } else if  (buffer[0] == '!') {
            char *dest_pseudonyme = strtok(buffer + 1, " ");
            char *message = strtok(NULL, "\0");
            char messageAEnvoyer[1024 + MAX_NAME_LENGTH + 20];
            sprintf(messageAEnvoyer, "\n%s : %s\n", pseudonyme, message);

            bool destinataire_trouve = false;
            for (int i = 0; i < nb_clients; ++i) {
                if (strcmp(clients[i].pseudonyme, dest_pseudonyme) == 0) {
                    write(clients[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
                    destinataire_trouve = true;
                    printf("Le client %s a envoyé un message privé à %s.\n", pseudonyme, dest_pseudonyme);
                    break;
                }
            }
            if (!destinataire_trouve) {
                printf("Le client %s n'est pas connecté.\n", dest_pseudonyme);
                // Envoyer un message au client actuel indiquant que le destinataire n'est pas connecté
                char message_destinataire_invalide[1024];
                sprintf(message_destinataire_invalide, "Le client %s n'est pas connecté.\n", dest_pseudonyme);
                write(sock, message_destinataire_invalide, strlen(message_destinataire_invalide));
            }

        } else if (strcmp(buffer, ".exit") == 0) {
            char exit_message[1024];
            sprintf(exit_message, "Vous avez quitté le chat.\n");
            write(sock, exit_message, strlen(exit_message));
            removeClient(sock); // Retirez le client du chat
            close(sock); // Fermez la connexion
            return NULL; // Terminez la fonction handleClient
        } else if (strcmp(buffer, ".liste") == 0) {
                char liste_clients[1024] = "Liste des clients connectés :\n";
                for (int i = 0; i < nb_clients; ++i) {
                    strcat(liste_clients, clients[i].pseudonyme);
                    strcat(liste_clients, "\n");
                }
                write(sock, liste_clients, strlen(liste_clients));
        } else if (strncmp(buffer, ".join ", 6) == 0) {
            char *channel_name = buffer + 6;
            joinChannel(sock, channel_name, pseudonyme);
            clientInChannel = true;
        } else if (strcmp(buffer, ".channels") == 0) {
            char channelList[1024];
            getAvailableChannels(channelList);
            write(sock, channelList, strlen(channelList));
        } else if (strcmp(buffer, ".chanListe") == 0) {
            // Rechercher le canal auquel appartient le client
            bool clientFoundInChannel = false;
            char clients_in_channel[1024] = "Clients dans le canal :\n";

            for (int i = 0; i < nb_channels; i++) {
                for (int j = 0; j < channels[i].nb_clients; j++) {
                    if (channels[i].clients[j]->socket == sock) {
                        clientFoundInChannel = true;
                        // Ajouter les pseudonymes des clients connectés au canal
                        for (int k = 0; k < channels[i].nb_clients; k++) {
                            strcat(clients_in_channel, channels[i].clients[k]->pseudonyme);
                            strcat(clients_in_channel, "\n");
                        }
                        break;
                    }
                }
                if (clientFoundInChannel) break;
            }

            if (clientFoundInChannel) {
                write(sock, clients_in_channel, strlen(clients_in_channel));
            } else {
                char error_message[1024] = "Vous n'êtes dans aucun canal.\n";
                write(sock, error_message, strlen(error_message));
            }
        } else if (strcmp(buffer, ".leave") == 0) {
            leaveChannel(sock);
            clientInChannel = false;
        } else {
            char messageAEnvoyer[1024 + MAX_NAME_LENGTH + 20];
            sprintf(messageAEnvoyer, "\n%s : %s\n", pseudonyme, buffer);

            if (clientInChannel == true){
                // Envoie le message aux clients dans le même canal
                for (int i = 0; i < nb_channels; i++) {
                    for (int j = 0; j < channels[i].nb_clients; j++) {
                        if (channels[i].clients[j]->socket == sock) {
                            sendMessageToChannel(&channels[i], messageAEnvoyer, sock);
                            break;
                        }
                    }
                }
            } else {
                write(sock, messageAEnvoyer, strlen(messageAEnvoyer));
                for (int i = 0; i < nb_clients; ++i) {
                    if (clients[i].socket != sock) {
                        write(clients[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
                    }
                }
            }
        }

     // Sauvegarder le message dans l'historique avec nom et timestamp
        save_message(pseudonyme, buffer);
        printf(". SAVE \n");
    }
        /*if (strcmp(buffer, ".exit") == 0) {
            char exit_message[1024];
            sprintf(exit_message, "Vous avez quitté le chat.\n");
            write(sock, exit_message, strlen(exit_message));
            removeClient(sock); // Retirez le client du chat
            close(sock); // Fermez la connexion
            return NULL; // Terminez la fonction handleClient
        }*/

    if (pseudonyme_saisi) {
        printf("Le client %s s'est déconnecté.\n", pseudonyme);
    }
    removeClient(sock);
    close(sock);
    return NULL;
}

int main() {
    initClients();

    int socket_descriptor, nouv_socket_descriptor;
    struct sockaddr_in adresse_locale, adresse_client;
    socklen_t longueur_adresse_courante = sizeof(adresse_client);

    // Mutex pour synchroniser l'écriture dans l'historique
    pthread_mutex_init(&file_mutex, NULL);

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
        perror("Erreur lors de la création de la socket.");
        exit(EXIT_FAILURE);
    }

    memset(&adresse_locale, 0, sizeof(adresse_locale));
    adresse_locale.sin_family = AF_INET;
    adresse_locale.sin_port = htons(PORT);
    adresse_locale.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_descriptor, (struct sockaddr *)&adresse_locale, sizeof(adresse_locale)) == -1) {
        perror("Erreur lors du binding de la socket.");
        close(socket_descriptor);
        exit(EXIT_FAILURE);
    }

    listen(socket_descriptor, 5);
    printf("Serveur en attente de connexions...\n");

    while (1) {
        nouv_socket_descriptor = accept(socket_descriptor, (struct sockaddr *)&adresse_client, &longueur_adresse_courante);
        if (nouv_socket_descriptor == -1) {
            perror("Erreur lors de l'acceptation de la connexion.");
            close(socket_descriptor);
            exit(EXIT_FAILURE);
        }

        pthread_t client_thread;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = nouv_socket_descriptor;

        if (pthread_create(&client_thread, NULL, handleClient, (void *)socket_ptr) != 0) {
            perror("Erreur lors de la création du thread client.");
            close(nouv_socket_descriptor);
            free(socket_ptr);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(socket_descriptor);

        // Fermeture du mutex
    pthread_mutex_destroy(&file_mutex);
    return 0;
}

// Fonction pour sauvegarder un message dans un fichier historique avec nom et timestamp
void save_message(const char *pseudonyme, const char *message) {
    pthread_mutex_lock(&file_mutex);  // Verrouiller l'accès au fichier

    // Obtenir l'heure actuelle
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Ouvrir le fichier et écrire le message formaté
    FILE *file = fopen("historique.txt", "a");
    if (file == NULL) {
        perror("fopen");
        printf("fichier non disponible");
    } else {
        fprintf(file, "[%s] %s : %s\n", timestamp, pseudonyme, message);
        fclose(file);
    }

    pthread_mutex_unlock(&file_mutex);  // Déverrouiller l'accès au fichier
}