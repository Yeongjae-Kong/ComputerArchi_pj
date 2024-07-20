#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned int opcode;
    unsigned int rs;
    unsigned int rt;
    unsigned int rd;
    unsigned int shamt;
    unsigned int funct;
    unsigned int immediate;
    unsigned int target;
} Instruction;

unsigned int registers[32];
unsigned int program_counter;
unsigned int data_start = 0x10000000;
unsigned int text_start = 0x00400000;
int* data;
int* text;
int data_size;
int text_size;
unsigned int accessed_addresses[1024];
int accessed_count = 0;

void decode_instruction(unsigned int instruction, Instruction* decoded) {
    decoded->opcode = (instruction >> 26) & 0x3F;
    decoded->rs = (instruction >> 21) & 0x1F;
    decoded->rt = (instruction >> 16) & 0x1F;
    decoded->rd = (instruction >> 11) & 0x1F;
    decoded->shamt = (instruction >> 6) & 0x1F;
    decoded->funct = instruction & 0x3F;
    decoded->immediate = instruction & 0xFFFF;
    decoded->target = instruction & 0x3FFFFFF;
}

void print_registers() {
    printf("Current register values:\n");
    printf("--------------------------------------------\n");
    printf("PC = 0x%x\n", program_counter);
    printf("Registers:\n");
    for (int i = 0; i < 32; i++) {
        printf("R%d = 0x%x\n", i, registers[i]);
    }
    printf("\n");
}

void print_memory_content(int start_address, int end_address, unsigned int* accessed_addresses, int accessed_count) {
    printf("Memory content [0x%x..0x%x]:\n", start_address, end_address);
    printf("--------------------------------------------\n");
    for (int address = start_address; address <= end_address; address += 4) {
        int is_accessed = 0;
        for (int i = 0; i < accessed_count; i++) {
            if (address == accessed_addresses[i]) {
                is_accessed = 1;
                break;
            }
        }

        if (is_accessed) {
            if (address >= text_start && address < text_start + text_size) {
                printf("0x%x: 0x%x\n", address, text[(address - text_start) / 4]);
            }
            else if (address >= data_start && address < data_start + data_size) {
                printf("0x%x: 0x%x\n", address, data[(address - data_start) / 4]);
            }
        }
        else {
            printf("0x%x: 0x0\n", address);
        }
    }
    printf("\n");
}

void execute_r_type(Instruction* instruction) {
    switch (instruction->funct) {
        // addu
        case 0x21:
            registers[instruction->rd] = registers[instruction->rs] + registers[instruction->rt];
            break;
        // and
        case 0x24:
            registers[instruction->rd] = registers[instruction->rs] & registers[instruction->rt];
            break;
        // nor
        case 0x27:
            registers[instruction->rd] = ~(registers[instruction->rs] | registers[instruction->rt]);
            break;
        // or
        case 0x25:
            registers[instruction->rd] = registers[instruction->rs] | registers[instruction->rt];
            break;
        // sltu
        case 0x2B:
            registers[instruction->rd] = (registers[instruction->rs] < registers[instruction->rt]) ? 1 : 0;
            break;
        // sll
        case 0x00:
            registers[instruction->rd] = registers[instruction->rt] << instruction->shamt;
            break;
        // srl
        case 0x02:
            registers[instruction->rd] = registers[instruction->rt] >> instruction->shamt;
            break;
        // subu
        case 0x23:
            registers[instruction->rd] = registers[instruction->rs] - registers[instruction->rt];
            break;
        // jr
        case 0x08:
            program_counter = registers[instruction->rs];
            return;
    }
}
void execute_i_type(Instruction* instruction) {
    int signed_immediate = (int)((instruction->immediate & 0x8000) ? 0xFFFF0000 | instruction->immediate : instruction->immediate);
    switch (instruction->opcode) {
        // addiu
        case 0x09:
            registers[instruction->rt] = registers[instruction->rs] + signed_immediate;
            break;
        // andi
        case 0x0C:
            registers[instruction->rt] = registers[instruction->rs] & instruction->immediate;
            break;
        // beq
        case 0x04:
            if (registers[instruction->rs] == registers[instruction->rt]) {
                program_counter += (signed_immediate << 2);
            }
            break;
        // bne
        case 0x05:
            if (registers[instruction->rs] != registers[instruction->rt]) {
                program_counter += (signed_immediate << 2);
            }
            break;
        // lb
        case 0x20:
        {
            unsigned int address = registers[instruction->rs] + signed_immediate - data_start;
            unsigned int byte = data[address / 4] >> ((3 - (address % 4)) * 8) & 0xFF;
            registers[instruction->rt] = (byte & 0x80) ? 0xFFFFFF00 | byte : byte;
            accessed_addresses[accessed_count++] = address;
            break;
        }
        // lui
        case 0x0F:
            registers[instruction->rt] = instruction->immediate << 16;
            break;
        // lw
        case 0x23:
        {
            unsigned int address = registers[instruction->rs] + signed_immediate - data_start;
            registers[instruction->rt] = data[address / 4];
            accessed_addresses[accessed_count++] = address;
            break;
        }
        // ori
        case 0x0D:
            registers[instruction->rt] = registers[instruction->rs] | instruction->immediate;
            break;
        // sb
        case 0x28:
        {
            unsigned int address = registers[instruction->rs] + signed_immediate - data_start;
            unsigned int aligned_address = address & ~3;
            unsigned int byte_offset = address & 3;
            data[aligned_address / 4] &= ~(0xFF << (byte_offset * 8));
            data[aligned_address / 4] |= (registers[instruction->rt] & 0xFF) << (byte_offset * 8);
            accessed_addresses[accessed_count++] = address;
            break;
        }
        // sltiu
        case 0x0B:
            registers[instruction->rt] = (registers[instruction->rs] < (unsigned int)signed_immediate) ? 1 : 0;
            break;
        // sw
        case 0x2B:
        {
            unsigned int address = registers[instruction->rs] + signed_immediate - data_start;
            data[address / 4] = registers[instruction->rt];
            accessed_addresses[accessed_count++] = address;
            break;
        }
    }
}

void execute_j_type(Instruction* instruction) {
    switch (instruction->opcode) {
        // j
        case 0x02:
            program_counter = (program_counter & 0xF0000000) | (instruction->target << 2);
            break;
        // jal
        case 0x03:
            registers[31] = program_counter;
            program_counter = (program_counter & 0xF0000000) | (instruction->target << 2);
            break;
    }
}

void execute_instruction(Instruction* instruction) {
    switch (instruction->opcode) {
        case 0x00:
            execute_r_type(instruction);
            break;
        case 0x02:
        case 0x03:
            execute_j_type(instruction);
            break;
        default:
            execute_i_type(instruction);
            break;
    }
}

int main(int argc, char* argv[]) {
    FILE* input_file = NULL;
    int d = 0;
    int num_instructions = -1;
    int memory_start = 0;
    int memory_end = 0;
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            d = 1;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            num_instructions = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-m") == 0) {
            sscanf(argv[++i], "0x%x:0x%x", &memory_start, &memory_end);
        }
        else {
            input_file = fopen(argv[i], "r");
        }
    }
    // Read input file
    fscanf(input_file, "%x", &text_size);
    fscanf(input_file, "%x", &data_size);
    text = (int*)malloc(text_size);
    data = (int*)malloc(data_size);
    for (int i = 0; i < text_size / 4; i++) {
        fscanf(input_file, "%x", &text[i]);
    }
    for (int i = 0; i < data_size / 4; i++) {
        fscanf(input_file, "%x", &data[i]);
        accessed_addresses[accessed_count++] = data_start + i * 4;
    }
    fclose(input_file);
    // Initialize registers and program counter
    memset(registers, 0, sizeof(registers));
    program_counter = text_start;
    // Execute instructions
    while (num_instructions != 0) {
        if (program_counter >= text_start && program_counter < text_start + text_size) {
            unsigned int instruction_code = text[(program_counter - text_start) / 4];
            Instruction instruction;
            decode_instruction(instruction_code, &instruction);
            accessed_addresses[accessed_count++] = program_counter;
            program_counter += 4;
            execute_instruction(&instruction);
            num_instructions--;
            if (d) {
                print_registers();
                if (memory_start != 0 || memory_end != 0) {
                    print_memory_content(memory_start, memory_end, accessed_addresses, accessed_count);
                    printf("\n");
                }
            }
        }
        else {
            break;
        }
    }

    if (d != 1) {
        print_registers();
        if (memory_start != 0 || memory_end != 0) {
            print_memory_content(memory_start, memory_end, accessed_addresses, accessed_count);
        }
    }
    free(text);
    free(data);

    return 0;
}
