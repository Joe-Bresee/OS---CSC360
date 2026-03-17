#include <stdio.h>
#include <stdint.h>

char* find_os_name(FILE *fp) {
    static char osname[9];

    fseek(fp, 3, SEEK_SET);
    fread(osname, 1, 8, fp);
    osname[8] = '\0';
    return osname;
}

char *find_disk_label(FILE *fp) {
    static char disk_label[12];
    fseek(fp, 0x2B, SEEK_SET);
    fread(disk_label, 1, 11, fp);
    disk_label[11] = '\0';

    // remove padding at end
    for (int i = 10; i >0; i--) {
        if (disk_label[i] == ' ') disk_label[i] = '\0';
        else break;
    }
    return disk_label;
}

int find_total_size_of_disk(FILE *fp) {
    uint16_t total_sectors, bytes_per_sector;

    fseek(fp, 0x0B, SEEK_SET);
    fread(&bytes_per_sector, 2, 1, fp);

    fseek(fp, 0x13, SEEK_SET);
    fread(&total_sectors, 2, 1, fp);

    return total_sectors * bytes_per_sector;

}

int find_free_size_of_disk(FILE *fp) {
    // count unused clusters (FAT entries w val 0x000 in FAT table.)
    // free size = unused cluster count x sectorspercluster x bytespersector
    uint16_t sectors_per_cluster, bytes_per_sector, table_size;
    uint16_t total_sectors, reserved_sector_count;
    uint8_t table_count;

    fseek(fp, 0x0B, SEEK_SET); fread(&bytes_per_sector, 2, 1, fp);
    fseek(fp, 0x0D, SEEK_SET); fread(&sectors_per_cluster, 1, 1, fp);
    fseek(fp, 0x10, SEEK_SET); fread(&table_count, 1, 1, fp);
    fseek(fp, 0x16, SEEK_SET); fread(&table_size, 2, 1, fp);
    fseek(fp, 0x0E, SEEK_SET); fread(&reserved_sector_count, 2, 1, fp);
    fseek(fp, 0x13, SEEK_SET); fread(&total_sectors, 2, 1, fp);

    uint16_t data_sector_count = total_sectors - (reserved_sector_count + table_count * table_size);
    uint16_t cluster_count = data_sector_count / sectors_per_cluster;

    uint16_t fat_start = reserved_sector_count * bytes_per_sector;
    uint16_t entry;

    uint16_t free_count = 0;

    fseek(fp, fat_start + 4, SEEK_SET); //skip first 2 entries
    for (uint16_t i = 2; i < cluster_count + 2; i++) {
        fread(&entry, 2, 1, fp);
        if (entry == 0x0000) free_count++;
    }

    return free_count * sectors_per_cluster * bytes_per_sector;
}

int find_num_files(FILE *fp) {
    // if attr != 0x10 and first byte != 0xE5/0x00, increment file count
    // perform recursively down all subdirs
    uint16_t root_entry_count, bytes_per_sector, sectors_per_cluster, reserved_sector_count, table_size;
    uint8_t table_count;

    fseek(fp, 0x11, SEEK_SET); fread(&root_entry_count, 2, 1, fp);
    fseek(fp, 0x0B, SEEK_SET); fread(&bytes_per_sector, 2, 1, fp);
    fseek(fp, 0x0D, SEEK_SET); fread(&sectors_per_cluster, 1, 1, fp);
    fseek(fp, 0x0E, SEEK_SET); fread(&reserved_sector_count, 2, 1, fp);
    fseek(fp, 0x10, SEEK_SET); fread(&table_count, 1, 1, fp);
    fseek(fp, 0x16, SEEK_SET); fread(&table_size, 2, 1, fp);

    uint16_t root_dir_start = (reserved_sector_count + table_count * table_size) * bytes_per_sector;
    uint8_t entry[32];
    int count = 0;

    fseek(fp, root_dir_start, SEEK_SET);
    for (int i = 0; i < root_entry_count; i++) {
        fread(entry, 32, 1, fp);
        if (entry[0] == 0x00) break;
        if (entry[0] == 0xE5) continue;
        if ((entry[11] & 0x10) == 0x10) continue;
        uint16_t cluster = *(uint16_t*)&entry[26];
        if (cluster == 0 || cluster == 1) continue;
        count++;
    }
    return count;
}

int find_num_FAT_copies(FILE *fp) {
    uint8_t fat_count;
    fseek(fp, 0x10, SEEK_SET);
    fread(&fat_count, 1, 1, fp);
    return fat_count;
}

int find_sectors_per_FAT(FILE *fp) {
    uint16_t table_size_16;
    fseek(fp, 0x16, SEEK_SET);
    fread(&table_size_16, 2, 1, fp);
    return table_size_16;
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

    printf("OS Name: %s\n", find_os_name(fp));
    printf("Total size of disk: %d\n", find_total_size_of_disk(fp));
    printf("Free size of the disk: %d\n", find_free_size_of_disk(fp));
    printf("==============\n");
    printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n", find_num_files(fp));
    printf("==============\n");
    printf("Number of FAT copies: %d\n", find_num_FAT_copies(fp));
    printf("Sectors per FAT: %d\n", find_sectors_per_FAT(fp));

    return 0;
}