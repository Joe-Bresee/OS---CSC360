#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

void format_datetime(uint16_t date, uint16_t time, char *out, size_t out_len) {
    // bitwise operations to shift to desired time attribute
    int day = date & 0x1F;
    int month = (date >> 5) & 0x0F;
    int year = 1980 + ((date >> 9) & 0x7F);
    int sec = (time & 0x1F) * 2;
    int min = (time >> 5) & 0x3F;
    int hour = (time >> 11) & 0x1F;

    snprintf(out, out_len, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
}

void list_directory(FILE *fp, uint16_t start_cluster, const char *dir_name, int is_root) {
    uint8_t entry[32];
    char name_buf[24];
    char datetime_buf[32];

    printf("%s\n==================\n", dir_name);

    if (is_root) {
        uint32_t root_start = find_root_dir_start_bytes(fp);
        uint16_t root_entry_count = find_root_entry_count(fp);

        fseek(fp, root_start, SEEK_SET);
        for (uint16_t i = 0; i < root_entry_count; i++) {
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00) break;
            if (entry[0] == 0xE5) continue;
            if (entry[11] == 0x0F) continue;

            uint16_t cluster = *(uint16_t *)&entry[26];
            if (cluster == 0 || cluster == 1) continue;

            uint8_t attr = entry[11];
            uint32_t size = *(uint32_t *)&entry[28];
            uint16_t time = *(uint16_t *)&entry[14];
            uint16_t date = *(uint16_t *)&entry[16];

            format_name(entry, name_buf, sizeof(name_buf));
            format_datetime(date, time, datetime_buf, sizeof(datetime_buf));

            printf("%c %10u %-20s %s\n", (attr & 0x10) ? 'D' : 'F', size, name_buf, datetime_buf);

            if (attr & 0x10) {
                list_directory(fp, cluster, name_buf, 0);
            }
        }
        return;
    }

    uint16_t cluster = start_cluster;
    while (cluster < 0xFF8) {
        uint32_t cluster_start = find_cluster_start_bytes(fp, cluster);
        uint16_t bytes_per_sector = find_bytes_per_sector(fp);
        uint8_t sectors_per_cluster = find_sectors_per_cluster(fp);
        uint32_t entries = (bytes_per_sector * sectors_per_cluster) / 32;

        fseek(fp, cluster_start, SEEK_SET);
        for (uint32_t i = 0; i < entries; i++) {
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00) return;
            if (entry[0] == 0xE5) continue;
            if (entry[11] == 0x0F) continue;

            uint16_t entry_cluster = *(uint16_t *)&entry[26];
            if (entry_cluster == 0 || entry_cluster == 1) continue;

            uint8_t attr = entry[11];
            uint32_t size = *(uint32_t *)&entry[28];
            uint16_t time = *(uint16_t *)&entry[14];
            uint16_t date = *(uint16_t *)&entry[16];

            format_name(entry, name_buf, sizeof(name_buf));
            format_datetime(date, time, datetime_buf, sizeof(datetime_buf));

            printf("%c %10u %-20s %s\n", (attr & 0x10) ? 'D' : 'F', size, name_buf, datetime_buf);

            if (attr & 0x10) {
                list_directory(fp, entry_cluster, name_buf, 0);
            }
        }

        cluster = find_fat12_next_cluster(fp, cluster);
    }
}

int main(int argc, char **argv) {
    FILE *fp;

    if (argc < 2) {
        printf("Usage: %s <disk.IMA>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    list_directory(fp, 0, "ROOT", 1);
    fclose(fp);

    return 0;
}