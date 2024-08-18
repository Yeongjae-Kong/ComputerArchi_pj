#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef struct {
    int address;
    char name[16];
} Label;

Label data_labels[1000];
int data_label_count = 0;
int data_memory[1000] = { 0 };

Label text_labels[1000];
int text_label_count = 0;
int text_memory[1000] = { 0 };
int instruction_count = 0;

char* sep = " ,\t\n$()";

void remove_colon(char* str) {
    char* ptr = strchr(str, ':');
    if (ptr != NULL) {
        *ptr = '\0';
    }
}

void parse_data_section(char* buffer) {
    char* label = strtok(buffer, sep);
    int data_index = 0;

    while (label != NULL) {
        if (strcmp(label, ".word") == 0) {
            char* data_value_str = strtok(NULL, sep);
            long int data_value = strtol(data_value_str, NULL, 0);
            data_memory[data_index] = (int)data_value;
            data_index++;
        }
        else if (strcmp(label, ".data") == 0) {
            break;
        }
        else if (strchr(label, ':') != NULL) {
            label[strlen(label) - 1] = '\0'; // 콜론 제거
            strcpy(data_labels[data_label_count].name, label);
            data_labels[data_label_count].address = 0x10000000 + data_index * 4;
            printf("label: %s, address: 0x%x\n", data_labels[data_label_count].name, data_labels[data_label_count].address);
            data_label_count++;
        }
        label = strtok(NULL, sep);
    }
    data_label_count = data_index;
    printf("data_labels 요소 확인:\n");
    for (int i = 0; i < data_label_count; ++i) {
        printf("data_labels: %s, Address: 0x%x\n", data_labels[i].name, data_labels[i].address);
    }
}

void parse_text_section(char* buffer) {
    char* label = strtok(buffer, sep);
    int* branch_indexes = malloc(1000 * sizeof(int));
    char branch_labels[1000][16];
    int branch_count = 0;

    while (label != NULL) {
        if (strchr(label, ':') != NULL) {
            strcpy(text_labels[text_label_count].name, label);
            text_labels[text_label_count].name[strlen(label) - 1] = '\0';
            remove_colon(text_labels[text_label_count].name);
            text_labels[text_label_count].address = 0x00400000 + instruction_count * 4;
            text_label_count++;
            label = strtok(NULL, sep);
            continue;
        }

        for (int i = 0; i < text_label_count; ++i) {
            printf("Label: %s, Address: 0x%x, length: %ld\n", text_labels[i].name, text_labels[i].address, strlen(text_labels[i].name));
        }
        // r
        if (strcmp(label, "addu") == 0 || strcmp(label, "and") == 0 || strcmp(label, "jr") == 0 ||
            strcmp(label, "nor") == 0 || strcmp(label, "or") == 0 || strcmp(label, "sltu") == 0 ||
            strcmp(label, "sll") == 0 || strcmp(label, "srl") == 0 || strcmp(label, "subu") == 0) {
            int funct = 0;
            if (strcmp(label, "addu") == 0) funct = 0x21;
            else if (strcmp(label, "and") == 0) funct = 0x24;
            else if (strcmp(label, "jr") == 0) funct = 0x08;
            else if (strcmp(label, "nor") == 0) funct = 0x27;
            else if (strcmp(label, "or") == 0) funct = 0x25;
            else if (strcmp(label, "sltu") == 0) funct = 0x2B;
            else if (strcmp(label, "sll") == 0) funct = 0x00;
            else if (strcmp(label, "srl") == 0) funct = 0x02;
            else if (strcmp(label, "subu") == 0) funct = 0x23;

            text_memory[instruction_count] += funct;

            int shift1, shift2, shift3;

            if (strcmp(label, "sll") == 0 || strcmp(label, "srl") == 0) {
                shift1 = 11;
                shift2 = 16;
                shift3 = 6;
            }
            else if (strcmp(label, "jr") == 0) {
                shift1 = 21;
                shift2 = shift3 = 0;
            }
            else {
                shift1 = 11;
                shift2 = 21;
                shift3 = 16;
            }
            text_memory[instruction_count] += atoi(strtok(NULL, sep)) << shift1;
            text_memory[instruction_count] += atoi(strtok(NULL, sep)) << shift2;
            if (shift3 != 0) {
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << shift3;
            }
            instruction_count++;
        }
        // i
        else if (strcmp(label, "addiu") == 0 || strcmp(label, "andi") == 0 || strcmp(label, "ori") == 0 ||
            strcmp(label, "sltiu") == 0 || strcmp(label, "sw") == 0 || strcmp(label, "sb") == 0 ||
            strcmp(label, "lb") == 0 || strcmp(label, "lw") == 0 || strcmp(label, "beq") == 0 ||
            strcmp(label, "bne") == 0 || strcmp(label, "lui") == 0) {
            int opcode = 0;
            if (strcmp(label, "addiu") == 0) opcode = 0x09;
            else if (strcmp(label, "andi") == 0) opcode = 0x0C;
            else if (strcmp(label, "ori") == 0) opcode = 0x0D;
            else if (strcmp(label, "sltiu") == 0) opcode = 0x0B;
            else if (strcmp(label, "sw") == 0) opcode = 0x2B;
            else if (strcmp(label, "sb") == 0) opcode = 0x28;
            else if (strcmp(label, "lb") == 0) opcode = 0x20;
            else if (strcmp(label, "lw") == 0) opcode = 0x23;
            else if (strcmp(label, "beq") == 0) opcode = 0x04;
            else if (strcmp(label, "bne") == 0) opcode = 0x05;
            else if (strcmp(label, "lui") == 0) opcode = 0x0F;

            text_memory[instruction_count] += opcode << 26;

            if (strcmp(label, "addiu") == 0 || strcmp(label, "andi") == 0 || strcmp(label, "ori") == 0 ||
                strcmp(label, "sltiu") == 0) {
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 16;
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 21;
                text_memory[instruction_count] += (strtol(strtok(NULL, sep), NULL, 0) & 0xFFFF);
            }
            else if (strcmp(label, "sw") == 0 || strcmp(label, "sb") == 0 || strcmp(label, "lb") == 0 ||
                strcmp(label, "lw") == 0) {
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 16;
                text_memory[instruction_count] += (strtol(strtok(NULL, sep), NULL, 0) & 0xFFFF);
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 21;
            }
            else if (strcmp(label, "beq") == 0 || strcmp(label, "bne") == 0) {
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 21;
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 16;
                strcpy(branch_labels[branch_count], strtok(NULL, sep));
                branch_indexes[branch_count++] = instruction_count;
            }
            else if (strcmp(label, "lui") == 0) {
                text_memory[instruction_count] += atoi(strtok(NULL, sep)) << 16;
                text_memory[instruction_count] += (strtol(strtok(NULL, sep), NULL, 0) & 0xFFFF);
            }
            instruction_count++;
        }
        // la
        else if (strcmp(label, "la") == 0) {
            int rt = atoi(strtok(NULL, sep));
            const char* label_name = strtok(NULL, sep);

            int string_equal(const char* str1, const char* str2) {
                int len1 = strlen(str1);
                int len2 = strlen(str2);

                if (len1 != len2 - 1) { // len2 : include \0
                    return 0;
                }

                for (int i = 0; i < len1; ++i) {
                    if (str1[i] != str2[i]) {
                        return 0;
                    }
                }

                return 1;
            }

            int address = 0;
            // find address
            for (int i = 0; i < data_label_count; ++i) {
                printf("data_label.name %s\n", data_labels[i].name);
                printf("label_name %s\n", label_name);
                if (string_equal(data_labels[i].name, label_name) == 1) {
                    address = data_labels[i].address;
                    printf("address: 0x%x\n", address);  // 주소 값 출력
                    break;
                }
            }

            // Process J-format instructions
            int upper_bits = (address >> 16) & 0xFFFF;
            text_memory[instruction_count] += (0x0F << 26) | (rt << 16) | upper_bits;
            printf("upper_bits: 0x%x\n", upper_bits);  // upper_bits 값 출력
            printf("instruction: 0x%x\n", text_memory[instruction_count]);
            printf("lui: instruction_count=%d, instruction=0x%x\n", instruction_count, text_memory[instruction_count]);
            instruction_count++;

            int lowbit = address & 0xFFFF;
            if (lowbit) {
                text_memory[instruction_count] += (0x0D << 26) | (rt << 21) | (rt << 16) | lowbit;
                printf("ori: instruction_count=%d, instruction=0x%x\n", instruction_count, text_memory[instruction_count]);
                instruction_count++;
            }
        }
        // j
        else if (strcmp(label, "j") == 0 || strcmp(label, "jal") == 0) {
            int opcode = (strcmp(label, "j") == 0) ? 2 : 3;
            text_memory[instruction_count] += opcode << 26;
            strcpy(branch_labels[branch_count], strtok(NULL, sep));
            branch_indexes[branch_count++] = instruction_count;
            instruction_count++;
        }
        label = strtok(NULL, sep);
        printf("Instruction: %s, instruction_count: %d\n", label, instruction_count);
    }
    // branch
    int temp = 0;
    while (temp < branch_count) {
        const char* branch_label = branch_labels[temp];
        int address = 0;

        size_t len = strlen(branch_label);
        char modified_branch_label[len];
        strncpy(modified_branch_label, branch_label, len - 1);
        modified_branch_label[len - 1] = '\0';

        printf("branch_label: %s, length: %ld\n", modified_branch_label, strlen(modified_branch_label));
        // find address
        int found = 0;
        for (int i = 0; i < text_label_count; ++i) {
            printf("text_labels[%d].name: %s\n", i, text_labels[i].name);
            if (strcmp(text_labels[i].name, modified_branch_label) == 0) {
                address = text_labels[i].address;
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Error: Label '%s' not found\n", branch_label);
            temp++;
            continue;
        }

        int index = branch_indexes[temp];
        int current_address = 0x00400000 + index * 4;
        printf("Branch instruction: 0x%x\n", text_memory[index]);
        printf("Branch address: 0x%x\n", address);
        printf("Current address: 0x%x\n", current_address);

        if ((text_memory[index] >> 26) == 2) { // j 또는 jal 명령어인 경우
            int jump_address = address / 4;
            printf("Jump address (before): 0x%x\n", text_memory[index]);
            text_memory[index] = (text_memory[index] & 0xFC000000) | (jump_address & 0x03FFFFFF);
            printf("Jump address (after): 0x%x\n", text_memory[index]);
            temp++;
            continue;
        }

        else if ((text_memory[index] >> 26) == 3) { // jal 명령어인 경우
            int jump_address = address / 4;
            printf("Jump address (before): 0x%x\n", text_memory[index]);

            // 디버깅용 출력 코드 추가
            int opcode = 3 << 26;
            int upper_bits = text_memory[index] & 0xFC000000;
            printf("Opcode: 0x%x\n", opcode);
            printf("Upper bits (before): 0x%x\n", upper_bits);

            text_memory[index] = (text_memory[index] & 0xFC000000) | opcode | (jump_address & 0x03FFFFFF);

            // 디버깅용 출력 코드 추가
            upper_bits = text_memory[index] & 0xFC000000;
            printf("Upper bits (after): 0x%x\n", upper_bits);
            printf("Jump address (after): 0x%x\n", text_memory[index]);
        }

        else { // bne
            int offset = (address - (current_address + 4)) / 4;
            printf("Offset (before): 0x%x\n", text_memory[index]);
            printf("Address: 0x%x\n", address);
            printf("Next instruction address: 0x%x\n", current_address + 4); // 수정된 부분
            printf("Offset: %d\n", offset);
            if (offset < 0) {
                offset = (1 << 16) + offset;
                printf("Negative offset: 0x%x\n", offset);
            }
            text_memory[index] = (text_memory[index] & 0xFFFF0000) | (offset & 0xFFFF);
            printf("Offset (after): 0x%x\n", text_memory[index]);
        }
        temp++;
    }

    free(branch_indexes);

}

int main(int argc, char* argv[]) {
    FILE* input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        printf("Failed to open input file.\n");
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) != 0) {
        printf("Failed to get file size.\n");
        fclose(input_file);
        return 1;
    }
    long size = st.st_size;

    char* buffer = (char*)malloc(size);
    if (buffer == NULL) {
        printf("Memory allocation failed.\n");
        fclose(input_file);
        return 1;
    }

    fread(buffer, 1, size, input_file);
    fclose(input_file);

    char* text_section = strstr(buffer, ".text");
    if (text_section != NULL) {
        *text_section = '\0';
        parse_data_section(buffer);
        parse_text_section(text_section + 6);
    }
    else {
        parse_data_section(buffer);
    }

    free(buffer);

    char output_filename[strlen(argv[1]) + 3]; // For ".o\0"
    strcpy(output_filename, argv[1]);
    char* extension = strrchr(output_filename, '.');
    if (extension != NULL) {
        strcpy(extension, ".o");
    }

    FILE* output_file = fopen(output_filename, "wb"); // Binary mode
    if (output_file == NULL) {
        printf("Failed to create output file.\n");
        return 1;
    }

    fprintf(output_file, "0x%x\n", instruction_count * 4);
    fprintf(output_file, "0x%x\n", data_label_count * 4);

    for (int i = 0; i < instruction_count; ++i) {
        fprintf(output_file, "0x%x\n", text_memory[i]);
    }
    for (int i = 0; i < data_label_count; ++i) {
        fprintf(output_file, "0x%x\n", data_memory[i]);
    }

    fclose(output_file);

    return 0;
}
