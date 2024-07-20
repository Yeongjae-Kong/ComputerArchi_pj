#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int cap = 0;
int assoc = 0;
int block = 0;

int total_W = 0;
int total_R = 0;

int l1_rd_miss = 0;
int l1_wr_miss = 0;
int l1_clean_eviction = 0;
int l1_dirty_eviction = 0;

int l2_rd_access = 0;
int l2_wr_access = 0;
int l2_rd_miss = 0;
int l2_wr_miss = 0;

int l2_clean_evict = 0;
int l2_dirty_evict = 0;

typedef struct {
    unsigned long long int tagbit;
    int dirtybit;
    int referbit;
} cache;

int power_two(int exponent) {
    return 1 << exponent;
}

int log_two(int n) {
    int result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

unsigned long long int calculate_evict_l1_tagbit(unsigned long long int tagbit, int block_bit, int l1_index_bit, int l1_index) {
    return (tagbit << (block_bit + l1_index_bit)) | ((unsigned long long int)l1_index << block_bit);
}

unsigned long long int calculate_evict_l2_tagbit(unsigned long long int address, int block_bit, int l2_index_bit) {
    return address >> (block_bit + l2_index_bit);
}

int calculate_evict_l1_index(unsigned long long int address, int block_bit, int l1_tagbit_bit, int l1_index_bit) {
    return (address << l1_tagbit_bit) >> (64 - l1_index_bit);
}

void update_l1_cache(cache L1[][16], int l1_index, int way, unsigned long long int tagbit, int dirtybit, int referbit) {
    L1[l1_index][way].tagbit = tagbit;
    L1[l1_index][way].dirtybit = dirtybit;
    L1[l1_index][way].referbit = referbit;
}

void update_l2_cache(cache L2[][16], int l2_index, int way, unsigned long long int tagbit, int dirtybit, int referbit) {
    L2[l2_index][way].tagbit = tagbit;
    L2[l2_index][way].dirtybit = dirtybit;
    L2[l2_index][way].referbit = referbit;
}

void process_write(cache L1[][16], cache L2[][16], int l1_index, unsigned long long int l1_tagbit, int l2_index, unsigned long long int l2_tagbit, int l1_assoc, int assoc, int block_bit, int l1_tagbit_bit, int l1_index_bit, int l2_index_bit, int replace) {
    int l1_hit_flag = 0, l2_hit_flag = 0, l1_find_flag = 0, l2_empty_flag = 0;
    int i = 0, j, max_assoc, max_lru, random_assoc;
    unsigned long long int evict_l1_tagbit, evict_address, evict_l2_tagbit, evict_tagbit, evict_l1_index;

    i = 0;
    while (i < l1_assoc) {
        if (l1_tagbit == L1[l1_index][i].tagbit) {
            l1_hit_flag = 1;
            update_l1_cache(L1, l1_index, i, l1_tagbit, 1, 0);
            break;
        }
        else if (L1[l1_index][i].tagbit != 0)
            L1[l1_index][i].referbit++;
        i++;
    }

    if (!l1_hit_flag) {
        l1_wr_miss++;
        l2_wr_access++;

        i = 0;
        while (i < assoc) {
            if (l2_tagbit == L2[l2_index][i].tagbit) {
                l2_hit_flag = 1;
                update_l2_cache(L2, l2_index, i, l2_tagbit, 1, 0);
                break;
            }
            else if (L2[l2_index][i].tagbit != 0)
                L2[l2_index][i].referbit++;
            i++;
        }

        i = 0;
        while (i < l1_assoc) {
            if (L1[l1_index][i].tagbit == 0) {
                l1_find_flag = 1;
                update_l1_cache(L1, l1_index, i, l1_tagbit, 0, 0);
                break;
            }
            i++;
        }

        if (!l1_find_flag) {
            switch (replace) {
            case -1:
                random_assoc = rand() % l1_assoc;
                if (L1[l1_index][random_assoc].dirtybit) {
                    l1_dirty_eviction++;
                    evict_l1_tagbit = L1[l1_index][random_assoc].tagbit;
                    evict_address = calculate_evict_l1_tagbit(evict_l1_tagbit, block_bit, l1_index_bit, l1_index);
                    evict_l2_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l2_index_bit);

                    j = 0;
                    while (j < assoc) {
                        if (evict_l2_tagbit == L2[l2_index][j].tagbit) {
                            update_l2_cache(L2, l2_index, j, L2[l2_index][j].tagbit, 1, L2[l2_index][j].referbit);
                            break;
                        }
                        j++;
                    }
                }
                else
                    l1_clean_eviction++;
                update_l1_cache(L1, l1_index, random_assoc, l1_tagbit, 0, 0);
                break;
            case 1:
                max_lru = 0;
                i = 0;
                while (i < l1_assoc) {
                    if (L1[l1_index][i].referbit > max_lru) {
                        max_assoc = i;
                        max_lru = L1[l1_index][i].referbit;
                    }
                    i++;
                }
                if (L1[l1_index][max_assoc].dirtybit) {
                    l1_dirty_eviction++;
                    evict_l1_tagbit = L1[l1_index][max_assoc].tagbit;
                    evict_address = calculate_evict_l1_tagbit(evict_l1_tagbit, block_bit, l1_index_bit, l1_index);
                    evict_l2_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l2_index_bit);

                    j = 0;
                    while (j < assoc) {
                        if (evict_l2_tagbit == L2[l2_index][j].tagbit) {
                            update_l2_cache(L2, l2_index, j, L2[l2_index][j].tagbit, 1, L2[l2_index][j].referbit);
                            break;
                        }
                        j++;
                    }
                }
                else
                    l1_clean_eviction++;
                update_l1_cache(L1, l1_index, max_assoc, l1_tagbit, 0, 0);
                break;
            }
        }
    }

    if (!l1_hit_flag && !l2_hit_flag) {
        l2_wr_miss++;
        i = 0;
        while (i < assoc) {
            if (L2[l2_index][i].tagbit == 0) {
                update_l2_cache(L2, l2_index, i, l2_tagbit, 0, 0);
                l2_empty_flag = 1;
                break;
            }
            i++;
        }

        if (!l2_empty_flag) {
            int l2_dirty_flag = 0;
            switch (replace) {
            case -1:
                random_assoc = rand() % assoc;
                if (L2[l2_index][random_assoc].dirtybit) {
                    l2_dirty_evict++;
                    l2_dirty_flag = 1;
                }
                else
                    l2_clean_evict++;

                evict_tagbit = L2[l2_index][random_assoc].tagbit;
                evict_address = calculate_evict_l1_tagbit(evict_tagbit, block_bit, l2_index_bit, l2_index);
                evict_l1_index = calculate_evict_l1_index(evict_address, block_bit, l1_tagbit_bit, l1_index_bit);
                evict_l1_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l1_index_bit);

                update_l2_cache(L2, l2_index, random_assoc, l2_tagbit, 0, 0);

                j = 0;
                while (j < l1_assoc) {
                    if (L1[evict_l1_index][j].tagbit == evict_l1_tagbit) {
                        if (L1[evict_l1_index][j].dirtybit) {
                            l1_dirty_eviction++;
                            if (!l2_dirty_flag) {
                                l2_clean_evict--;
                                l2_dirty_evict++;
                            }
                        }
                        else
                            l1_clean_eviction++;
                        update_l1_cache(L1, evict_l1_index, j, 0, 0, 0);
                        break;
                    }
                    j++;
                }
                break;
            case 1:
                max_lru = 0;
                l2_dirty_flag = 0;
                i = 0;
                while (i < assoc) {
                    if (L2[l2_index][i].referbit > max_lru) {
                        max_assoc = i;
                        max_lru = L2[l2_index][i].referbit;
                    }
                    i++;
                }
                if (L2[l2_index][max_assoc].dirtybit) {
                    l2_dirty_evict++;
                    l2_dirty_flag = 1;
                }
                else
                    l2_clean_evict++;

                evict_tagbit = L2[l2_index][max_assoc].tagbit;
                evict_address = calculate_evict_l1_tagbit(evict_tagbit, block_bit, l2_index_bit, l2_index);
                evict_l1_index = calculate_evict_l1_index(evict_address, block_bit, l1_tagbit_bit, l1_index_bit);
                evict_l1_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l1_index_bit);

                update_l2_cache(L2, l2_index, max_assoc, l2_tagbit, 0, 0);

                j = 0;
                while (j < l1_assoc) {
                    if (L1[evict_l1_index][j].tagbit == evict_l1_tagbit) {
                        if (L1[evict_l1_index][j].dirtybit) {
                            l1_dirty_eviction++;
                            if (!l2_dirty_flag) {
                                l2_clean_evict--;
                                l2_dirty_evict++;
                            }
                        }
                        else
                            l1_clean_eviction++;
                        update_l1_cache(L1, evict_l1_index, j, 0, 0, 0);
                        break;
                    }
                    j++;
                }
                break;
            }
        }
    }
}

void process_read(cache L1[][16], cache L2[][16], int l1_index, unsigned long long int l1_tagbit, int l2_index, unsigned long long int l2_tagbit, int l1_assoc, int assoc, int block_bit, int l1_tagbit_bit, int l1_index_bit, int l2_index_bit, int replace) {
    int l1_hit_flag = 0, l2_hit_flag = 0, l1_find_flag = 0, l2_empty_flag = 0;
    int i = 0, j, max_assoc, max_lru, random_assoc;
    unsigned long long int evict_l1_tagbit, evict_address, evict_l2_tagbit, evict_tagbit, evict_l1_index;

    i = 0;
    while (i < l1_assoc) {
        if (l1_tagbit == L1[l1_index][i].tagbit) {
            l1_hit_flag = 1;
            update_l1_cache(L1, l1_index, i, l1_tagbit, L1[l1_index][i].dirtybit, 0);
            break;
        }
        else if (L1[l1_index][i].tagbit != 0)
            L1[l1_index][i].referbit++;
        i++;
    }

    if (!l1_hit_flag) {
        l1_rd_miss++;
        l2_rd_access++;

        i = 0;
        while (i < assoc) {
            if (l2_tagbit == L2[l2_index][i].tagbit) {
                l2_hit_flag = 1;
                update_l2_cache(L2, l2_index, i, l2_tagbit, L2[l2_index][i].dirtybit, 0);
                break;
            }
            else if (L2[l2_index][i].tagbit != 0)
                L2[l2_index][i].referbit++;
            i++;
        }

        i = 0;
        while (i < l1_assoc) {
            if (L1[l1_index][i].tagbit == 0) {
                l1_find_flag = 1;
                update_l1_cache(L1, l1_index, i, l1_tagbit, 0, 0);
                break;
            }
            i++;
        }

        if (!l1_find_flag) {
            switch (replace) {
            case -1:
                random_assoc = rand() % l1_assoc;
                if (L1[l1_index][random_assoc].dirtybit) {
                    l1_dirty_eviction++;
                    evict_l1_tagbit = L1[l1_index][random_assoc].tagbit;
                    evict_address = calculate_evict_l1_tagbit(evict_l1_tagbit, block_bit, l1_index_bit, l1_index);
                    evict_l2_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l2_index_bit);

                    j = 0;
                    while (j < assoc) {
                        if (evict_l2_tagbit == L2[l2_index][j].tagbit) {
                            update_l2_cache(L2, l2_index, j, L2[l2_index][j].tagbit, 1, L2[l2_index][j].referbit);
                            break;
                        }
                        j++;
                    }
                }
                else
                    l1_clean_eviction++;
                update_l1_cache(L1, l1_index, random_assoc, l1_tagbit, 0, 0);
                break;
            case 1:
                max_lru = 0;
                i = 0;
                while (i < l1_assoc) {
                    if (L1[l1_index][i].referbit > max_lru) {
                        max_assoc = i;
                        max_lru = L1[l1_index][i].referbit;
                    }
                    i++;
                }
                if (L1[l1_index][max_assoc].dirtybit) {
                    l1_dirty_eviction++;
                    evict_l1_tagbit = L1[l1_index][max_assoc].tagbit;
                    evict_address = calculate_evict_l1_tagbit(evict_l1_tagbit, block_bit, l1_index_bit, l1_index);
                    evict_l2_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l2_index_bit);

                    j = 0;
                    while (j < assoc) {
                        if (evict_l2_tagbit == L2[l2_index][j].tagbit) {
                            update_l2_cache(L2, l2_index, j, L2[l2_index][j].tagbit, 1, L2[l2_index][j].referbit);
                            break;
                        }
                        j++;
                    }
                }
                else
                    l1_clean_eviction++;
                update_l1_cache(L1, l1_index, max_assoc, l1_tagbit, 0, 0);
                break;
            }
        }
    }

    if (!l1_hit_flag && !l2_hit_flag) {
        l2_rd_miss++;
        i = 0;
        while (i < assoc) {
            if (L2[l2_index][i].tagbit == 0) {
                update_l2_cache(L2, l2_index, i, l2_tagbit, 0, 0);
                l2_empty_flag = 1;
                break;
            }
            i++;
        }

        if (!l2_empty_flag) {
            int l2_dirty_flag = 0;
            switch (replace) {
            case -1:
                random_assoc = rand() % assoc;
                if (L2[l2_index][random_assoc].dirtybit) {
                    l2_dirty_evict++;
                    l2_dirty_flag = 1;
                }
                else
                    l2_clean_evict++;

                evict_tagbit = L2[l2_index][random_assoc].tagbit;
                evict_address = calculate_evict_l1_tagbit(evict_tagbit, block_bit, l2_index_bit, l2_index);
                evict_l1_index = calculate_evict_l1_index(evict_address, block_bit, l1_tagbit_bit, l1_index_bit);
                evict_l1_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l1_index_bit);

                update_l2_cache(L2, l2_index, random_assoc, l2_tagbit, 0, 0);

                j = 0;
                while (j < l1_assoc) {
                    if (L1[evict_l1_index][j].tagbit == evict_l1_tagbit) {
                        if (L1[evict_l1_index][j].dirtybit) {
                            l1_dirty_eviction++;
                            if (!l2_dirty_flag) {
                                l2_clean_evict--;
                                l2_dirty_evict++;
                            }
                        }
                        else
                            l1_clean_eviction++;
                        update_l1_cache(L1, evict_l1_index, j, 0, 0, 0);
                        break;
                    }
                    j++;
                }
                break;
            case 1:
                max_lru = 0;
                l2_dirty_flag = 0;
                i = 0;
                while (i < assoc) {
                    if (L2[l2_index][i].referbit > max_lru) {
                        max_assoc = i;
                        max_lru = L2[l2_index][i].referbit;
                    }
                    i++;
                }
                if (L2[l2_index][max_assoc].dirtybit) {
                    l2_dirty_evict++;
                    l2_dirty_flag = 1;
                }
                else
                    l2_clean_evict++;

                evict_tagbit = L2[l2_index][max_assoc].tagbit;
                evict_address = calculate_evict_l1_tagbit(evict_tagbit, block_bit, l2_index_bit, l2_index);
                evict_l1_index = calculate_evict_l1_index(evict_address, block_bit, l1_tagbit_bit, l1_index_bit);
                evict_l1_tagbit = calculate_evict_l2_tagbit(evict_address, block_bit, l1_index_bit);

                update_l2_cache(L2, l2_index, max_assoc, l2_tagbit, 0, 0);

                j = 0;
                while (j < l1_assoc) {
                    if (L1[evict_l1_index][j].tagbit == evict_l1_tagbit) {
                        if (L1[evict_l1_index][j].dirtybit) {
                            l1_dirty_eviction++;
                            if (!l2_dirty_flag) {
                                l2_clean_evict--;
                                l2_dirty_evict++;
                            }
                        }
                        else
                            l1_clean_eviction++;
                        update_l1_cache(L1, evict_l1_index, j, 0, 0, 0);
                        break;
                    }
                    j++;
                }
                break;
            }
        }
    }
}

void write_output_file(char* argv[], int argc, int cap, int assoc, int block, int l1_cap, int l1_assoc, int total_R, int total_W, int l1_rd_miss, int l2_rd_miss, int l1_wr_miss, int l2_wr_miss, int l2_rd_access, int l2_wr_access, int l1_clean_eviction, int l2_clean_evict, int l1_dirty_eviction, int l2_dirty_evict) {
    char* point = argv[argc - 1];
    char* ext = strrchr(point, '.');
    if (ext != NULL) {
        *ext = '\0';
    }
    char a[100], b[100];
    sprintf(a, "%s", point);
    sprintf(b, "_%d_%d_%d.out", cap / 1024, assoc, block);
    strcat(a, b);

    FILE* final_f = fopen(a, "w");
    fprintf(final_f, "-- General Stats -- \n");
    fprintf(final_f, "L1 cap: %d\n", l1_cap / 1024);
    fprintf(final_f, "L1 way: %d\n", l1_assoc);
    fprintf(final_f, "L2 cap: %d\n", cap / 1024);
    fprintf(final_f, "L2 way: %d\n", assoc);
    fprintf(final_f, "Block Size: %d\n", block);
    fprintf(final_f, "Total accesses: %d\n", total_R + total_W);
    fprintf(final_f, "Read accesses: %d\n", total_R);
    fprintf(final_f, "Write accesses: %d\n", total_W);
    fprintf(final_f, "L1 Read misses: %d \n", l1_rd_miss);
    fprintf(final_f, "L2 Read misses: %d \n", l2_rd_miss);
    fprintf(final_f, "L1 Write misses: %d \n", l1_wr_miss);
    fprintf(final_f, "L2 Write misses: %d \n", l2_wr_miss);
    fprintf(final_f, "L1 Read miss rate: %d%% \n", l1_rd_miss * 100 / total_R);
    fprintf(final_f, "L2 Read miss rate: %d%% \n", l2_rd_miss * 100 / l2_rd_access);
    fprintf(final_f, "L1 Write miss rate: %d%%\n", l1_wr_miss * 100 / total_W);
    fprintf(final_f, "L2 Write miss rate: %d%%\n", l2_wr_miss * 100 / l2_wr_access);
    fprintf(final_f, "L1 Clean eviction: %d \n", l1_clean_eviction);
    fprintf(final_f, "L2 Clean eviction: %d \n", l2_clean_evict);
    fprintf(final_f, "L1 dirty eviction: %d \n", l1_dirty_eviction);
    fprintf(final_f, "L2 dirty eviction: %d \n", l2_dirty_evict);
    fflush(final_f);
    fclose(final_f);
}

int get_counts(int cap, int block) {
    return cap / block;
}

int get_index(int counts, int assoc) {
    return counts / assoc;
}

int get_l1_cap(int cap) {
    return cap / 4;
}

int get_l1_assoc(int assoc) {
    return (assoc <= 2) ? assoc : assoc / 4;
}

int get_l1_counts(int l1_cap, int block) {
    return l1_cap / block;
}

int get_l1_index(int l1_counts, int l1_assoc) {
    return l1_counts / l1_assoc;
}

int get_block_bit(int block) {
    return log_two(block);
}

int get_l1_index_bit(int l1_index) {
    return log_two(l1_index);
}

int get_l1_tagbit_bit(int block_bit, int l1_index_bit) {
    return 64 - (block_bit + l1_index_bit);
}

int get_l2_index_bit(int index) {
    return log_two(index);
}

int get_l2_tagbit_bit(int block_bit, int l2_index_bit) {
    return 64 - (block_bit + l2_index_bit);
}

int main(int argc, char* argv[]) {
    FILE* f;
    int c = 0;
    int a = 0;
    int b = 0;
    int replace = 0;

    for (int x = 1; x < argc; x++) {
        if (strstr(argv[x], "-c") != NULL) {
            c = 1;
            cap = strtol(argv[++x], NULL, 10) * 1024;
        }
        else if (strstr(argv[x], "-a") != NULL) {
            a = 1;
            assoc = strtol(argv[++x], NULL, 10);
        }
        else if (strstr(argv[x], "-b") != NULL) {
            b = 1;
            block = strtol(argv[++x], NULL, 10);
        }
        else if (strstr(argv[x], "-lru") != NULL) {
            replace = 1;
        }
        else if (strstr(argv[x], "-random") != NULL) {
            replace = -1;
        }
        else {
            f = fopen(argv[x], "r");
        }
    }

    if (!(c && a && b && replace)) {
        printf("command = ./runfile <-c cap> <-a assoc> <-b block> <-lru 또는 -random> <tracefile>\n");
        return 0;
    }

    int counts = get_counts(cap, block);
    int index = get_index(counts, assoc);
    int l1_cap = get_l1_cap(cap);
    int l1_assoc = get_l1_assoc(assoc);
    int l1_counts = get_l1_counts(l1_cap, block);
    int l1_index = get_l1_index(l1_counts, l1_assoc);
    cache L1[l1_index][16];
    cache L2[index][16];
    memset(L1, 0, sizeof(L1));
    memset(L2, 0, sizeof(L2));

    int block_bit = get_block_bit(block);
    int l1_index_bit = get_l1_index_bit(l1_index);
    int l1_tagbit_bit = get_l1_tagbit_bit(block_bit, l1_index_bit);
    int l2_index_bit = get_l2_index_bit(index);
    int l2_tagbit_bit = get_l2_tagbit_bit(block_bit, l2_index_bit);

    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    char* file_buf = (char*)malloc(file_size + 1);
    memset(file_buf, 0, file_size + 1);
    fseek(f, 0, SEEK_SET);
    fread(file_buf, file_size + 1, 1, f);
    fclose(f);

    char* sep = " \n";
    char* cp = strtok(file_buf, sep);

    while (cp) {
        if (strstr(cp, "W") != NULL) {
            total_W++;
            cp = strtok(NULL, sep);
            unsigned long long int address;
            if (strstr(cp, "x") != NULL)
                address = strtoll(cp, NULL, 16);
            else
                address = strtoll(cp, NULL, 10);

            unsigned long long int l1_tagbit = (address >> (block_bit + l1_index_bit));
            unsigned int l1_index_mask = (1 << l1_index_bit) - 1;
            int l1_index = (address >> block_bit) & l1_index_mask;

            unsigned long long int l2_tagbit = (address >> (block_bit + l2_index_bit));
            unsigned int l2_index_mask = (1 << l2_index_bit) - 1;
            int l2_index = (address >> block_bit) & l2_index_mask;

            process_write(L1, L2, l1_index, l1_tagbit, l2_index, l2_tagbit, l1_assoc, assoc, block_bit, l1_tagbit_bit, l1_index_bit, l2_index_bit, replace);
        }
        else if (strstr(cp, "R") != NULL) {
            total_R++;
            cp = strtok(NULL, sep);
            unsigned long long int address;
            if (strstr(cp, "x") != NULL)
                address = strtoll(cp, NULL, 16);
            else
                address = strtoll(cp, NULL, 10);

            unsigned long long int l1_tagbit = (address >> (block_bit + l1_index_bit));
            unsigned int l1_index_mask = (1 << l1_index_bit) - 1;
            int l1_index = (address >> block_bit) & l1_index_mask;

            unsigned long long int l2_tagbit = (address >> (block_bit + l2_index_bit));
            unsigned int l2_index_mask = (1 << l2_index_bit) - 1;
            int l2_index = (address >> block_bit) & l2_index_mask;

            process_read(L1, L2, l1_index, l1_tagbit, l2_index, l2_tagbit, l1_assoc, assoc, block_bit, l1_tagbit_bit, l1_index_bit, l2_index_bit, replace);
        }

        cp = strtok(NULL, sep);
    }
    write_output_file(argv, argc, cap, assoc, block, l1_cap, l1_assoc, total_R, total_W, l1_rd_miss, l2_rd_miss, l1_wr_miss, l2_wr_miss, l2_rd_access, l2_wr_access, l1_clean_eviction, l2_clean_evict, l1_dirty_eviction, l2_dirty_evict);
    free(file_buf);
    return 0;
}
