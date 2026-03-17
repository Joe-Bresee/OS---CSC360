#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

uint16_t find_bytes_per_sector(FILE *fp) {
    uint16_t value;
    fseek(fp, 0x0B, SEEK_SET);
    fread(&value, 2, 1, fp);
    return value;
}

uint8_t find_sectors_per_cluster(FILE *fp) {
    uint8_t value;
    fseek(fp, 0x0D, SEEK_SET);
    fread(&value, 1, 1, fp);
    return value;
}

uint16_t find_reserved_sector_count(FILE *fp) {
    uint16_t value;
    fseek(fp, 0x0E, SEEK_SET);
    fread(&value, 2, 1, fp);
    return value;
}

uint8_t find_fat_count(FILE *fp) {
    uint8_t value;
    fseek(fp, 0x10, SEEK_SET);
    fread(&value, 1, 1, fp);
    return value;
}

uint16_t find_root_entry_count(FILE *fp) {
    uint16_t value;
    fseek(fp, 0x11, SEEK_SET);
    fread(&value, 2, 1, fp);
    return value;
}

uint16_t find_sectors_per_fat(FILE *fp) {
    uint16_t value;
    fseek(fp, 0x16, SEEK_SET);
    fread(&value, 2, 1, fp);
    return value;
}

uint32_t find_root_dir_start_bytes(FILE *fp) {
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    uint16_t reserved_sector_count = find_reserved_sector_count(fp);
    uint8_t fat_count = find_fat_count(fp);
    uint16_t sectors_per_fat = find_sectors_per_fat(fp);
    return (reserved_sector_count + (fat_count * sectors_per_fat)) * bytes_per_sector;
}

uint32_t find_data_start_bytes(FILE *fp) {
    uint32_t root_start = find_root_dir_start_bytes(fp);
    uint16_t root_entry_count = find_root_entry_count(fp);
    return root_start + (uint32_t)root_entry_count * 32;
}

uint32_t find_cluster_start_bytes(FILE *fp, uint16_t cluster) {
    uint32_t data_start = find_data_start_bytes(fp);
    uint8_t sectors_per_cluster = find_sectors_per_cluster(fp);
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    return data_start + (uint32_t)(cluster - 2) * sectors_per_cluster * bytes_per_sector;
}

uint16_t find_fat12_next_cluster(FILE *fp, uint16_t cluster) {
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    uint16_t reserved_sector_count = find_reserved_sector_count(fp);
    uint32_t fat_start = reserved_sector_count * bytes_per_sector;
    uint32_t offset = fat_start + (cluster * 3) / 2;
    uint16_t value;

    fseek(fp, offset, SEEK_SET);
    fread(&value, 2, 1, fp);

    if (cluster & 1) {
        return value >> 4;
    }
    return value & 0x0FFF;
}

void format_name(const uint8_t *entry, char *out, size_t out_len) {
    char name[9];
    char ext[4];
    memcpy(name, entry, 8);
    memcpy(ext, entry + 8, 3);
    name[8] = '\0';
    ext[3] = '\0';

    for (int i = 7; i >= 0; i--) {
        if (name[i] == ' ') name[i] = '\0';
        else break;
    }
    for (int i = 2; i >= 0; i--) {
        if (ext[i] == ' ') ext[i] = '\0';
        else break;
    }

    if (ext[0] != '\0') {
        snprintf(out, out_len, "%s.%s", name, ext);
    } else {
        snprintf(out, out_len, "%s", name);
    }
}

void uppercase_inplace(char *s) {
    for (; *s; s++) {
        *s = (char)toupper((unsigned char)*s);
    }
}

uint16_t find_file_cluster_in_root(FILE *fp, const char *filename, uint32_t *out_size) {
    uint32_t root_start = find_root_dir_start_bytes(fp);
    uint16_t root_entry_count = find_root_entry_count(fp);

    uint8_t entry[32];
    char entry_name[24];
    char target_name[24];

    snprintf(target_name, sizeof(target_name), "%s", filename);
    uppercase_inplace(target_name);

    fseek(fp, root_start, SEEK_SET);
    for (uint16_t i = 0; i < root_entry_count; i++) {
        fread(entry, 32, 1, fp);
        if (entry[0] == 0x00) break;
        if (entry[0] == 0xE5) continue;
        if (entry[11] == 0x0F) continue;

        uint8_t attr = entry[11];
        if (attr & 0x10) continue;

        format_name(entry, entry_name, sizeof(entry_name));
        uppercase_inplace(entry_name);

        if (strcmp(entry_name, target_name) == 0) {
            *out_size = *(uint32_t *)&entry[28];
            return *(uint16_t *)&entry[26];
        }
    }

    return 0;
}

int copy_file_from_cluster_chain(FILE *fp, FILE *out, uint16_t start_cluster, uint32_t size) {
    uint32_t bytes_per_cluster = (uint32_t)find_bytes_per_sector(fp) * find_sectors_per_cluster(fp);
    uint32_t remaining = size;
    uint16_t cluster = start_cluster;

    uint8_t buffer[4096];
    while (remaining > 0 && cluster < 0xFF8) {
        uint32_t to_read = bytes_per_cluster;
        if (to_read > remaining) to_read = remaining;

        uint32_t cluster_start = find_cluster_start_bytes(fp, cluster);
        fseek(fp, cluster_start, SEEK_SET);

        uint32_t read_so_far = 0;
        while (read_so_far < to_read) {
            uint32_t chunk = to_read - read_so_far;
            if (chunk > sizeof(buffer)) chunk = sizeof(buffer);
            fread(buffer, 1, chunk, fp);
            fwrite(buffer, 1, chunk, out);
            read_so_far += chunk;
        }

        remaining -= to_read;
        if (remaining == 0) break;
        cluster = find_fat12_next_cluster(fp, cluster);
    }

    return (remaining == 0) ? 0 : 1;
}

int main(int argc, char **argv) {
    FILE *fp;

    if (argc < 3) {
        printf("Usage: %s <disk.IMA> <FILENAME>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    uint32_t file_size = 0;
    uint16_t start_cluster = find_file_cluster_in_root(fp, argv[2], &file_size);

    if (start_cluster == 0 || start_cluster == 1) {
        fclose(fp);
        printf("File not found.\n");
        return 1;
    }

    FILE *out = fopen(argv[2], "wb");
    if (out == NULL) {
        fclose(fp);
        printf("Error creating output file\n");
        return 1;
    }

    if (copy_file_from_cluster_chain(fp, out, start_cluster, file_size) != 0) {
        fclose(out);
        fclose(fp);
        printf("Error reading file data\n");
        return 1;
    }

    fclose(out);
    fclose(fp);
    return 0;
}