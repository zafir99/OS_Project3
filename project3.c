#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define SIZEUINT64_T sizeof(uint64_t)
#define MAGICNUMBER "4348PRJ3"
#define MAGICSIZE 8
#define NUMCOMMANDS 6
#define HEADERSIZE 24
#define BLOCKSIZE 512
#define REMAINING 488
#define DEGREE 10
#define MAXIMAL 2*DEGREE-1
#define SPLIT (2*DEGREE-1)/2
#define MAXCHILDREN 2*DEGREE
#define EMPTYSPACE 24 // 24 bytes of empty space with this blocksize and TreeNode struct
#define BUFSIZE 21

const char *COMMANDS[NUMCOMMANDS] = {"create", "insert", "search", "load", "print", "extract"};

typedef uint64_t be_int;

typedef struct {
	be_int block_id;
	be_int parent;
	be_int numKeys;
	be_int key[MAXIMAL];
	be_int value[MAXIMAL];
	be_int child[MAXCHILDREN];
	uint8_t empty[HEADERSIZE];
} TreeNode;

typedef struct {
	char magic[MAGICSIZE];
	be_int root_id;
	be_int next_block;
} Header;

static inline int validCSV (int csvfd) {
	char c;
	int i = 0;
	while (read(csvfd, &c, 1)) {
		if (!isdigit(c)) {
			if (c == ',' && !i) {
				++i;
				continue;
			}
			if (c == '\n') {
				i = 0;
				continue;
			}
			return 0;
		}
	}
	lseek(csvfd, 0, SEEK_SET);
	return 1;
}

static inline int bigEndian (void) {
	int x = 1;
	return ((uint8_t *)&x)[0] != 1;
}

static inline void reverseBytes (uint64_t *src) {
	*src = (*src << 56) | (*src >> 56) | (*src & 0xFF00) << 40 | (*src >> 40 & 0xFF00) | (*src & 0xFF0000) << 24 | (*src >> 24 & 0xFF0000) |
		   (*src & 0xFF000000) << 8 | (*src >> 8 & 0xFF000000);
}

static inline int validCommand (char *str) {
	for (int i = 0; i < NUMCOMMANDS; ++i) {
		if (!strcmp(str, COMMANDS[i])) {
			return i;
		}
	}
	return -1;
}

static inline void reverseHeader (Header *header) {
	reverseBytes(&header->root_id);
	reverseBytes(&header->next_block);
}

static inline void reverseNode (TreeNode *node) {
	reverseBytes(&node->block_id);
	reverseBytes(&node->parent);
	reverseBytes(&node->numKeys);
	for (uint64_t i = 0; i < node->numKeys; ++i) {
		reverseBytes(&node->key[i]);
		reverseBytes(&node->value[i]);
		reverseBytes(&node->child[i]);
	}
	reverseBytes(&node->child[node->numKeys]);
}

static inline void reverseNodeI (TreeNode *node, uint64_t numKeys) {
	reverseBytes(&node->block_id);
	reverseBytes(&node->parent);
	reverseBytes(&node->numKeys);
	for (uint64_t i = 0; i < numKeys; ++i) {
		reverseBytes(&node->key[i]);
		reverseBytes(&node->value[i]);
		reverseBytes(&node->child[i]);
	}
	reverseBytes(&node->child[numKeys]);
}

static inline void sortNode (TreeNode *node) {
	for (uint64_t i = 1; i < node->numKeys; ++i) {
		uint64_t j = i;
		while (j > 0 && node->key[j] < node->key[j-1]) {
			// could do this with a single temp var instead
			// lowk shouldve done a nicer insertion sort
			uint64_t temp_key = node->key[j];
			uint64_t temp_val = node->value[j];
			uint64_t temp_child = node->child[j+1];
			node->key[j] = node->key[j-1];
			node->value[j] = node->value[j-1];
			node->child[j+1] = node->child[j];
			node->key[j-1] = temp_key;
			node->value[j-1] = temp_val;
			node->child[j] = temp_child;
			--j;
		}
	}
}

static inline void shiftNode (TreeNode *node) {
	memmove(node->key+1, node->key, (node->numKeys-1)*SIZEUINT64_T);
	memmove(node->value+1, node->value, (node->numKeys-1)*SIZEUINT64_T);
	memmove(node->child+1, node->key, node->numKeys*SIZEUINT64_T);
}

static inline void printTree (int idxfd) {
	TreeNode node;
	ssize_t bytesRead = read(idxfd, &node, BLOCKSIZE);
	if (!bytesRead) {
		puts("\nTree is empty.\n\n");
		return;
	}
	while (bytesRead > 0) {
		if (!bigEndian()) {
			reverseNode(&node);
		}
		for (uint64_t i = 0; i < node.numKeys; ++i) {
			printf("Key: %ld, Value: %ld\n", node.key[i], node.value[i]);
		}
		bytesRead = read(idxfd, &node, BLOCKSIZE);
	}
	if (bytesRead == -1) {
		perror("ERROR: ");
		exit(10);
	}
}

static inline void printNode (TreeNode *node) {
	for (uint64_t i = 0; i < node->numKeys; ++i) {
		if (!bigEndian()) {
			reverseNode(node);
		}
		printf("Block ID: %ld\n", node->block_id);
		for (uint64_t i = 0; i < node->numKeys; ++i) {
			printf("Key: %ld, Value: %ld\n", node->key[i], node->value[i]);
		}
	}
}

static inline void insertTree (int idxfd, Header *header, uint64_t key, uint64_t value) {
	TreeNode node;
	memset(&node, 0, BLOCKSIZE);

	if (!header->root_id) {
		header->root_id = 1;
		header->next_block = 2;
		node.block_id = 1;
		if (!bigEndian()) {
			reverseHeader(header);
			node.block_id <<= 56;
		}
		lseek(idxfd, 0, SEEK_SET);
		if (write(idxfd, header, HEADERSIZE) == -1) {
			perror("ERROR: ");
			exit(30);
		}
		lseek(idxfd, BLOCKSIZE, SEEK_SET);
		if (write(idxfd, &node, BLOCKSIZE) == -1) {
			perror("ERROR: ");
			exit(30);
		}
		if (!bigEndian()) {
			reverseHeader(header);
		}
	}

	lseek(idxfd, BLOCKSIZE*header->root_id, SEEK_SET);
	ssize_t bytesRead = read(idxfd, &node, BLOCKSIZE);
	while (bytesRead > 0) {
		if (!bigEndian()) {
			reverseNode(&node);
		}
		uint64_t i = 0;
		while (i < node.numKeys && node.key[i] < key) {
			++i;
		}
		if (node.key[i] == key && node.numKeys) { // include 0 as a valid key
			printf("\nERROR: Key %ld already assigned Value %ld.\n\n", key, node.value[i]);
			break;
		}

		if (!node.child[0] && node.numKeys < MAXIMAL) {
			node.key[node.numKeys] = key;
			node.value[node.numKeys] = value;
			++node.numKeys;
			sortNode(&node);
			if (!bigEndian()) {
				reverseNodeI(&node, node.numKeys);
			}
			lseek(idxfd, -BLOCKSIZE, SEEK_CUR);
			if (write(idxfd, &node, BLOCKSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			printf("\nKey-Value Pair: (%ld, %ld) successfully inserted.\n\n", key, value);
			break;
		}
		else if (!node.child[0] && node.numKeys == MAXIMAL && node.block_id == header->root_id) { // logic can likely be simplified
			TreeNode node2, newRoot;
			memset(&node2, 0, BLOCKSIZE);
			memset(&newRoot, 0, BLOCKSIZE);

			node2.block_id = header->next_block++;
			newRoot.block_id = header->next_block++;
			newRoot.numKeys = 1;
			newRoot.child[0] = header->root_id;
			newRoot.child[1] = node2.block_id;
			node.parent = newRoot.block_id;
			node2.parent = newRoot.block_id;
			header->root_id = newRoot.block_id;
			newRoot.key[0] = node.key[SPLIT];
			newRoot.value[0] = node.value[SPLIT];

			memcpy(node2.key, node.key+DEGREE, SPLIT*SIZEUINT64_T);
			memcpy(node2.value, node.value+DEGREE, SPLIT*SIZEUINT64_T);
			memset(node.key+SPLIT, 0, DEGREE*SIZEUINT64_T);
			memset(node.value+SPLIT, 0, DEGREE*SIZEUINT64_T);
			node.numKeys = SPLIT;
			node2.numKeys = SPLIT;

			TreeNode *indirect = (key < newRoot.key[0]) ? &node : &node2;
			indirect->key[indirect->numKeys] = key;
			indirect->value[indirect->numKeys] = value;
			++indirect->numKeys;

			uint64_t node_id = node.block_id;
			uint64_t node2_id = node2.block_id;
			uint64_t newRoot_id = newRoot.block_id;

			if (!bigEndian()) {
				reverseNodeI(&node, node.numKeys);
				reverseNodeI(&node2, node2.numKeys);
				reverseNodeI(&newRoot, 1);
				reverseHeader(header);
			}

			lseek(idxfd, 0, SEEK_SET);
			if (write(idxfd, header, HEADERSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			lseek(idxfd, BLOCKSIZE*node_id, SEEK_SET);
			if (write(idxfd, &node, BLOCKSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			lseek(idxfd, BLOCKSIZE*node2_id, SEEK_SET);
			if (write(idxfd, &node2, BLOCKSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			lseek(idxfd, BLOCKSIZE*newRoot_id, SEEK_SET);
			if (write(idxfd, &newRoot, BLOCKSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			printf("\nKey-Value Pair: (%ld, %ld) successfully inserted.\n\n", key, value);
			if (!bigEndian()) {
				reverseHeader(header);
			}
			break;
		}
		else if (!node.child[0] && node.numKeys == MAXIMAL) {
			uint64_t child = 0;
			uint64_t node_id = 0;
			printf("\nKey-Value Pair: (%ld, %ld) successfully inserted.\n\n", key, value);

			while (node.numKeys == MAXIMAL) {
				TreeNode node2;
				memset(&node2, 0, BLOCKSIZE);
				node2.block_id = header->next_block++;
				uint64_t node2_id;
				TreeNode *indirect;

				if (node.block_id == header->root_id) {
					TreeNode newRoot;
					memset(&newRoot, 0, BLOCKSIZE);
					newRoot.block_id = header->next_block++;
					newRoot.numKeys = 1;
					newRoot.child[0] = header->root_id;
					newRoot.child[1] = node2.block_id;
					node.parent = newRoot.block_id;
					node2.parent = newRoot.block_id;
					header->root_id = newRoot.block_id;
					newRoot.key[0] = node.key[SPLIT];
					newRoot.value[0] = node.value[SPLIT];

					memcpy(node2.key, node.key+DEGREE, SPLIT*SIZEUINT64_T);
					memcpy(node2.value, node.value+DEGREE, SPLIT*SIZEUINT64_T);
					memcpy(node2.child, node.child+DEGREE, DEGREE*SIZEUINT64_T);
					memset(node.key+SPLIT, 0, DEGREE*SIZEUINT64_T);
					memset(node.value+SPLIT, 0, DEGREE*SIZEUINT64_T);
					memset(node.child+DEGREE, 0, DEGREE*SIZEUINT64_T);
					node2.numKeys = SPLIT;
					node.numKeys = SPLIT;
					node2.parent = node.parent;
					node_id = node.block_id;
					node2_id = node2.block_id;
					uint64_t newRoot_id = newRoot.block_id;
					node.numKeys = SPLIT;
					node2.numKeys = SPLIT;

					indirect = (key < newRoot.key[0]) ? &node : &node2;
					indirect->key[indirect->numKeys] = key;
					indirect->value[indirect->numKeys] = value;
					indirect->child[++indirect->numKeys] = child;
					sortNode(indirect);

					if (!bigEndian()) {
						reverseBytes(&node2_id);
					}
					for (uint64_t j = 0; j <= node2.numKeys; ++j) {
						lseek(idxfd, BLOCKSIZE*node2.child[j]+SIZEUINT64_T, SEEK_SET);
						if (write(idxfd, &node2_id, SIZEUINT64_T) == -1) {
							perror("ERROR: ");
							exit(30);
						}
					}
					if (!bigEndian()) {
						reverseBytes(&node2_id);
						reverseNodeI(&node, node.numKeys);
						reverseNodeI(&node2, node2.numKeys);
						reverseNodeI(&newRoot, 1);
						reverseHeader(header);
					}

					lseek(idxfd, 0, SEEK_SET);
					if (write(idxfd, header, HEADERSIZE) == -1) {
						perror("ERROR: ");
						exit(30);
					}
					lseek(idxfd, BLOCKSIZE*node_id, SEEK_SET);
					if (write(idxfd, &node, BLOCKSIZE) == -1) {
						perror("ERROR: ");
						exit(30);
					}
					lseek(idxfd, BLOCKSIZE*node2_id, SEEK_SET);
					if (write(idxfd, &node2, BLOCKSIZE) == -1) {
						perror("ERROR: ");
						exit(30);
					}
					lseek(idxfd, BLOCKSIZE*newRoot_id, SEEK_SET);
					if (write(idxfd, &newRoot, BLOCKSIZE) == -1) {
						perror("ERROR: ");
						exit(30);
					}
					if (!bigEndian()) {
						reverseHeader(header);
					}
					return;
				}

				uint64_t oldKey = node.key[SPLIT];
				uint64_t oldValue = node.value[SPLIT];
				uint64_t parent = node.parent;

				memcpy(node2.key, node.key+DEGREE, SPLIT*SIZEUINT64_T);
				memcpy(node2.value, node.value+DEGREE, SPLIT*SIZEUINT64_T);
				memset(node.key+SPLIT, 0, DEGREE*SIZEUINT64_T);
				memset(node.value+SPLIT, 0, DEGREE*SIZEUINT64_T);
				node2.numKeys = SPLIT;
				node.numKeys = SPLIT;
				node2.parent = parent;
				node_id = node.block_id;
				node2_id = node2.block_id;

				indirect = (key < oldKey) ? &node : &node2;
				indirect->key[indirect->numKeys] = key;
				indirect->value[indirect->numKeys++] = value;
				if (node.child[0]) {
					memcpy(node2.child, node.child+DEGREE, DEGREE*SIZEUINT64_T);
					memset(node.child+DEGREE, 0, DEGREE*SIZEUINT64_T);
					indirect->child[indirect->numKeys] = child;
					if (!bigEndian()) {
						reverseBytes(&node2_id);
					}
					for (uint64_t j = 0; j <= node2.numKeys; ++j) {
						lseek(idxfd, BLOCKSIZE*node2.child[j]+SIZEUINT64_T, SEEK_SET); // navigate to parent id location
						if (write(idxfd, &node2_id, SIZEUINT64_T) == -1) {
							perror("ERROR: ");
							exit(30);
						}
					}
					if (!bigEndian()) {
						reverseBytes(&node2_id);
					}
				}
				sortNode(indirect);

				child = node2_id;
				if (!bigEndian()) {
					reverseNodeI(&node, node.numKeys);
					reverseNodeI(&node2, node2.numKeys);
				}

				lseek(idxfd, BLOCKSIZE*node_id, SEEK_SET);
				if (write(idxfd, &node, BLOCKSIZE) == -1) {
					perror("ERROR: ");
					exit(30);
				}
				lseek(idxfd, BLOCKSIZE*node2_id, SEEK_SET);
				if (write(idxfd, &node2, BLOCKSIZE) == -1) {
					perror("ERROR: ");
					exit(30);
				}

				lseek(idxfd, BLOCKSIZE*parent, SEEK_SET);
				read(idxfd, &node, BLOCKSIZE);
				if (!bigEndian()) {
					reverseNode(&node);
				}
				key = oldKey;
				value = oldValue;
			}

			node.key[node.numKeys] = key;
			node.value[node.numKeys] = value;
			node.child[++node.numKeys] = child;
			node_id = node.block_id;
			sortNode(&node);
			if (!bigEndian()) {
				reverseNodeI(&node, node.numKeys);
				reverseHeader(header);
			}
			lseek(idxfd, 0, SEEK_SET);
			if (write(idxfd, header, HEADERSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			lseek(idxfd, BLOCKSIZE*node_id, SEEK_SET);
			if (write(idxfd, &node, BLOCKSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			if (!bigEndian()) {
				reverseHeader(header);
			}
			break;
		}
		lseek(idxfd, BLOCKSIZE*node.child[i], SEEK_SET);
		bytesRead = read(idxfd, &node, BLOCKSIZE);
	}
	if (bytesRead == -1) {
		perror("ERROR: ");
		exit(10);
	}
}

static inline void loadTree (int idxfd, int csvfd, Header *header) {
	FILE *csv = fdopen(csvfd, "r"); // easy input parsing, i dont want to write the string to int conversions
	uint64_t key, value;
	while (fscanf(csv, "%ld,%ld\n", &key, &value) != EOF) {
		insertTree(idxfd, header, key, value);
	}
}

static inline void searchTree (int idxfd, uint64_t root, uint64_t key) {
	TreeNode node;
	lseek(idxfd, BLOCKSIZE*root, SEEK_SET);
	ssize_t bytesRead = read(idxfd, &node, BLOCKSIZE);
	while (bytesRead > 0) {
		if (!bigEndian()) {
			reverseNode(&node);
		}
		uint64_t i = 0;
		while (i < node.numKeys && node.key[i] < key) {
			++i;
		}
		if (node.key[i] == key) {
			printf("\nKey: %ld, Value: %ld\n\n", key, node.value[i]);
			break;
		}
		if (!node.child[i]) {
			printf("\nKey %ld does not exist.\n\n", key);
			break;
		}
		lseek(idxfd, BLOCKSIZE*node.child[i], SEEK_SET);
		bytesRead = read(idxfd, &node, BLOCKSIZE);
	}
	if (bytesRead == -1) {
		perror("ERROR: ");
		exit(10);
	}
}

static inline void extractTree (int idxfd, int csvfd) {
	TreeNode node;
	ssize_t bytesRead = read(idxfd, &node, BLOCKSIZE);
	while (bytesRead > 0) {
		if (!bigEndian()) {
			reverseNode(&node);
		}
		for (uint64_t i = 0; i < node.numKeys; ++i) {
			dprintf(csvfd, "%ld,%ld\n", node.key[i], node.value[i]);
		}
		bytesRead = read(idxfd, &node, BLOCKSIZE);
	}
	if (bytesRead == -1) {
		perror("ERROR: ");
		exit(10);
	}
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
		printf("\nERROR: \"%s\" is an invalid command.\n\n", argv[1]);
		exit(2);
	}
	if (argc != expectedArg[commandNum]) {
		printf("\nERROR: Invalid number of arguments.\nUsage: %s\n\n", usage[commandNum]);
		exit(1);
	}

	int idxfd;
	Header header;
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

		read(idxfd, &header, HEADERSIZE);
		if (strcmp(header.magic, MAGICNUMBER)) {
			printf("\nERROR: Index file \"%s\" is in invalid format.\n\n", argv[2]);
			exit(5);
		}
		reverseHeader(&header);
		lseek(idxfd, BLOCKSIZE, SEEK_SET);
	}

	int csvfd;
	switch (commandNum) {
		case 0 :
			if (write(idxfd, MAGICNUMBER, MAGICSIZE) == -1) {
				perror("ERROR: ");
				exit(30);
			}

			header.root_id = 0;
			if (write(idxfd, &header.root_id, SIZEUINT64_T) == -1) {
				perror("ERROR: ");
				exit(30);
			}

			header.next_block = 1;
			if (!bigEndian()) {
				reverseBytes(&header.next_block);
			}

			if (write(idxfd, &header.next_block, SIZEUINT64_T) == -1) {
				perror("ERROR: ");
				exit(30);
			}

			uint8_t zero[REMAINING] = {};
			if (write(idxfd, zero, REMAINING) == -1) {
				perror("ERROR: ");
				exit(30);
			}
			
			printf("\nIndex file \"%s\" successfully created.\n\n", argv[2]);
			break;

		case 1 :
				insertTree(idxfd, &header, atol(argv[3]), atol(argv[4]));
			break;

		case 2 :
				searchTree(idxfd, header.root_id, atol(argv[3]));
			break;

		case 3 :
				csvfd = open(argv[3], O_RDWR, S_IRWXU);
				if (csvfd == -1) {
					printf("\nERROR: CSV File \"%s\" does not exist.\n\n", argv[3]);
					exit(12);
				}
				if (!validCSV(csvfd)) {
					printf("\nERROR: CSV File \"%s\" is in invalid format.\n\n", argv[3]);
					exit(16);
				}
				loadTree(idxfd, csvfd, &header);
				if (close(csvfd) == -1) {
					perror("\nERROR: Unable to close CSV file.\n");
					exit(13);
				}
				printf("\nCSV File \"%s\" successfully loaded into tree.\n\n", argv[3]);
			break;

		case 4 :
				printTree(idxfd);
			break;

		case 5 :
				csvfd = open(argv[3], O_CREAT|O_EXCL|O_RDWR, S_IRWXU);
				if (csvfd == -1) {
					printf("\nERROR: CSV File \"%s\" already exists.\n\n", argv[4]);
					exit(14);
				}
				if (!validCSV(csvfd)) {
					printf("\nERROR: CSV File \"%s\" is in invalid format.\n\n", argv[3]);
					exit(16);
				}
				extractTree(idxfd, csvfd);
				if (close(csvfd) == -1) {
					perror("\nERROR: Unable to close CSV file.\n");
					exit(15);
				}
			break;

		default :
			puts ("\nThis shouldn't be happening!");
	}

	if (close(idxfd) == -1) {
		perror("\nERROR: Unable to close index file.\n");
		exit(50);
	}

	return 0;
}
