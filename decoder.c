#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;

struct CryptoStateStore {
    int first;
    int sec;
    int last;
};

void initialize(struct CryptoStateStore *store) {
    store -> first = 0x12000032;
    store -> sec = 0x2527ac91;
    store -> last = 0x888c1214;
}

void supplyKey(struct CryptoStateStore *store, int key_len, char *key) {
    char buf[32] = {0};
    for (int i = 0; i < key_len; i++) {
        buf[i] = key[i];
    }

    int key_fill_index = 0;
    while (key_len < 32) {
        buf[key_len] = buf[key_fill_index];
        key_fill_index++;
        key_len++;
    }

    for (int offset = 0; offset < 4; offset++) {
        (store->first) <<= 8;
        (store->first) |= buf[offset + 0];

        (store->sec) <<= 8;
        (store->sec) |= buf[offset + 4];

        (store->last) <<= 8;
        (store->last) |= buf[offset + 8];
    }

    if (store->first == 0) {
        store->first = 301989938;
    }

    if (store->sec == 0) {
        store->sec = 623357073;
    }

    if (store->last == 0) {
        store->last = -2004086252;
    }
}

BYTE decryptByte(struct CryptoStateStore *store, BYTE data, int unknown) {
    BYTE ba = 0;
    BYTE bb = 0;

    int a = 1;
    int b = 0;

    int counter = 8;
    while (counter--) {
        if (store->first & 1 != 0) {
            store->first = ((0x80000062 ^ (store->first)) >> 1) | 0x80000000;
    
            int sec = store->sec;
            if (sec & 1 != 0) {
                store->sec = ((sec ^ 0x40000020) >> 1) | 0xc0000000;
                a = 1;
            } else {
                store->sec = (sec >> 1) & 0x3fffffff;
                a = 0;
            }
        } else {
            store->first = (store -> first >> 1) & 0x7fffffff;

            int last = store->last;
            if (last & 1 != 0) {
                store->last = ((last ^ 0x10000002) >> 1) | 0xf0000000;
                b = 1;
            } else {
                store->last = (last >> 1) & 0xfffffff;
                b = 0;
            }
        }

        ba = (BYTE) ((ba << 1) | (a ^ b));
    }

    bb = data ^ ba;
    return (unknown == 0 && bb == 0) ? (BYTE) (bb ^ ba) : bb;
}

void decrypt(struct CryptoStateStore *store, int arr_size, BYTE *byte_arr, int unknown) {
    for (int a = 0; a < arr_size; a++) {
        byte_arr[a] = decryptByte(store, byte_arr[a], unknown);
    }
}

// http://mwultong.blogspot.com/2007/02/c-file-copy-function-source-code.html
int fileCopy(FILE *in, FILE *out) {
  char* buf;
  size_t len;

  if ((buf = (char *) malloc(65536)) == NULL) { return 10; }

  while ( (len = fread(buf, sizeof(char), sizeof(buf), in)) != NULL )
    if (fwrite(buf, sizeof(char), len, out) == 0) {
      free(buf);
      return 3;
    }

  free(buf);

  return 0;
}

/*
 *
 * Simple decrypter
 * 
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <emot_file_name> [output_file_name]\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    char *out_filename;
    if (argc < 3) out_filename = argv[1];
	else out_filename = argv[2];

    BYTE buf[128] = {0};
    struct CryptoStateStore store;
    initialize(&store);

    supplyKey(&store, 0x20, "a271730728cbe141e47fd9d677e9006d");

    FILE *inputPtr = fopen(filename, "rb");
    if (inputPtr == NULL) {
        printf("Cannot read file %s\n", filename);
        return 1;
    }
	
    fread(buf, 1, 0x80, inputPtr);
    decrypt(&store, 0x80, buf, 1);

    FILE *outputPtr;
    if (strcmp(filename, out_filename) == 0) {
        outputPtr = fopen(out_filename, "rb+");
        fwrite(buf, 1, 0x80, outputPtr);
    } else {
        outputPtr = fopen(out_filename, "wb");
		
        if (outputPtr == NULL) {
            printf("Cannot write file %s\n", out_filename);
            return 1;
        }
    
        fseek(inputPtr, 0x80, SEEK_SET);
        fwrite(buf, 1, 0x80, outputPtr);
        fileCopy(inputPtr, outputPtr);
    }

    fclose(inputPtr);
    fclose(outputPtr);

    return 0;
}