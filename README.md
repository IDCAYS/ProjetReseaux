# ProjetReseaux
Projet Réseaux : Réalisation d'un chat client-serveur pour M1-MIAGE

Lancer ubuntu

Ouvrir 3 terminaux

Pour les 3 : 

cd ..  ( accès /home)
cd ..  ( accès /$ au lieu de vague$)
cd mnt ( besoin d’accès mnt pour accéder aux autres documents )
cd c/Users/julie/Documents/Code

Pour le serveur : 
gcc -o server server_1.c

ensuite ./server ( pour allumer le serv )

2ème terminal (  Client 1 ) : 
gcc -o client client.c

./client 127.0.0.1 ( allumer 1 er client )

3ème terminal ( Client 2 ):

./client 127.0.0.1





PROJET : Communication par chat en langage C

2 clients qui échangent des messages

Fonctionnalitées :

- Message privé à un client en particulier

- Message global à tous les clients connectés

- Listing de tous les clients connectés

- Message en couleur ( petit plus choix de la couleur )

- Quitter le chat

C - Stocker conversation ( petit plus si le client peut garder son propre historique )

J - Créer des canaux de discussion pour que le client puisse s'y connecter ( côté serveur ajouter des canaux et côté client peut aussi créer aussi des canaux )

- ( Peut-être une IHM )
