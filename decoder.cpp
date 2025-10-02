#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static signed char b64_rev[256];

static void init_b64_rev(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;
    for (int i = 0; i < 256; ++i) b64_rev[i] = -1;
    for (int i = 0; i < 64; ++i) b64_rev[(unsigned char)b64_table[i]] = (signed char)i;
    b64_rev[(unsigned char)'='] = 0;
}

static unsigned char *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    unsigned char *buf = (unsigned char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = 0;
    if (out_size) *out_size = n;
    return buf;
}

static int write_file(const char *path, const unsigned char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t w = fwrite(data, 1, len, f);
    fclose(f);
    return (w == len) ? 0 : -1;
}

static char *base64_encode(const unsigned char *data, size_t in_len, size_t *out_len) {
    size_t out_size = ((in_len + 2) / 3) * 4;
    char *out = (char*)malloc(out_size + 1);
    if (!out) return NULL;
    size_t i = 0, o = 0;

    while (i < in_len) {
        unsigned a = data[i++];
        unsigned b = (i < in_len) ? data[i++] : 0;
        unsigned c = (i < in_len) ? data[i++] : 0;

        unsigned triple = (a << 16) | (b << 8) | c;

        out[o++] = b64_table[(triple >> 18) & 0x3F];
        out[o++] = b64_table[(triple >> 12) & 0x3F];
        if ((i - 1) < in_len) {
            out[o++] = b64_table[(triple >> 6) & 0x3F];
        } else {
            out[o++] = '=';
        }
        if (i <= in_len) {
            out[o++] = b64_table[triple & 0x3F];
        } else {
            out[o++] = '=';
        }
    }

    out[o] = '\0';
    if (out_len) *out_len = o;
    return out;
}

static unsigned char *base64_decode(const unsigned char *b64, size_t in_len, size_t *out_len) {
    init_b64_rev();
    size_t out_est = (in_len / 4) * 3 + 1;
    unsigned char *out = (unsigned char*)malloc(out_est + 1);
    if (!out) return NULL;

    size_t wi = 0;
    unsigned char quad[4];
    size_t qn = 0;

    for (size_t i = 0; i < in_len; ++i) {
        unsigned char ch = b64[i];
        if (isspace((unsigned char)ch)) continue;
        if (b64_rev[ch] == -1 && ch != '=') continue;
        quad[qn++] = ch;
        if (qn == 4) {
            int v0 = b64_rev[quad[0]];
            int v1 = b64_rev[quad[1]];
            int v2 = (quad[2] == '=') ? 0 : b64_rev[quad[2]];
            int v3 = (quad[3] == '=') ? 0 : b64_rev[quad[3]];
            unsigned triple = (v0 << 18) | (v1 << 12) | (v2 << 6) | v3;
            out[wi++] = (triple >> 16) & 0xFF;
            if (quad[2] != '=') out[wi++] = (triple >> 8) & 0xFF;
            if (quad[3] != '=') out[wi++] = triple & 0xFF;
            qn = 0;
        }
    }

    out[wi] = 0;
    if (out_len) *out_len = wi;
    return out;
}

void menu(void) {
    for (;;) {
        system("cls");
        printf("Base 64 Decode/Encode Tool\n");
        printf("Written by SilverWolf(SW84)\n");
        printf("1. Decode base64 (read encode.txt -> write decode.txt)\n");
        printf("2. Encode base64  (read decode.txt -> write encode.txt with wrapper)\n");
        printf("3. Exit\n");
        printf(">>>: ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("Invalid input\n");
            system("pause");
            continue;
        }

        if (choice == 1) {
            size_t in_size = 0;
            unsigned char *in = read_file("encode.txt", &in_size);
            if (!in) {
                printf("Failed to open encode.txt\n");
                system("pause");
                continue;
            }
            size_t out_size = 0;
            unsigned char *dec = base64_decode(in, in_size, &out_size);
            free(in);
            if (!dec) {
                printf("Decode failed\n");
                system("pause");
                continue;
            }
            if (write_file("decode.txt", dec, out_size) == 0) {
                printf("Wrote decode.txt (%zu bytes)\n", out_size);
            } else {
                printf("Failed to write decode.txt\n");
            }
            free(dec);
            system("pause");
        }
        else if (choice == 2) {
            size_t in_size = 0;
            unsigned char *in = read_file("decode.txt", &in_size);
            if (!in) {
                printf("Failed to open decode.txt\n");
                system("pause");
                continue;
            }
            size_t b64_len = 0;
            char *enc = base64_encode(in, in_size, &b64_len);
            free(in);
            if (!enc) {
                printf("Encode failed\n");
                system("pause");
                continue;
            }
            const char *prefix = "const encoded = \"";
            const char *suffix = "\";\n";
            size_t total_len = strlen(prefix) + b64_len + strlen(suffix);
            char *wrapped = (char*)malloc(total_len + 1);
            if (!wrapped) {
                free(enc);
                printf("Memory allocation failed\n");
                system("pause");
                continue;
            }
            memcpy(wrapped, prefix, strlen(prefix));
            memcpy(wrapped + strlen(prefix), enc, b64_len);
            memcpy(wrapped + strlen(prefix) + b64_len, suffix, strlen(suffix));
            wrapped[total_len] = '\0';
            if (write_file("encode.txt", (unsigned char*)wrapped, total_len) == 0) {
                printf("Wrote encode.txt with wrapper (%zu bytes)\n", total_len);
            } else {
                printf("Failed to write encode.txt\n");
            }
            free(enc);
            free(wrapped);
            system("pause");
        }
        else if (choice == 3) {
            puts("Bye.");
            system("pause");
            break;
        } else {
            printf("Unknown option\n");
            system("pause");
        }
    }
}

int main(void) {
    init_b64_rev();
    menu();
    return 0;
}
