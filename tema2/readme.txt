Tema 2 PC,2020-2021
Saceleanu Andrei Iulian,321CB


Client TCP/subscriber
Un client multiplexeaza intre a primi comenzi de la stdin si mesaje de la
server.
Comenzile disponibile sunt exit,subscribe si unsubscribe.Pentru subscribe/
unsubscribe,mesajele transmise la server respecta formatul/protocolul dat de
de structura to_server_msg_t.
Mesajele primite de la server respecta formatul/protocolul dat de structura
to_client_msg_t.Clientul afiseaza informatiile relevante din acesta.

Server
Server-ul contine logica de gestionare a mesajelor primite de la clientii
UDP si care trebuie transmise la abonatii TCP.
Server-ul mentine o stare locala pentru fiecare client care s-a conectat
cel putin o data,acesta fiind prezent,in functie de status,in map-urile 
online sau offline(nu simultan).Pentru a face posibila interactiunea cu
clientii,exista 2 map-uri de corespondenta intre ID-urile furnizate de clienti
si socketii asociati.
Dupa initializarile aferente ale socketilor,server-ul multiplexeaza IO:

-daca descriptorul 0 este setat,s-a primit o comanda de la stdin;doar comanda
exit este disponibila la server.

-daca descriptorul asociat clientilor UDP este setat,s-a primit un mesaj de la 
un publisher;daca parsarea de la un protocol la altul se realizeaza cu succes,
pentru fiecare client online si abonat la topicul mesajului,se trimite noul mesaj;
pentru fiecare client offline si abonat la topic,se retine o copie a mesajului
intr-un buffer(pentru fiecare client se mentine un set de buffere indexate dupa
topic,precum si un map care asociaza fiecarui topic starea flagului SF).

-daca descriptorul asociat clientilor TCP este setat,s-a primit o noua cerere de
conexiune;daca exista un client intre cei online cu acelasi id,noua conexiune este
respinsa si inchisa;daca clientul apare in setul offline(a mai fost conectat anterior),
se trimit mesajele din bufferele asociate,daca exista;la o conexiune acceptata cu succes,
clientul trece din setul offline in cel online si se afiseaza mesajul de New client.

-orice alt descriptor setat semnifica o comanda de la un client TCP;daca numarul de
octeti receptionati este nul,atunci clientul a inchis conexiunea iar server-ul actualizeaza
max_fd si structurile de stare locala;altfel,s-a primit o comanda de subscribe/unsubscribe,
astfel ca structura ce modeleaza clientul curent retine modificarile cerute.

-la final,se inchid socketii inca utilizati.

-orice actiune nefinalizata cu succes este insotita de terminarea abrupta a programului
si afisarea erorii la stderr,prin intermediul macro-ului DIE.

**toate protocoalele/structurile de modelare ale mesajelor si clientilor se regasesc in
fisierul helpers.h.