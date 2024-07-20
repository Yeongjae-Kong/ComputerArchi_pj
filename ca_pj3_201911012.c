#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	unsigned int noop, mux, NPC, branch, jump;
} IF;
typedef struct {
	unsigned int noop, NPC, Instr;
} IF_ID;
typedef struct {
	unsigned int noop, NPC, rs, rt, rd, IMM, funct, opcode, read_m, write_m, write_r, m2r, shampt, rs2, rt2;
} ID_EX;
typedef struct {
	unsigned int noop, alu_out, reg_num, read_m, write_m, write_r, m2r, offset, branch, NPC, branch_pc;
} EX_MEM;
typedef struct {
	unsigned int noop, reg_num, write_r, m2r, atp, antp, NPC, brach_pc, result;
} MEM_WB;

unsigned int start_address;
unsigned int end_address;
unsigned int data_start = 0x10000000;
unsigned int text_start = 0x400000;

void forwardingEXMEM(ID_EX* id_ex, EX_MEM* ex_mem) {
	if (ex_mem->write_r == 1) {
		if (ex_mem->reg_num == id_ex->rs2) {
			id_ex->rs = ex_mem->alu_out;
		}
		if (ex_mem->reg_num == id_ex->rt2) {
			id_ex->rt = ex_mem->alu_out;
		}
	}
}

void forwardingMEMWB(ID_EX* id_ex, EX_MEM* ex_mem, MEM_WB* mem_wb) {
	if (mem_wb->write_r == 1) {
		if ((ex_mem->reg_num != id_ex->rs2) && (mem_wb->reg_num == id_ex->rs2)) {
			id_ex->rs = mem_wb->result;
		}
		if ((ex_mem->reg_num != id_ex->rt2) && (mem_wb->reg_num == id_ex->rt2)) {
			id_ex->rt = mem_wb->result;
		}
	}
}

void stallPipeline(IF* IF, IF_ID* IF_ID) {
	IF->mux = 0;
	IF->NPC -= 1;
	IF_ID->noop = 0;
	IF_ID->NPC = 0;
	IF_ID->Instr = 0;
}

void loadUseDetect(IF* IF, IF_ID* IF_ID, ID_EX* ID_EX) {
	if (ID_EX->read_m == 1 && (ID_EX->opcode == 0x23 || ID_EX->opcode == 0x20)) {
		unsigned int Instr = IF_ID->Instr;
		int rs = (Instr << 6) >> 27;
		int rt = (Instr << 11) >> 27;

		if (((Instr << 26) >> 26 == 8 && ID_EX->rt == rs) ||
			(Instr >> 26 == 0 && (ID_EX->rt == rs || ID_EX->rt == rt)) ||
			((Instr >> 26 == 4 || Instr >> 26 == 5) && (ID_EX->rt == rs || ID_EX->rt == rt)) ||
			((Instr >> 26 == 0x2b || Instr >> 26 == 0x28) && (ID_EX->rt == rs || ID_EX->rt == rt)) ||
			(ID_EX->rt == rs)) {
			stallPipeline(IF, IF_ID);
		}
	}
}

int atp_antp = 0;
int m = 0;
int d = 0;
int p = 0;
int n = 0;
FILE* input_file;
int n_count;

void parse_options(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
			m = 1;
			char* addr = argv[++i];
			sscanf(addr, "%x:%x", &start_address, &end_address);
		}
		else if (strcmp(argv[i], "-d") == 0) {
			d = 1;
		}
		else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
			n = 1;
			n_count = (int)strtol(argv[++i], NULL, 0);
		}
		else if (strcmp(argv[i], "-p") == 0) {
			p = 1;
		}
		else if (strcmp(argv[i], "-atp") == 0) {
			atp_antp = 1;
		}
		else if (strcmp(argv[i], "-antp") == 0) {
			atp_antp = 2;
		}
		else {
			input_file = fopen(argv[i], "r");
		}
	}
}

int* data;
int* text;
int data_size;
int text_size;

void read_file() {
	int size;
	fseek(input_file, 0, SEEK_END);
	size = ftell(input_file);
	char* f = malloc(size + 1);
	memset(f, 0, size + 1);
	fseek(input_file, 0, SEEK_SET);
	fread(f, size, 1, input_file);
	fclose(input_file);

	char* line = strtok(f, "\n");
	text_size = (int)strtol(line, NULL, 0);
	line = strtok(NULL, "\n");
	data_size = (int)strtol(line, NULL, 0);

	text = (int*)malloc(sizeof(int) * (text_size / 4));
	data = malloc(sizeof(int) * (data_size / 4));

	memset(text, 0, sizeof(int) * (text_size / 4));
	memset(data, 0, sizeof(int) * (data_size / 4));

	for (int i = 0; i < text_size / 4; i++) {
		line = strtok(NULL, "\n");
		text[i] = (int)strtol(line, NULL, 0);
	}
	for (int j = 0; j < data_size / 4; j++) {
		line = strtok(NULL, "\n");
		data[j] = (int)strtol(line, NULL, 0);
	}

	free(f);
}

void print_register_values(unsigned int address, unsigned int* registers) {
	printf("Current register values:\n");
	printf("PC: 0x%x\n", text_start + address * 4);
	printf("Registers:\n");
	for (int i = 0; i < 32; i++) {
		printf("R%d: 0x%x\n", i, registers[i]);
	}
	printf("\n");
}

void print_memory_content(unsigned int start, unsigned int end, int* data_addr, int* text_addr, int data_size, int text_size) {
	printf("Memory content [0x%x..0x%x]:\n", start, end);
	printf("---------------------------------------\n");
	for (int i = start; i <= end; i += 4) {
		int found = 0;
		for (int j = 0; j < data_size / 4; j++) {
			if (data_addr[j] == i) {
				printf("0x%x: 0x%x\n", i, data[j]);
				found = 1;
			}
		}
		for (int j = 0; j < text_size / 4; j++) {
			if (text_addr[j] == i) {
				printf("0x%x: 0x%x\n", i, text[j]);
				found = 1;
			}
		}
		if (found == 0) {
			printf("0x%x: 0x0\n", i);
		}
	}
	printf("\n");
}

void init_pipeline(IF_ID* IF_ID, ID_EX* ID_EX, EX_MEM* EX_MEM, MEM_WB* MEM_WB) {
	IF_ID->noop = 0;
	IF_ID->NPC = 0;
	IF_ID->Instr = 0;
	ID_EX->noop = 0;
	ID_EX->NPC = 0;
	ID_EX->rs = 0;
	ID_EX->rt = 0;
	ID_EX->rd = 0;
	ID_EX->IMM = 0;
	ID_EX->funct = 0;
	ID_EX->opcode = 0;
	ID_EX->read_m = 0;
	ID_EX->write_m = 0;
	ID_EX->write_r = 0;
	ID_EX->m2r = 0;
	ID_EX->shampt = 0;
	ID_EX->rs2 = 0;
	ID_EX->rt2 = 0;
	EX_MEM->noop = 0;
	EX_MEM->alu_out = 0;
	EX_MEM->reg_num = 0;
	EX_MEM->read_m = 0;
	EX_MEM->write_m = 0;
	EX_MEM->write_r = 0;
	EX_MEM->m2r = 0;
	EX_MEM->offset = 0;
	EX_MEM->branch = 0;
	EX_MEM->NPC = 0;
	EX_MEM->branch_pc = 0;
}

int num_i = 0;
int idx = 0;
int cycle = 0;

int main(int argc, char* argv[]) {

	parse_options(argc, argv);
	read_file();

	int* data_addr = (int*)malloc(sizeof(int) * (data_size) / 4);
	memset(data_addr, 0, sizeof(int) * (data_size) / 4);

	for (int i = 0; i < (data_size / 4); i++) {
		data_addr[i] = data_start + i * 4;
	}

	int* text_addr = (int*)malloc(sizeof(int) * (text_size) / 4);
	memset(text_addr, 0, sizeof(int) * (text_size) / 4);

	for (int j = 0; j < (text_size / 4); j++) {
		text_addr[j] = text_start + j * 4;
	}

	IF IF;
	IF.noop = 1;

	IF_ID IF_ID;
	ID_EX ID_EX;
	EX_MEM EX_MEM;
	MEM_WB MEM_WB;

	unsigned int registers[32] = { 0 };

	while (IF.noop || IF_ID.noop || ID_EX.noop || EX_MEM.noop || MEM_WB.noop) {

		unsigned int PC_arr[5] = { 0 };
		if ((n && num_i == n_count) || (d && p && (IF.noop + IF_ID.noop + ID_EX.noop + EX_MEM.noop + MEM_WB.noop == 0))) {
			break;
		}

		if (ID_EX.opcode == 0 || ID_EX.opcode == 4 || ID_EX.opcode == 5 || ID_EX.opcode == 0x2b || ID_EX.opcode == 0x28 || ID_EX.funct == 8) {
			forwardingEXMEM(&ID_EX, &EX_MEM);
		}
		else {
			forwardingEXMEM(&ID_EX, &EX_MEM);
		}
		//datahazard
		if (ID_EX.opcode == 0 || ID_EX.opcode == 4 || ID_EX.opcode == 5 || ID_EX.opcode == 0x2b || ID_EX.opcode == 0x28 || ID_EX.funct == 8) {
			forwardingMEMWB(&ID_EX, &EX_MEM, &MEM_WB);
		}
		else {
			forwardingMEMWB(&ID_EX, &EX_MEM, &MEM_WB);
		}
		loadUseDetect(&IF, &IF_ID, &ID_EX);
		//WB
		if (MEM_WB.noop != 0) {
			PC_arr[4] = (MEM_WB.NPC - 1) * 4 + text_start;
			num_i++;

			if (MEM_WB.atp == 1) {
				IF.noop = 1;
				IF.mux = 1;
				IF.branch = MEM_WB.NPC;
				init_pipeline(&IF_ID, &ID_EX, &EX_MEM, &MEM_WB);
			}

			else if (MEM_WB.antp == 1) {
				IF.noop = 1;
				IF.mux = 1;
				IF.branch = MEM_WB.brach_pc;
				init_pipeline(&IF_ID, &ID_EX, &EX_MEM, &MEM_WB);
			}

			else if (MEM_WB.write_r == 1) {
				int reg_num = MEM_WB.reg_num;
				int result = MEM_WB.result;
				registers[reg_num] = result;
			}
			MEM_WB.noop = 0;
			MEM_WB.reg_num = 0;
			MEM_WB.write_r = 0;
			MEM_WB.m2r = 0;
			MEM_WB.atp = 0;
			MEM_WB.antp = 0;
			MEM_WB.NPC = 0;
			MEM_WB.brach_pc = 0;
			MEM_WB.result = 0;
		}
		// mem
		if (EX_MEM.noop != 0) {
			PC_arr[3] = (EX_MEM.NPC - 1) * 4 + text_start;
			MEM_WB.noop = 1;
			MEM_WB.NPC = EX_MEM.NPC;
			MEM_WB.write_r = EX_MEM.write_r;
			MEM_WB.m2r = EX_MEM.m2r;

			if (EX_MEM.branch == 1) {
				if (atp_antp == 1 && EX_MEM.alu_out == 0) {
					MEM_WB.atp = 1;
					MEM_WB.NPC = EX_MEM.NPC;
				}
				else if (atp_antp == 2 && EX_MEM.alu_out == 1) {
					MEM_WB.antp = 1;
					MEM_WB.brach_pc = EX_MEM.branch_pc;
				}
			}
			else {
				if (EX_MEM.read_m == 1) {
					MEM_WB.write_r = 1;
					MEM_WB.m2r = 1;
					MEM_WB.reg_num = EX_MEM.reg_num;
					if (EX_MEM.offset != 0) {
						MEM_WB.result = (data[EX_MEM.alu_out] << (8 * EX_MEM.offset)) >> 24;
					}
					else {
						MEM_WB.result = data[EX_MEM.alu_out];
					}
				}
				else if (EX_MEM.write_m == 1) {
					MEM_WB.write_r = 0;
					MEM_WB.m2r = 0;
					int mask = 0xFF << (8 * (3 - EX_MEM.offset));
					data[EX_MEM.alu_out] = (data[EX_MEM.alu_out] & ~mask) | ((EX_MEM.reg_num << (8 * (3 - EX_MEM.offset))) & mask);
				}
				else if (EX_MEM.write_r == 1) {
					MEM_WB.result = EX_MEM.alu_out;
					MEM_WB.reg_num = EX_MEM.reg_num;
					MEM_WB.write_r = 1;
					MEM_WB.m2r = 0;
				}
			}
			EX_MEM.noop = 0;
			EX_MEM.alu_out = 0;
			EX_MEM.reg_num = 0;
			EX_MEM.read_m = 0;
			EX_MEM.write_m = 0;
			EX_MEM.write_r = 0;
			EX_MEM.m2r = 0;
			EX_MEM.offset = 0;
			EX_MEM.branch = 0;
			EX_MEM.NPC = 0;
			EX_MEM.branch_pc = 0;
		}
		// ex
		if (ID_EX.noop != 0) {
			PC_arr[2] = (ID_EX.NPC - 1) * 4 + text_start;
			EX_MEM.noop = 1;
			EX_MEM.NPC = ID_EX.NPC;
			EX_MEM.read_m = ID_EX.read_m;
			EX_MEM.write_m = ID_EX.write_m;
			EX_MEM.write_r = ID_EX.write_r;
			EX_MEM.m2r = ID_EX.m2r;

			int rs = ID_EX.rs;
			int rt = ID_EX.rt;
			int IMM = ID_EX.IMM;

			if (ID_EX.opcode == 0) {
				EX_MEM.reg_num = ID_EX.rd;

				switch (ID_EX.funct) {
				case 0x21: EX_MEM.alu_out = rs + rt; break;
				case 0x24: EX_MEM.alu_out = rs & rt; break;
				case 0x25: EX_MEM.alu_out = rs | rt; break;
				case 0x27: EX_MEM.alu_out = ~(rs | rt); break;
				case 0x00: EX_MEM.alu_out = rt << ID_EX.shampt; break;
				case 0x02: EX_MEM.alu_out = rt >> ID_EX.shampt; break;
				case 0x23: EX_MEM.alu_out = rs - rt; break;
				case 0x2b: EX_MEM.alu_out = (rs < rt) ? 1 : 0; break;
				case 0x08:
					EX_MEM.reg_num = 0;
					IF.noop = 1;
					IF.mux = 2;
					IF.jump = (rs - text_start) / 4;
					IF_ID.noop = 0;
					IF_ID.NPC = 0;
					IF_ID.Instr = 0;
					break;
				}
			}
			else if (ID_EX.opcode == 2 || ID_EX.opcode == 3) {
				int target = ID_EX.IMM;
				int idx_buf = ID_EX.NPC;
				int pc = (((idx_buf * 4) + text_start) >> 28) << 28;
				idx_buf = ((target + pc) - text_start) / 4;

				IF.noop = 1;
				IF.mux = 2;
				IF.jump = idx_buf;
				IF_ID.noop = 0;
				IF_ID.NPC = 0;
				IF_ID.Instr = 0;

				if (ID_EX.opcode == 2) {
				}
				else {
					EX_MEM.alu_out = (ID_EX.NPC * 4 + text_start);
					EX_MEM.reg_num = rt;
				}
			}
			else if (ID_EX.opcode == 4 || ID_EX.opcode == 5) {
				EX_MEM.branch = 1;
				EX_MEM.NPC = ID_EX.NPC;

				if (atp_antp == 1) {
					IF.mux = 1;
					IF.branch = IMM + ID_EX.NPC;
					IF_ID.noop = 0;
					IF_ID.NPC = 0;
					IF_ID.Instr = 0;
				}
				else if (atp_antp == 2) {
					EX_MEM.branch_pc = IMM + ID_EX.NPC;
				}

				EX_MEM.alu_out = (ID_EX.opcode == 4) ? (rs == rt) : (rs != rt);
			}
			else {
				EX_MEM.reg_num = ID_EX.rt;

				switch (ID_EX.opcode) {
				case 0x09: EX_MEM.alu_out = rs + IMM; break;
				case 0x0c: EX_MEM.alu_out = rs & IMM; break;
				case 0x0d: EX_MEM.alu_out = rs | IMM; break;
				case 0x0f: EX_MEM.alu_out = IMM << 16; break;
				case 0x0b: EX_MEM.alu_out = (rs < IMM) ? 1 : 0; break;
				case 0x23: EX_MEM.alu_out = (rs + IMM - data_start) / 4; break;
				case 0x20:
					EX_MEM.alu_out = (rs + IMM - data_start) / 4;
					EX_MEM.offset = (rs + IMM - data_start) % 4;
					break;
				case 0x2b: EX_MEM.alu_out = (rs + IMM - data_start) / 4; break;
				case 0x28:
					EX_MEM.alu_out = (rs + IMM - data_start) / 4;
					EX_MEM.offset = (rs + IMM - data_start) % 4;
					break;
				}
			}
			ID_EX.noop = 0;
			ID_EX.NPC = 0;
			ID_EX.rs = 0;
			ID_EX.rt = 0;
			ID_EX.rd = 0;
			ID_EX.IMM = 0;
			ID_EX.funct = 0;
			ID_EX.opcode = 0;
			ID_EX.read_m = 0;
			ID_EX.write_m = 0;
			ID_EX.write_r = 0;
			ID_EX.m2r = 0;
			ID_EX.shampt = 0;
			ID_EX.rs2 = 0;
			ID_EX.rt2 = 0;
		}

		// id
		if (IF_ID.noop != 0) {
			PC_arr[1] = (IF_ID.NPC - 1) * 4 + text_start;
			ID_EX.noop = 1;
			ID_EX.NPC = IF_ID.NPC;
			unsigned int Instr = IF_ID.Instr;

			int rs, rt, rd, funct, shampt, target, opcode, IMM;

			switch (Instr >> 26) {
			case 0x00:
				rs = (Instr >> 21) & 0x1F;
				rt = (Instr >> 16) & 0x1F;
				rd = (Instr >> 11) & 0x1F;
				funct = Instr & 0x3F;

				ID_EX.rs = registers[rs];
				ID_EX.rt = registers[rt];
				ID_EX.rd = rd;
				ID_EX.funct = funct;
				ID_EX.opcode = 0;
				ID_EX.write_r = 1;
				ID_EX.rs2 = rs;
				ID_EX.rt2 = rt;

				if (funct == 8) {
					ID_EX.write_r = 0;
				}
				else if ((funct == 0) || (funct == 2)) {
					shampt = (Instr << 21) >> 27;
					ID_EX.shampt = shampt;
				}
				break;

			case 0x02:
				target = ((Instr << 6) >> 4);
				ID_EX.IMM = target;
				ID_EX.opcode = 2;
				break;

			case 0x03:
				target = ((Instr << 6) >> 4);
				ID_EX.rt = 31;
				ID_EX.IMM = target;
				ID_EX.opcode = 3;
				ID_EX.write_r = 1;
				break;

			default:
				opcode = Instr >> 26;
				rs = (Instr >> 21) & 0x1F;
				rt = (Instr >> 16) & 0x1F;

				ID_EX.rs = registers[rs];
				ID_EX.rt = rt;
				ID_EX.opcode = opcode;
				ID_EX.rs2 = rs;

				if (opcode == 0x20 || opcode == 0x23) {
					ID_EX.read_m = 1;
					ID_EX.write_r = 1;
					ID_EX.m2r = 1;
				}
				else if (opcode == 0x2b || opcode == 0x28) {
					ID_EX.write_m = 1;
				}
				else {
					ID_EX.write_r = 1;
				}

				if (opcode == 0xc || opcode == 0xd) {
					IMM = (Instr << 16) >> 16;
					ID_EX.IMM = IMM;
				}
				else if (opcode == 4 || opcode == 5) {
					ID_EX.write_r = 0;
					ID_EX.rt = registers[rt];
					IMM = Instr << 16;
					ID_EX.IMM = IMM >> 16;
					ID_EX.rt2 = rt;
				}
				else if (opcode == 0x2b || opcode == 0x28) {
					ID_EX.rt = registers[rt];
					IMM = Instr << 16;
					ID_EX.IMM = IMM >> 16;
					ID_EX.rt2 = rt;
				}
				else {
					IMM = Instr << 16;
					ID_EX.IMM = IMM >> 16;
				}
				break;
			}
			IF_ID.noop = 0;
			IF_ID.NPC = 0;
			IF_ID.Instr = 0;
		}

		// if
		if (IF.noop != 0) {
			unsigned int Instr;
			switch (IF.mux) {
			case 0:
				idx = IF.NPC;
				Instr = text[idx];
				break;
			case 1:
				idx = IF.branch;
				Instr = text[idx];
				IF.branch = 0;
				break;
			case 2:
				idx = IF.jump;
				Instr = text[idx];
				IF.jump = 0;
				break;
			}

			IF_ID.noop = 1;
			IF_ID.NPC = ++idx;
			IF_ID.Instr = Instr;
			PC_arr[0] = (idx - 1) * 4 + text_start;
			IF.NPC = idx;
			IF.mux = 0;

			if (idx > text_size / 4) {
				PC_arr[0] = 0;
				IF.noop = 0;
				IF_ID.noop = 0;
				IF_ID.NPC = 0;
				IF_ID.Instr = 0;
				idx -= 1;
			}
		}
		cycle++;
		int address = idx;
		for (int i = 0; i < 5; i++) {
			if (PC_arr[i] > text_start + text_size) {
				PC_arr[i] = 0;
			}
		}
		if (p) {
			printf("===== Cycle %d =====\n", cycle);
			printf("Current pipeline PC state: \n");
			printf("{");
			printf((PC_arr[0] != 0) ? "0x%x|" : "|", PC_arr[0]);
			printf((PC_arr[1] != 0) ? "0x%x|" : "|", PC_arr[1]);
			printf((PC_arr[2] != 0) ? "0x%x|" : "|", PC_arr[2]);
			printf((PC_arr[3] != 0) ? "0x%x|" : "|", PC_arr[3]);
			printf((PC_arr[4] != 0) ? "0x%x}\n\n" : "}\n\n", PC_arr[4]);
		}
		if (d) {
			if (ID_EX.read_m == 1 && (ID_EX.opcode == 0x23 || ID_EX.opcode == 0x20)) {
				unsigned int Instr = IF_ID.Instr;
				int rs = (Instr << 6) >> 27;
				int rt = (Instr << 11) >> 27;
				int opcode = Instr >> 26;

				if ((opcode == 0 || opcode == 4 || opcode == 5 || opcode == 0x2b || opcode == 0x28) && (ID_EX.rt == rs || ID_EX.rt == rt) ||
					((Instr << 26) >> 26 == 8 && ID_EX.rt == rs)) {
					address = IF.NPC - 1;
				}
				else {
					if (ID_EX.rt == rs) {
						address -= IF.NPC - 1;
					}
				}
			}
			if (MEM_WB.noop != 0) {
				address = MEM_WB.atp ? MEM_WB.NPC : (MEM_WB.antp ? MEM_WB.brach_pc : address);
			}
			if (ID_EX.noop != 0) {
				if (ID_EX.opcode == 0 && ID_EX.funct == 8) {
					address = (ID_EX.rs - text_start) / 4;
				}
				else if (ID_EX.opcode == 2 || ID_EX.opcode == 3) {
					int idx_arr = ID_EX.NPC * 4;
					int addr = ID_EX.IMM;
					int pc = ((idx_arr + text_start) >> 28);
					address = (((pc << 28) + addr) - text_start) / 4;
				}
				else if ((ID_EX.opcode == 4 || ID_EX.opcode == 5) && atp_antp == 1) {
					address = ID_EX.IMM + ID_EX.NPC;
				}
			}
			printf("===== Cycle %d =====\n\n", cycle);
			print_register_values(address, registers);
			if (m == 1) {
				print_memory_content(start_address, end_address, data_addr, text_addr, data_size, text_size);
			}
		}
	}
	if (!n) {
		printf("===== Completion cycle: %d =====\n\n", cycle);
		printf("Current pipeline PC state:\n");
		printf("{||||}\n\n");
		print_register_values(idx, registers);
		if (m == 1) {
			print_memory_content(start_address, end_address, data_addr, text_addr, data_size, text_size);
		}
	}
	free(data);
	free(text);

	free(data_addr);
	free(text_addr);

	return 0;
}
