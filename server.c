/* Copyright 2023 < Croitoru Constantin-Bogdan > */
#include "server.h"

#include <stdlib.h>
#include <string.h>

#include "hashtable.h"
#include "utils.h"
#define HMAX 100

server_memory *init_server_memory() {
	struct server_memory *server;
	server = malloc(sizeof(struct server_memory));
	DIE(!server, "Alocare esuata");

	// se creeaza serverul cu ajutorul functiei ht_create de la hashtable
	server->serv = ht_create(HMAX, hash_function_string, compare_function_strings,
							 key_val_free_function);

	return server;
}

void server_store(server_memory *server, char *key, char *value) {
	// se adauga produsul in server cu ajutorul functiei ht_put
	ht_put(server->serv, key, strlen(key) + 1, value, (strlen(value) + 1));
}

char *server_retrieve(server_memory *server, char *key) {
	// se determina valoare unei chei cu ajutorul functiei ht_get
	void *value = ht_get(server->serv, key);

	return (char *)value;
}

void server_remove(server_memory *server, char *key) {
	// se elimina produsul cu cheia key
	ht_remove_entry(server->serv, key);
}

void free_server_memory(server_memory *server) {
	// se elibereaza memoria pentru un server
	ht_free(server->serv);
	free(server);
}
