#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SIZEUINT64_T 8
#define MAGICNUMBER "4348PRJ3"
#define MAGICSIZE 8
#define NUMCOMMANDS 6
#define BLOCKSIZE 512
#define DEGREE 10
#define MAXIMAL 2*DEGREE-1
#define MAXCHILDREN 2*DEGREE
#define EMPTYSPACE 24 // 24 bytes of empty space with this blocksize and TreeNode struct

const char *COMMANDS[NUMCOMMANDS] = {"create", "insert", "search", "load", "print", "extract"};

typedef union {
	uint8_t byte[SIZEUINT64_T];
	uint64_t val;
} be_int;

typedef struct {
	be_int block_id;
	be_int parent;
	be_int numKeys;
	be_int key[MAXIMAL];
	be_int value[MAXIMAL];
	be_int child[MAXCHILDREN];
	uint8_t empty[EMPTYSPACE];
} TreeNode;

static inline int bigEndian (void) {
	int x = 1;
	return ((uint8_t *)&x)[0] != 1;
}

static inline void reverseBytes (be_int *src) {
	for (int i = 0; i < SIZEUINT64_T/2; ++i) {
		uint8_t temp = src->byte[i];
		src->byte[i] = src->byte[SIZEUINT64_T-1-i];
		src->byte[SIZEUINT64_T-1-i] = temp;
	}
}

int validCommand (char *str) {
	for (int i = 0; i < NUMCOMMANDS; ++i) {
		if (!strcmp(str, COMMANDS[i])) {
			return i;
		}
	}
	return -1;
}

const char* usage[NUMCOMMANDS] = {"<program> create <new_idx_filename>",
								  "<program> insert <existing_idx_file> <new_integer_key> <integer_value>",
								  "<program> search <existing_idx_file> <integer_key>",
								  "<program> load <existing_idx_file> <existing_csv_file>",
								  "<program> print <existing_idx_file>",
								  "<program> extract <existing_idx_file> <new_csv_filename>"};

const int expectedArg[NUMCOMMANDS] = {3, 5, 4, 4, 3, 4};

int main (int argc, char **argv) {
	if (argc < 3) {
		printf("\nUsage: %s\n", usage[0]);
		for (int i = 1; i < NUMCOMMANDS-1; ++i) {
			printf("       %s\n", usage[i]);
		}
		printf("       %s\n\n", usage[NUMCOMMANDS-1]);
		exit(1);
	}

	int commandNum = validCommand(argv[1]);
	if (commandNum == -1) {
		printf("\n\"%s\" is an invalid command.\n\n", argv[1]);
		exit(2);
	}
	if (argc != expectedArg[commandNum]) {
		printf("\nERROR: Invalid number of arguments.\nUsage: %s\n\n", usage[commandNum]);
		exit(1);
	}

	int idxfd;
	if (!commandNum) {
		idxfd = open(argv[2], O_CREAT|O_EXCL|O_RDWR, S_IRWXU);
		if (idxfd == -1) {
			printf("\nERROR: Index file \"%s\" already exists.\n\n", argv[2]);
			exit(3);
		}
	}
	else {
		idxfd = open(argv[2], O_RDWR, S_IRWXU);
		if (idxfd == -1) {
			printf("\nERROR: Index file \"%s\" does not exist.\n\n", argv[2]);
			exit(4);
		}

		char buf[MAGICSIZE];
		read(idxfd, buf, MAGICSIZE);
		if (strcmp(buf, MAGICNUMBER)) {
			printf("\nERROR: Index file \"%s\" is in invalid format.\n\n", argv[2]);
			exit(5);
		}
	}

	switch (commandNum) {
		case 0 :
			write(idxfd, MAGICNUMBER, MAGICSIZE);
			be_int num = {0};
			write(idxfd, &num, SIZEUINT64_T);
			num.val = 1;
			if (!bigEndian()) {
				reverseBytes(&num);
			}
			write(idxfd, &num, SIZEUINT64_T);
			num.val = 0;
			uint64_t remaining = BLOCKSIZE-MAGICSIZE-(2*SIZEUINT64_T);
			while (remaining > 0) {
				write(idxfd, &num, SIZEUINT64_T);
				remaining -= SIZEUINT64_T;
			}
			printf("\nIndex file \"%s\" successfully created.\n\n", argv[2]);
			break;

		case 1 :

			break;

		case 2 :

			break;

		case 3 :

			break;

		case 4 :

			break;

		case 5 :

			break;

		default :
			puts ("\nThis shouldn't be happening!");
	}

	idxfd = close(idxfd);
	if (idxfd == -1) {
		perror("\nERROR: Unable to close index file.\n");
		exit(50);
	}

	return 0;
}
