#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t magic;
    char name[64];
    uint32_t offset;
    uint32_t length;
} initrd_file_header_t;

typedef struct {
    uint32_t nfiles;
} initrd_header_t;

int main(int argc, char **argv)
{
    if (argc < 3 || (argc - 1) % 2 != 0) {
        fprintf(stderr, "Usage: %s <input_file> <fs_name> [<input_file> <fs_name> ...]\n", argv[0]);
        return 1;
    }

    int nfiles = (argc - 1) / 2;
    initrd_file_header_t headers[64];

    if (nfiles > 64) {
        fprintf(stderr, "Too many files (max 64)\n");
        return 1;
    }

    uint32_t offset = sizeof(initrd_header_t) + 64 * sizeof(initrd_file_header_t);

    for (int i = 0; i < nfiles; i++) {
        const char *input_path = argv[i * 2 + 1];
        const char *fs_name = argv[i * 2 + 2];

        FILE *f = fopen(input_path, "rb");
        if (!f) {
            fprintf(stderr, "Error: cannot open file '%s'\n", input_path);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fclose(f);

        strncpy(headers[i].name, fs_name, 63);
        headers[i].name[63] = '\0';
        headers[i].magic = 0xBF;
        headers[i].offset = offset;
        headers[i].length = len;
        offset += len;
    }

    FILE *out = fopen("initrd.img", "wb");
    if (!out) {
        fprintf(stderr, "Error: cannot create initrd.img\n");
        return 1;
    }

    initrd_header_t main_header;
    main_header.nfiles = nfiles;
    fwrite(&main_header, sizeof(main_header), 1, out);
    fwrite(headers, sizeof(initrd_file_header_t), 64, out);

    for (int i = 0; i < nfiles; i++) {
        FILE *f = fopen(argv[i * 2 + 1], "rb");
        if (!f) {
            fprintf(stderr, "Error: cannot read file '%s'\n", argv[i * 2 + 1]);
            fclose(out);
            return 1;
        }
        uint8_t *buf = (uint8_t *)malloc(headers[i].length);
        if (!buf) {
            fclose(f);
            fclose(out);
            return 1;
        }
        fread(buf, 1, headers[i].length, f);
        fwrite(buf, 1, headers[i].length, out);
        free(buf);
        fclose(f);
    }

    fclose(out);
    printf("Created initrd.img with %d files.\n", nfiles);
    return 0;
}
