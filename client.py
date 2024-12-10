import socket
import threading
import tkinter as tk
from tkinter import scrolledtext
from tkinter import simpledialog

class ChatClient:
    def __init__(self, master):
        self.master = master
        self.master.title("Client de Chat")
        
        # Zone d'affichage des messages
        self.chat_display = scrolledtext.ScrolledText(master, state='disabled', width=50, height=20, wrap='word')
        self.chat_display.grid(row=0, column=0, columnspan=2, padx=10, pady=10)

        # Champ de saisie
        self.entry_message = tk.Entry(master, width=40)
        self.entry_message.grid(row=1, column=0, padx=10, pady=10)
        self.entry_message.bind("<Return>", self.send_message)

        # Bouton d'envoi
        self.send_button = tk.Button(master, text="Envoyer", command=self.send_message)
        self.send_button.grid(row=1, column=1, padx=10, pady=10)

        # Connexion au serveur
        self.sock = None

        self.connect_to_server()

    def connect_to_server(self):
        server_ip = "127.0.0.1"  # Adresse IP du serveur
        server_port = 5001       # Port du serveur
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((server_ip, server_port))

            # Saisie du pseudonyme
            pseudonyme = tk.simpledialog.askstring("Pseudonyme", "Entrez votre pseudonyme:")
            self.sock.sendall(pseudonyme.encode('utf-8'))

            # Lancement du thread pour écouter les messages
            threading.Thread(target=self.receive_messages, daemon=True).start()
        except Exception as e:
            self.show_message(f"Erreur de connexion : {e}")

    def receive_messages(self):
        while True:
            try:
                message = self.sock.recv(1024).decode('utf-8')
                if not message:
                    break
                self.show_message(message)
            except Exception as e:
                self.show_message(f"Erreur lors de la réception des messages : {e}")
                break

    def send_message(self, event=None):
        message = self.entry_message.get()
        if message:
            try:
                self.sock.sendall(message.encode('utf-8'))
                self.entry_message.delete(0, tk.END)
            except Exception as e:
                self.show_message(f"Erreur lors de l'envoi du message : {e}")

    def show_message(self, message):
        self.chat_display.config(state='normal')
        self.chat_display.insert(tk.END, message + "\n")
        self.chat_display.config(state='disabled')
        self.chat_display.yview(tk.END)

# Lancement de l'IHM
if __name__ == "__main__":
    root = tk.Tk()
    app = ChatClient(root)
    root.mainloop()
