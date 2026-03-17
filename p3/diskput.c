#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

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

uint16_t find_total_sectors(FILE *fp) {
    uint16_t value;
    fseek(fp, 0x13, SEEK_SET);
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

uint32_t find_root_dir_sectors(FILE *fp) {
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    uint16_t root_entry_count = find_root_entry_count(fp);
    uint32_t root_bytes = (uint32_t)root_entry_count * 32;
    return (root_bytes + bytes_per_sector - 1) / bytes_per_sector;
}

uint32_t find_data_start_bytes(FILE *fp) {
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    uint16_t reserved_sector_count = find_reserved_sector_count(fp);
    uint8_t fat_count = find_fat_count(fp);
    uint16_t sectors_per_fat = find_sectors_per_fat(fp);
    uint32_t root_sectors = find_root_dir_sectors(fp);
    return (reserved_sector_count + (fat_count * sectors_per_fat) + root_sectors) * bytes_per_sector;
}

uint32_t find_cluster_start_bytes(FILE *fp, uint16_t cluster) {
    uint32_t data_start = find_data_start_bytes(fp);
    uint8_t sectors_per_cluster = find_sectors_per_cluster(fp);
    uint16_t bytes_per_sector = find_bytes_per_sector(fp);
    return data_start + (uint32_t)(cluster - 2) * sectors_per_cluster * bytes_per_sector;
}

uint32_t find_cluster_count(FILE *fp) {
    uint16_t total_sectors = find_total_sectors(fp);
    uint16_t reserved_sector_count = find_reserved_sector_count(fp);
    uint8_t fat_count = find_fat_count(fp);
    uint16_t sectors_per_fat = find_sectors_per_fat(fp);
    uint32_t root_sectors = find_root_dir_sectors(fp);
    uint8_t sectors_per_cluster = find_sectors_per_cluster(fp);

    uint32_t data_sectors = total_sectors - (reserved_sector_count + (fat_count * sectors_per_fat) + root_sectors);
    return data_sectors / sectors_per_cluster;
}

void uppercase_inplace(char *s) {
    for (; *s; s++) {
        *s = (char)toupper((unsigned char)*s);
    }
}

void format_name_from_entry(const uint8_t *entry, char *out, size_t out_len) {
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

void build_short_name(const char *input, uint8_t *out11) {
    char name[9] = {0};
    char ext[4] = {0};
    const char *dot = strrchr(input, '.');

    if (dot != NULL && dot != input) {
        size_t nlen = dot - input;
        if (nlen > 8) nlen = 8;
        memcpy(name, input, nlen);
        strncpy(ext, dot + 1, 3);
    } else {
        strncpy(name, input, 8);
    }

    uppercase_inplace(name);
    uppercase_inplace(ext);

    memset(out11, ' ', 11);
    memcpy(out11, name, strlen(name));
    memcpy(out11 + 8, ext, strlen(ext));
}

uint16_t fat12_get(const uint8_t *fat, uint16_t cluster) {
    uint32_t offset = (cluster * 3) / 2;
    uint16_t value = *(uint16_t *)(fat + offset);
    if (cluster & 1) {
        return value >> 4;
    }
    return value & 0x0FFF;
}

void fat12_set(uint8_t *fat, uint16_t cluster, uint16_t value) {
    uint32_t offset = (cluster * 3) / 2;
    uint16_t current = *(uint16_t *)(fat + offset);

    if (cluster & 1) {
        current &= 0x000F;
        current |= (value << 4);
    } else {
        current &= 0xF000;
        current |= (value & 0x0FFF);
    }

    *(uint16_t *)(fat + offset) = current;
}

int find_dir_cluster(FILE *fp, const uint8_t *fat, const char *path, uint16_t *out_cluster, int *out_is_root) {
    if (path == NULL || path[0] == '\0') {
        *out_cluster = 0;
        *out_is_root = 1;
        return 1;
    }

    char temp[256];
    snprintf(temp, sizeof(temp), "%s", path);
    if (temp[0] == '/') {
        memmove(temp, temp + 1, strlen(temp));
    }

    char *saveptr = NULL;
    char *token = strtok_r(temp, "/", &saveptr);
    uint16_t current_cluster = 0;
    int is_root = 1;

    while (token != NULL) {
        char target[24];
        snprintf(target, sizeof(target), "%s", token);
        uppercase_inplace(target);

        int found = 0;

        if (is_root) {
            uint32_t root_start = find_root_dir_start_bytes(fp);
            uint16_t root_entry_count = find_root_entry_count(fp);
            fseek(fp, root_start, SEEK_SET);
            for (uint16_t i = 0; i < root_entry_count; i++) {
                uint8_t entry[32];
                fread(entry, 32, 1, fp);
                if (entry[0] == 0x00) break;
                if (entry[0] == 0xE5) continue;
                if (entry[11] == 0x0F) continue;
                if ((entry[11] & 0x10) == 0) continue;

                char entry_name[24];
                format_name_from_entry(entry, entry_name, sizeof(entry_name));
                uppercase_inplace(entry_name);

                if (strcmp(entry_name, target) == 0) {
                    current_cluster = *(uint16_t *)&entry[26];
                    if (current_cluster == 0 || current_cluster == 1) return 0;
                    found = 1;
                    break;
                }
            }
        } else {
            uint16_t cluster = current_cluster;
            while (cluster < 0xFF8) {
                uint32_t cluster_start = find_cluster_start_bytes(fp, cluster);
                uint32_t entries = (find_sectors_per_cluster(fp) * find_bytes_per_sector(fp)) / 32;
                fseek(fp, cluster_start, SEEK_SET);
                for (uint32_t i = 0; i < entries; i++) {
                    uint8_t entry[32];
                    fread(entry, 32, 1, fp);
                    if (entry[0] == 0x00) break;
                    if (entry[0] == 0xE5) continue;
                    if (entry[11] == 0x0F) continue;
                    if ((entry[11] & 0x10) == 0) continue;

                    char entry_name[24];
                    format_name_from_entry(entry, entry_name, sizeof(entry_name));
                    uppercase_inplace(entry_name);

                    if (strcmp(entry_name, target) == 0) {
                        current_cluster = *(uint16_t *)&entry[26];
                        if (current_cluster == 0 || current_cluster == 1) return 0;
                        found = 1;
                        break;
                    }
                }
                if (found) break;
                cluster = fat12_get(fat, cluster);
            }
        }

        if (!found) return 0;
        is_root = 0;
        token = strtok_r(NULL, "/", &saveptr);
    }

    *out_cluster = current_cluster;
    *out_is_root = is_root;
    return 1;
}

int find_free_dir_entry(FILE *fp, const uint8_t *fat, uint16_t dir_cluster, int is_root, uint32_t *out_offset) {
    if (is_root) {
        uint32_t root_start = find_root_dir_start_bytes(fp);
        uint16_t root_entry_count = find_root_entry_count(fp);
        fseek(fp, root_start, SEEK_SET);
        for (uint16_t i = 0; i < root_entry_count; i++) {
            uint8_t entry[32];
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00 || entry[0] == 0xE5) {
                *out_offset = root_start + (uint32_t)i * 32;
                return 1;
            }
        }
        return 0;
    }

    uint16_t cluster = dir_cluster;
    while (cluster < 0xFF8) {
        uint32_t cluster_start = find_cluster_start_bytes(fp, cluster);
        uint32_t entries = (find_sectors_per_cluster(fp) * find_bytes_per_sector(fp)) / 32;
        fseek(fp, cluster_start, SEEK_SET);
        for (uint32_t i = 0; i < entries; i++) {
            uint8_t entry[32];
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00 || entry[0] == 0xE5) {
                *out_offset = cluster_start + i * 32;
                return 1;
            }
        }
        cluster = fat12_get(fat, cluster);
    }

    return 0;
}

int file_exists_in_dir(FILE *fp, const uint8_t *fat, uint16_t dir_cluster, int is_root, const char *target_name) {
    char target[24];
    snprintf(target, sizeof(target), "%s", target_name);
    uppercase_inplace(target);

    if (is_root) {
        uint32_t root_start = find_root_dir_start_bytes(fp);
        uint16_t root_entry_count = find_root_entry_count(fp);
        fseek(fp, root_start, SEEK_SET);
        for (uint16_t i = 0; i < root_entry_count; i++) {
            uint8_t entry[32];
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00) break;
            if (entry[0] == 0xE5) continue;
            if (entry[11] == 0x0F) continue;
            if (entry[11] & 0x10) continue;

            char entry_name[24];
            format_name_from_entry(entry, entry_name, sizeof(entry_name));
            uppercase_inplace(entry_name);

            if (strcmp(entry_name, target) == 0) return 1;
        }
        return 0;
    }

    uint16_t cluster = dir_cluster;
    while (cluster < 0xFF8) {
        uint32_t cluster_start = find_cluster_start_bytes(fp, cluster);
        uint32_t entries = (find_sectors_per_cluster(fp) * find_bytes_per_sector(fp)) / 32;
        fseek(fp, cluster_start, SEEK_SET);
        for (uint32_t i = 0; i < entries; i++) {
            uint8_t entry[32];
            fread(entry, 32, 1, fp);
            if (entry[0] == 0x00) return 0;
            if (entry[0] == 0xE5) continue;
            if (entry[11] == 0x0F) continue;
            if (entry[11] & 0x10) continue;

            char entry_name[24];
            format_name_from_entry(entry, entry_name, sizeof(entry_name));
            uppercase_inplace(entry_name);

            if (strcmp(entry_name, target) == 0) return 1;
        }
        cluster = fat12_get(fat, cluster);
    }

    return 0;
}

void fat_time_date(time_t t, uint16_t *out_time, uint16_t *out_date) {
    struct tm *tm_info = localtime(&t);
    int year = tm_info->tm_year + 1900;
    if (year < 1980) year = 1980;

    uint16_t date = (uint16_t)(((year - 1980) << 9) | ((tm_info->tm_mon + 1) << 5) | tm_info->tm_mday);
    uint16_t time = (uint16_t)((tm_info->tm_hour << 11) | (tm_info->tm_min << 5) | (tm_info->tm_sec / 2));

    *out_date = date;
    *out_time = time;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <disk.IMA> <path/to/file>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(argv[2], &st) != 0) {
        printf("File not found.\n");
        return 1;
    }

    FILE *img = fopen(argv[1], "r+b");
    if (img == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    uint16_t bytes_per_sector = find_bytes_per_sector(img);
    uint16_t sectors_per_fat = find_sectors_per_fat(img);
    uint32_t fat_size = (uint32_t)sectors_per_fat * bytes_per_sector;
    uint8_t *fat = (uint8_t *)malloc(fat_size);
    if (fat == NULL) {
        fclose(img);
        printf("Error allocating FAT buffer\n");
        return 1;
    }

    uint32_t fat_start = find_reserved_sector_count(img) * bytes_per_sector;
    fseek(img, fat_start, SEEK_SET);
    fread(fat, 1, fat_size, img);

    char path_copy[256];
    snprintf(path_copy, sizeof(path_copy), "%s", argv[2]);
    char *basename = strrchr(path_copy, '/');
    char *dirpath = NULL;
    if (basename != NULL) {
        *basename = '\0';
        basename++;
        dirpath = path_copy;
    } else {
        basename = path_copy;
        dirpath = NULL;
    }

    uint16_t dir_cluster = 0;
    int dir_is_root = 1;
    if (dirpath != NULL && dirpath[0] != '\0') {
        if (!find_dir_cluster(img, fat, dirpath, &dir_cluster, &dir_is_root)) {
            free(fat);
            fclose(img);
            printf("The directory not found.\n");
            return 1;
        }
    }

    if (file_exists_in_dir(img, fat, dir_cluster, dir_is_root, basename)) {
        free(fat);
        fclose(img);
        printf("File already exists.\n");
        return 1;
    }

    uint32_t bytes_per_cluster = (uint32_t)bytes_per_sector * find_sectors_per_cluster(img);
    uint32_t needed_clusters = (st.st_size + bytes_per_cluster - 1) / bytes_per_cluster;

    uint32_t total_clusters = find_cluster_count(img);
    uint16_t *alloc = (uint16_t *)malloc(sizeof(uint16_t) * needed_clusters);
    if (alloc == NULL) {
        free(fat);
        fclose(img);
        printf("Error allocating cluster list\n");
        return 1;
    }

    uint32_t free_found = 0;
    for (uint16_t c = 2; c < total_clusters + 2 && free_found < needed_clusters; c++) {
        if (fat12_get(fat, c) == 0x000) {
            alloc[free_found++] = c;
        }
    }

    if (free_found < needed_clusters) {
        free(alloc);
        free(fat);
        fclose(img);
        printf("No enough free space in the disk image.\n");
        return 1;
    }

    uint32_t dir_entry_offset = 0;
    if (!find_free_dir_entry(img, fat, dir_cluster, dir_is_root, &dir_entry_offset)) {
        free(alloc);
        free(fat);
        fclose(img);
        printf("No enough free space in the disk image.\n");
        return 1;
    }

    for (uint32_t i = 0; i < needed_clusters; i++) {
        uint16_t next = (i + 1 < needed_clusters) ? alloc[i + 1] : 0xFFF;
        fat12_set(fat, alloc[i], next);
    }

    for (uint8_t i = 0; i < find_fat_count(img); i++) {
        uint32_t fat_copy_start = fat_start + i * fat_size;
        fseek(img, fat_copy_start, SEEK_SET);
        fwrite(fat, 1, fat_size, img);
    }

    FILE *in = fopen(argv[2], "rb");
    if (in == NULL) {
        free(alloc);
        free(fat);
        fclose(img);
        printf("File not found.\n");
        return 1;
    }

    uint8_t buffer[4096];
    uint32_t remaining = st.st_size;
    for (uint32_t i = 0; i < needed_clusters; i++) {
        uint16_t cluster = alloc[i];
        uint32_t cluster_start = find_cluster_start_bytes(img, cluster);
        fseek(img, cluster_start, SEEK_SET);

        uint32_t to_write = bytes_per_cluster;
        if (to_write > remaining) to_write = remaining;

        uint32_t written = 0;
        while (written < to_write) {
            uint32_t chunk = to_write - written;
            if (chunk > sizeof(buffer)) chunk = sizeof(buffer);
            size_t r = fread(buffer, 1, chunk, in);
            if (r == 0) break;
            fwrite(buffer, 1, r, img);
            written += r;
        }
        remaining -= to_write;
        if (remaining == 0) break;
    }

    fclose(in);

    uint8_t entry[32];
    memset(entry, 0, sizeof(entry));
    build_short_name(basename, entry);
    entry[11] = 0x20;

    uint16_t ctime, cdate;
    fat_time_date(st.st_mtime, &ctime, &cdate);
    *(uint16_t *)&entry[14] = ctime;
    *(uint16_t *)&entry[16] = cdate;
    *(uint16_t *)&entry[18] = cdate;
    *(uint16_t *)&entry[22] = ctime;
    *(uint16_t *)&entry[24] = cdate;
    *(uint16_t *)&entry[26] = alloc[0];
    *(uint32_t *)&entry[28] = (uint32_t)st.st_size;

    fseek(img, dir_entry_offset, SEEK_SET);
    fwrite(entry, 1, 32, img);

    free(alloc);
    free(fat);
    fclose(img);
    return 0;
}