**Nume: Croitoru Constantin-Bogdan**
**Grupa: 314CA**

## Tema2

Am implementat functii in 2 fisiere **"server"** si **"load_balancer"**.

#a)Functiile din "server" sunt:
Structura **server_memory** are in interior un pointer de tip "hashtable_t".

**1.init_server_memory**
Aceasta aloca memorie pentru o structura de tip "memory_sensor", creeaza 
serverul din aceasta cu ajutorul functie "ht_create" si returneaza structura.

**2.server_store**
Aceasta adauga in server produsul care are cheia key, folosindu-se de functia 
"ht_put" de la hashtable.

**3.server_retrieve**
Aceasta returneaza valoarea produsului cu cheia "key". Pentru realizarea 
acesteia se foloseste functia "ht_get".

**4.server_remove**
Se elimina produsul cu cheia key din serverul indicat, folosindu-se functia 
"ht_remove_entry".

**5.free_server_memory**
Se sterge serverul din meorie cu ajutorul functiei "ht_free", si se elibereaza 
memoria pentru structura.


#b)Functiile din "load_balancer" sunt :
Structura **load_balancer** are 5 campuri:
-**1)size_max** care reprezinta campul in functie de care alocam si realocam, 
initial acesta este initializat cu 2, iar apoi acesta se dubleaza mereu.
-**2)size** reprezinta numarul de servere retinute in memorie.
-**3)vect_ser** este un vector de pointeri.
-**4)hash_ring** retine id-ul serverului si al replicilor, care au elementele 
retinute pe acelasi server.
-**5)poz** este un vector care retine pozitia cum sunt retinute in vectorul 
vect_ser.

**1.init_load_balancer**
Aceasta aloca memorie pentru structura de tip "load_balancer" si initializeaza 
datele necesare.

**2.loader_add_server**
Aceasta adauga in memorie un server. Se verifica daca este alocat sau nu, iar 
daca s-a atins dimensiunea maxima "size_max" isi dubleaza valoarea si dam 
realloc. Se initializea vectorul "hash_ring" cu id-ul fiecarui server, iar poz 
cu pozitiile serverului si al replicilor, toate avand aceeasi pozitie deoarece 
sunt retinute pe acelasi server. Dupa ce se adauga un server, se ordoneaza 
vectorul "hash_ring" dupa hash cu ajutorul functiei "ordering_hash_ring", iar 
apoi sunt redistribuie produsele cu ajutorul functiei "move_prod_server".
Funtia "ordering_hash_ring" ordoneaza valorile din hash_ring dupa hash-ul 
acestora, iar daca 2 valori au acelasi hash le ordoneaza crescator dupa valoare.

Functia "move_prod_server" retine intr-un vector vecinii din dreapta a 
serverului introdus si al replicilor acestora si primul server si ultimul.
Apoi pastreaza in vectorul acesta doar un id al fiecarui server(adica din 
cele 3 id-uri ale serverului si replicilor pastreaza doar 1), iar pentru 
fiecare server ii extrage toate produsele intr-o lista folosind functia 
"server_retrieve_all". Si le introduce din nou, iar daca un produs se memoreaza 
pe alt server se sterge din serverul in care a fost memorat pana acum. 

Pentru determinarea pozitiei din "vect_ser" se foloseste functia "searc_poz" 
care returneaza pozitia pe care se afla serverul sau o replica a acestuia in 
vectorul hash_ring.


**3.loader_remove_server**
Aceasta elimina din memorie un server. Se determina pozitia din "hash_ring" al 
serverului cu functia "search_poz", se retin intr-o lista produsele din serverul 
cu id-ul indicat, se sterg din "hash_ring" id-ul serverul si al replicilor, si 
din vectorul poz pozitiile acestora. Se muta cu o pozitie la stanga serverele 
din dreapta acestuia, iar din vectorul poz se scade o pozitie pentru fiecare 
valoare care are valoarea mai mare ca serverul eliminat. 

La final se retin produsele in serverele celelalte cu ajutorul functiei 
"loader_store" si este eliberata memoria pentru serverul eliminat cu ajutorul 
functiei "free_server_memory".


**4.loader_store**
Aceasta adauga un produs intr-un server. Pentru a afla serverul in care trebuie 
sa adaugam folosim functia "binary_search" care cauta un server care are hashul 
mai mare ca hashul cheii. Se retine serverul in care trebuie adaugat in 
pointerul "server_id", si se adauga produsul cu ajutorul functiei "server_store".

**5.loader_retrieve**
Aceasta returneaza valoarea cheii indicate. Pentru aflarea serverului in care 
cautam folosim tot functia "binary_search", si extragem valoarea folosind 
functia "server_retrieve" si o returnam.


**6.free_load_balancer**
Aceasta functia elibereaza memoria totala .
Se pastraza pentru fiecare server in "hash_ring" doar un id al serverului sau 
al unei replici, iar apoi se sterg serverele care mai sunt in memorie cu 
ajutorul functiei "free_server_memory", si sunt eliberate si celelalte campuri 
care au fost alocate.


**Observatie**
Codul de la hashtable si liste este luat din laboratorul in care a trebuit 
sa implementam hashtablelul.





