/* Copyright 2023 < Croitoru Constantin-Bogdan > */
#include "load_balancer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "utils.h"

#define PUTERE10 100000

struct load_balancer {
	// dimensiunea maxima
	int size_max;
	// numarul de servere
	int size;
	struct server_memory **vect_ser;
	// vector care retine id-ul serverului si al replicilor
	int *hash_ring;
	// vector care retine pozitia fiecarui server din vect_ser
	int *poz;
};

unsigned int hash_function_servers(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_key(void *a) {
	unsigned char *puchar_a = (unsigned char *)a;
	unsigned int hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

// initializeaza campurile
load_balancer *init_load_balancer() {
	struct load_balancer *load_bal;
	load_bal = malloc(sizeof(struct load_balancer));
	DIE(!load_bal, "Alocare esuata");

	load_bal->size_max = 2;
	load_bal->size = 0;

	load_bal->vect_ser = NULL;
	load_bal->hash_ring = NULL;
	load_bal->poz = NULL;

	return load_bal;
}

// se ordoneaza serverele dupa hash
// se modifica ordinea atat in hash_ring, cat si in vectorul poz
void ordering_hash_ring(int *hash_ring, int size, int *poz) {
	for (int i = 0; i < size - 1; i++)
		for (int j = i + 1; j < size; j++) {
			unsigned int hash_1 = hash_function_servers(&hash_ring[i]);
			unsigned int hash_2 = hash_function_servers(&hash_ring[j]);

			if (hash_1 > hash_2) {
				int aux = hash_ring[i];
				hash_ring[i] = hash_ring[j];
				hash_ring[j] = aux;
				aux = poz[i];
				poz[i] = poz[j];
				poz[j] = aux;
			}

			if (hash_1 == hash_2 && hash_ring[i] % PUTERE10 >
				hash_ring[j] % PUTERE10) {
				int aux = hash_ring[i];
				hash_ring[i] = hash_ring[j];
				hash_ring[j] = aux;

				aux = poz[i];
				poz[i] = poz[j];
				poz[j] = aux;
			}
		}
}

// returneaza al catelea element este in hash_ring serverul cu id-ul server_id
int search_poz(int *hash_ring, int size, int server_id) {
	for (int i = 0; i < size; i++)
		if (hash_ring[i] % PUTERE10 == server_id)
			return i;

	return -1;
}

// functia returneaza toate hey-ile si valorile dintr-un hashtable intr-o lista
linked_list_t *server_retrieve_all(load_balancer *main, int server_id) {
	linked_list_t *list;
	list = ll_create(sizeof(struct info));

	if (main->vect_ser[server_id]->serv) {
		unsigned int j;
		// se parcurge fiecare element din server
		for (j = 0; j < main->vect_ser[server_id]->serv->hmax; j++) {
			ll_node_t *curr;
			curr = main->vect_ser[server_id]->serv->buckets[j]->head;

			// se adauga in lista
			while (curr) {
				ll_add_nth_node(list, 0, curr->data);
				curr = curr->next;
			}
		}
	}

	return list;
}

// se redistribuie produsele dupa adaugarea unui nou server
void move_prod_server(load_balancer *main, int server_id) {
	int *vect, size = 0;
	vect = malloc(main->size * 3 * sizeof(int));
	DIE(!vect, "Alocare esuata");

	// se retin intr-un vector serverele care se afla in partea dreapta
	for (int i = 0; i < main->size * 3 - 1; i++)
		if (main->hash_ring[i] % PUTERE10 == server_id) {
			int dr = i + 1;
			int nr_server = main->hash_ring[dr] % PUTERE10;

			if (server_id != nr_server)
				vect[size++] = nr_server;
		}

	// se verifica si ultimul server de pe hash_ring
	if (main->hash_ring[main->size * 3 - 1] % PUTERE10 == server_id) {
		int dr = 0;
		int nr_server = main->hash_ring[dr] % PUTERE10;

		while (server_id == nr_server) {
			dr++;
			nr_server = main->hash_ring[dr] % PUTERE10;
		}
		vect[size++] = nr_server;
	}

	// se verifica primul server din hash_ring
	if (main->hash_ring[0] % PUTERE10 == server_id) {
		int dr = 1;
		int nr_server = main->hash_ring[dr] % PUTERE10;

		while (server_id == nr_server) {
			dr++;
			nr_server = main->hash_ring[dr] % PUTERE10;
		}

		vect[size++] = nr_server;
	}

	// se scot serverele care apar de mai multe ori
	for (int i = 0; i < size - 1; i++)
		for (int j = i + 1; j < size; j++)
			if (vect[i] == vect[j]) {
				for (int k = i; k < size - 1; k++)
					vect[k] = vect[k + 1];
				i--;
				size--;
				break;
			}

	// se redistribuie elementele fiecarui server din vect
	for (int i = 0; i < size; i++) {
		// se afla pozitia serverului din hash_ring
		int aux = search_poz(main->hash_ring, main->size * 3, vect[i]);

		// se afla pozitia serverului din main->vect_ser
		aux = main->poz[aux];

		// se creeaza lista cu produsele
		linked_list_t *list = server_retrieve_all(main, aux);

		ll_node_t *curr = list->head;
		while (curr) {
			int size;
			char *key = (char *)((info *)curr->data)->key;
			char *value = (char *)((info *)curr->data)->value;

			// se introduce produsul
			loader_store(main, key, value, &size);
			curr = curr->next;

			// daca se aduga in alt server, se sterge din serverul curent
			if (size != vect[i])
				server_remove(main->vect_ser[aux], key);
		}

		ll_free(&list);
	}

	free(vect);
}

// se adauga un server
void loader_add_server(load_balancer *main, int server_id) {
	int max_size = main->size_max;

	// se aloca daca sunt NULL, sau se realoca cand s-a atins max_size
	if (main->vect_ser == NULL) {
		main->vect_ser = malloc(max_size * sizeof(server_memory *));
		DIE(!main->vect_ser, "Alocare esuata");

	} else if (main->size == max_size) {
		// size_max isi dubleaza valoarea
		main->size_max *= 2;
		main->vect_ser = realloc(main->vect_ser,
								 main->size_max * sizeof(server_memory *));
		DIE(!main->vect_ser, "Alocare esuata");
	}

	if (main->hash_ring == NULL) {
		main->hash_ring = malloc(main->size_max * 3 * sizeof(int));
		DIE(!main->hash_ring, "Alocare esuata");

	} else if (main->size == max_size) {
		main->hash_ring = realloc(main->hash_ring,
								  main->size_max * 3 * sizeof(int));
		DIE(!main->hash_ring, "Alocare esuata");
	}

	if (main->poz == NULL) {
		main->poz = malloc(max_size * 3 * sizeof(int));
		DIE(!main->poz, "Alocare esuata");

	} else if (main->size == max_size) {
		main->poz = realloc(main->poz, main->size_max * 3 * sizeof(int));
		DIE(!main->poz, "Alocare esuata");
	}

	main->size++;

	main->vect_ser[main->size - 1] = init_server_memory();

	// se retine id serverului si al replicilor
	main->hash_ring[(main->size - 1) * 3] = server_id;
	main->hash_ring[(main->size - 1) * 3 + 1] = server_id + PUTERE10;
	main->hash_ring[(main->size - 1) * 3 + 2] = server_id + 2 * PUTERE10;

	// se retine pozitia pe care s-a adaugat in vect_ser
	main->poz[(main->size - 1) * 3] = main->size - 1;
	main->poz[(main->size - 1) * 3 + 1] = main->size - 1;
	main->poz[(main->size - 1) * 3 + 2] = main->size - 1;

	// se ordoneaza dupa hash
	ordering_hash_ring(main->hash_ring, main->size * 3, main->poz);

	// se redistribuie produsele din servere
	if (main->size > 1)
		move_prod_server(main, server_id);
}

// eliminare server
void loader_remove_server(load_balancer *main, int server_id) {
	int server_id_cpy = server_id;
	// se determina pozitia din hash_ring
	server_id_cpy = search_poz(main->hash_ring, main->size * 3, server_id_cpy);

	// se determina pozitia din vect_ser
	server_id_cpy = main->poz[server_id_cpy];

	// se creeaza lista cu produsele din server
	linked_list_t *list = server_retrieve_all(main, server_id_cpy);

	int size_hr = main->size * 3;
	// se elimina serverul si replicile din hash_ring
	for (int i = 0; i < size_hr; i++)
		if (main->hash_ring[i] % PUTERE10 == server_id) {
			for (int j = i; j < size_hr - 1; j++) {
				main->hash_ring[j] = main->hash_ring[j + 1];
				main->poz[j] = main->poz[j + 1];
			}
			size_hr--;
			i--;
		}

	server_memory *ptr_server = main->vect_ser[server_id_cpy];

	// se muta serverele cu o pozitie la stanga
	for (int i = server_id_cpy; i < main->size - 1; i++)
		main->vect_ser[i] = main->vect_ser[i + 1];

	main->size--;

	// se scad pozitiile cu o unitate deoarece s-a eliminat un server
	for (int i = 0; i < main->size * 3; i++)
		if (main->poz[i] >= server_id_cpy)
			main->poz[i] = main->poz[i] - 1;

	// se redistribuie produsele
	ll_node_t *curr = list->head;
	while (curr) {
		int size;
		char *key = (char *)((info *)curr->data)->key;
		char *value = (char *)((info *)curr->data)->value;
		loader_store(main, key, value, &size);
		curr = curr->next;
	}

	ll_free(&list);

	free_server_memory(ptr_server);

	// realocare
	if(main->size < main->size_max / 2) {
		main->size_max = main->size_max / 2;
		main->vect_ser = realloc(main->vect_ser,
								 main->size_max * sizeof(server_memory *));
		DIE(!main->vect_ser, "Alocare esuata");

		main->hash_ring = realloc(main->hash_ring,
								  main->size_max * 3 * sizeof(int));
		DIE(!main->hash_ring, "Alocare esuata");

		main->poz = realloc(main->poz, main->size_max * 3 * sizeof(int));
		DIE(!main->poz, "Alocare esuata");
	}
}

// se cauta un serverul in care se poate introduce produsul cu cheia key
int binary_search(int *hash_ring, char *key, int size) {
	int st = 0, dr = size - 1;
	int mij = (st + dr) / 2;
	if (hash_function_servers(&hash_ring[0]) > hash_function_key(key))
		return 0;
	if (hash_function_servers(&hash_ring[size - 1]) < hash_function_key(key))
		return 0;

	while (st <= dr) {
		mij = (st + dr) / 2;
		if (hash_function_servers(&hash_ring[mij]) > hash_function_key(key) &&
			hash_function_servers(&hash_ring[mij - 1]) <= hash_function_key(key))
			return mij;

		if (hash_function_servers(&hash_ring[mij]) > hash_function_key(key))
			dr = mij - 1;
		else
			st = mij + 1;
	}

	return 0;
}

// se adauga un produs intr-un server
void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
	int i;
	// se cauta serverul in care se poate introduce
	i = binary_search(main->hash_ring, key, main->size * 3);

	int nr_server = main->hash_ring[i] % PUTERE10;

	// se retine id-ul serverului
	*server_id = nr_server;

	nr_server = search_poz(main->hash_ring, main->size * 3, nr_server);
	nr_server = main->poz[nr_server];

	// se adauga pe server
	server_store(main->vect_ser[nr_server], key, value);
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
	int i;
	// se cauta serverul in care se poate introduce
	i = binary_search(main->hash_ring, key, main->size * 3);

	void *value;

	int nr_server = main->hash_ring[i] % PUTERE10;
	// se retine id-ul serverului
	*server_id = nr_server;

	nr_server = search_poz(main->hash_ring, main->size * 3, nr_server);
	nr_server = main->poz[nr_server];

	// se retine valoarea produsului cu cheia key
	value = server_retrieve(main->vect_ser[nr_server], (void *)key);

	return (char *)value;
}

void free_load_balancer(load_balancer *main) {
	int size = main->size * 3;
	// se retine doar o replica a unui server
	for (int i = 0; i < size - 1; i++)
		for (int j = i + 1; j < size; j++)
			if (main->hash_ring[i] % PUTERE10 == main->hash_ring[j] % PUTERE10) {
				for (int k = i; k < size - 1; k++) {
					main->hash_ring[k] = main->hash_ring[k + 1];
					main->poz[k] = main->poz[k + 1];
				}
				size--;
				i--;
				break;
			}
	// se sterg serverele
	for (int i = 0; i < main->size; i++) {
		free_server_memory(main->vect_ser[main->poz[i]]);
	}

	// se elibereaza memoria si pentru restul campurilor
	free(main->hash_ring);
	free(main->vect_ser);
	free(main->poz);
	free(main);
}
