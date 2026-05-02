#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "minimips.h"

FILE *arquivo = NULL;
FILE *arquivoMemDados = NULL;

//---------------------------------------LEITURA E INICIALIZAÇÃO------------------------------------------------

/*void contaLinhas(char *arq, int *qtInst, int *qtDados){

    FILE *arquivo = fopen(arq, "r");
    if (arquivo == NULL) {
        printf("\nErro ao abrir arquivo .mem!\n");
        return;
    }

    char leitura[64];
    int lerDados = 0;

    *qtInst = 0;
    *qtDados = 0;

    while(fgets(leitura, sizeof(leitura), arquivo)){

        leitura[strcspn(leitura, "\n")] = '\0';

        if(strcmp(leitura, ".data") == 0){
            lerDados = 1;
            continue;
        }

        if(lerDados == 0){
            (*qtInst)++;
        }else{
            (*qtDados)++;
        }
    }

    fclose(arquivo);
}*/

// Leitura da memória
int lerMemUnificada(char *arq, MemoriaUnificada *memUnificada) {

    FILE *arquivo = fopen(arq, "r");

    if (arquivo == NULL) {
        printf("\nErro ao abrir arquivo .mem!\n");
        return 1;
    }

    char leitura[64];
    char valor[17];
    int lerDados = 0;
    int i = 0;
    int qtInst = 0;
    int end = 0;

    while (fgets(leitura, sizeof(leitura), arquivo)) {
        leitura[strcspn(leitura, "\n")] = '\0';

        if (strcmp(leitura, ".data") == 0) {
            lerDados = 1;
            continue;
        }

        if (lerDados == 0) {
            if (i >= FIM_INST) {
                printf("\nMemória de instruções cheia!\n");
                break;
            }
            strcpy(memUnificada[i].mem, leitura);
            memUnificada[i].memoria = (uint16_t) strtoul(leitura, NULL, 2);

            qtInst++;
            i++;
        }

        else {

            if (sscanf(leitura, "%d:%16s", &end, valor) == 2) {

                if (end >= INI_DADOS && end <= FIM_DADOS) {
                    strcpy(memUnificada[end].mem, valor);
                    memUnificada[end].memoria = (uint16_t) strtoul(valor, NULL, 2);
                    memUnificada[end].dado = (int8_t) memUnificada[end].memoria;
                } else {
                    printf("\nEndereço de dado inválido: %d\n", end);
                }
            } else {
                printf("\nLinha de dado inválida: %s\n", leitura);
            }
        }
    }

    fclose(arquivo);

    printf("\nMemória carregada!\n");
    printf("Instruções: %d\n", qtInst);

    return qtInst;
}


int *inicializaBReg(){
    return calloc(8, sizeof(int));
}


void escreveMemDados(MemoriaUnificada *memUnificada, int endereco, int8_t valor) {
    if (endereco >= 128 && endereco < 256) {
        memUnificada[endereco].dado = (int8_t) valor;
    } else {
        printf("\nErro ao escrever na memória.\n");
    }
}

int8_t retornaMemoria(int *memDados, uint8_t enderecoULA) {
    return memDados[enderecoULA];
}

void acessoMemoria(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado, MemoriaUnificada *memoria) {
    if (instrucao->opcode == 11) { // LW
        estado->MDR = memoria[estado->ULASaida].memoria;
    } else if (instrucao->opcode == 15) { // SW
        memoria[estado->ULASaida].memoria = estado->B;
    }
}


//----------------------------------------------BUSCA (IF)-----------------------------------------------------

void buscaInstrucao(MemoriaUnificada *memoria, int *pc, regEstado *estado) {
    // Carrega instrução no IR
    estado->IR = memoria[*pc].memoria;
    // Incrementa PC
    (*pc)++;

    printf("\n[Busca] PC=%d, IR=%04x\n", *pc, estado->IR);
}

void programCounter(int *pc, sinaisUC *sinais, MemoriaUnificada *instrucao, int zero, regEstado *estado) {
    if (instrucao->opcode == 8 && zero) { // BEQ
        *pc = estado->ULASaida;
    } else if (instrucao->opcode == 2) { // JUMP
        *pc = estado->IR & 0xFF; // 8 bits menos significativos
    }
}

//------------------------------------------Decodificação (ID)-------------------------------------------------

// Decodifica a instrução guardada no IR e carrega registradores
void decodificaInstrucao(regEstado *estado, int *bReg, MemoriaUnificada *instrucao) {
    uint16_t instr = estado->IR;

    instrucao->opcode = (instr >> 12) & 0xF;
    instrucao->rs     = (instr >> 9) & 0x7;
    instrucao->rt     = (instr >> 6) & 0x7;
    instrucao->rd     = (instr >> 3) & 0x7;
    instrucao->funct  = instr & 0x7;
    instrucao->imm    = instr & 0x3F;

    estado->A = bReg[instrucao->rs];
    estado->B = bReg[instrucao->rt];

    int8_t imm_signed = (instrucao->imm & 0x20) ? (instrucao->imm | 0xC0) : instrucao->imm;
    estado->ULASaida = (*estado->estadoEtapa) + imm_signed;
}

//Decodifica Instrução pro salvaASM
void decodificaInst(MemoriaUnificada *instrucao){

    (*instrucao).opcode = (*instrucao).memoria  >> 12; // Pega os 4 bits do opcode

    switch((*instrucao).opcode){
    case 0:
        (*instrucao).tipoInst = tipoR;
        (*instrucao).rs = ((*instrucao).memoria  >> 9) & 0x7; // pega os 3 bits do rs (desloca 6 bits para a direita e pega os 3 mais significativos que ficaram)
        (*instrucao).rt = ((*instrucao).memoria  >> 6) & 0x7; // pega os 3 bits do rt
        (*instrucao).rd = ((*instrucao).memoria  >> 3) & 0x7; // pega os 3 bits do rd
        (*instrucao).funct = ((*instrucao).memoria ) & 0x7;
        printf("\n[ Tipo Rzz ] \n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("rs: %d\n", (*instrucao).rs);
        printf("rt: %d\n", (*instrucao).rt);
        printf("rd: %d\n", (*instrucao).rd);
        printf("funct: %d\n", (*instrucao).funct);
        break;

    case 2:
        (*instrucao).tipoInst = tipoJ;
        (*instrucao).addr = ((*instrucao).memoria ) &0xFF; // pega os 8 bits do adress
        printf("\n[ Tipo J ]\n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("address: %d\n", (*instrucao).addr);
        break;

    default:
        (*instrucao).tipoInst = tipoI;
        (*instrucao).rs = ((*instrucao).memoria  >> 9) &0x7; // pega os 3 bits do rs
        (*instrucao).rt = ((*instrucao).memoria  >> 6) &0x7; // pega os 3 bits do rt
        (*instrucao).imm = ((*instrucao).memoria ) &0x3F; // pega os 6 bits do imediato (deve passar por um extensor antes da ULA)
        (*instrucao).imm = extensorBit((*instrucao).imm);
        printf("\n[ Tipo I ] \n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("rs: %d\n", (*instrucao).rs);
        printf("rt: %d\n", (*instrucao).rt);
        printf("imediato: %d\n", (*instrucao).imm);
    }
}

void decodifica(MemoriaUnificada *instrucao){

    (*instrucao).opcode = (*instrucao).memoria  >> 12; // Pega os 4 bits do opcode

    switch((*instrucao).opcode){
    case 0:
        (*instrucao).tipoInst = tipoR;
        (*instrucao).rs = ((*instrucao).memoria  >> 9) & 0x7; // pega os 3 bits do rs (desloca 6 bits para a direita e pega os 3 mais significativos que ficaram)
        (*instrucao).rt = ((*instrucao).memoria  >> 6) & 0x7; // pega os 3 bits do rt
        (*instrucao).rd = ((*instrucao).memoria  >> 3) & 0x7; // pega os 3 bits do rd
        (*instrucao).funct = ((*instrucao).memoria ) & 0x7;
        break;

    case 2:
        (*instrucao).tipoInst = tipoJ;
        (*instrucao).addr = ((*instrucao).memoria ) &0xFF; // pega os 8 bits do adress
        break;

    default:
        (*instrucao).tipoInst = tipoI;
        (*instrucao).rs = ((*instrucao).memoria  >> 9) &0x7; // pega os 3 bits do rs
        (*instrucao).rt = ((*instrucao).memoria  >> 6) &0x7; // pega os 3 bits do rt
        (*instrucao).imm = ((*instrucao).memoria ) &0x3F; // pega os 6 bits do imediato (deve passar por um extensor antes da ULA)
        (*instrucao).imm = extensorBit((*instrucao).imm);
    }
}

int8_t extensorBit(int8_t imm){
    imm = imm<<2;
    //printf("\n%d", imm);      // 111111 = -1  <- 00111111 << 2 -> 111111100 >> 2 -> 11111111

    imm = imm>>2;
    //printf("\n%d", imm);

    return imm;
}


//---------------------------------------Unidade de Controle (UC)----------------------------------------------

void unidadeControleMulti(uint8_t opcode, uint8_t funct, int ciclo, sinaisUC *sinais) {
    // Zera sinais
    sinais->branch = 0;
    sinais->jump = 0;
    sinais->IncPC = 0;
    sinais->RegDst = 0;
    sinais->UlaFonte = 0;
    sinais->MemParaReg = 0;
    sinais->EscReg = 0;
    sinais->EscMem = 0;
    sinais->ulaOp = 0;
    sinais->LerMem = 0;
    sinais->IorD = 0;
    sinais->IRWrite = 0;
    sinais->PCWrite = 0;
    sinais->PCWriteCond = 0;
    sinais->PCSource = 0;

    switch(ciclo) {
        case 0: // IF - Instruction Fetch
            sinais->LerMem = 1;   // Habilita leitura da memória
            sinais->IorD = 0;     // Endereço vem do PC
            sinais->IRWrite = 1;  // Carrega instrução no IR

            // Calcula PC + 1
            sinais->UlaFonte = 1; // Usa constante 1 na ULA
            sinais->ulaOp = 0;    // ADD
            sinais->PCWrite = 1;  // Atualiza PC = PC + 1
            sinais->PCSource = 0; // PC vem da saída da ULA
            break;

        case 1: // ID - Instruction Decode / Register Fetch
            // Lê Rs e Rt, calcula endereço do BEQ antecipado
            sinais->UlaFonte = 3; // imediato com extensão de sinal
            sinais->ulaOp = 0;    // ADD: PC + offset → ALUOut
            break;

        case 2: // EX
            switch(opcode) {
                case 0: // Tipo R
                    sinais->ulaOp = funct;
                    sinais->RegDst = 1;   // Rd
                    sinais->UlaFonte = 0; // Reg B
                    break;

                case 4: // Addi
                    sinais->ulaOp = 0;    // ADD
                    sinais->UlaFonte = 2; // Imediato
                    sinais->RegDst = 0;   // Rt
                    break;

                case 8: // BEQ
                    sinais->ulaOp = 1;    // SUB para comparar
                    sinais->UlaFonte = 0; // Reg B
                    sinais->PCWriteCond = 1; // Só escreve PC se Zero=1
                    sinais->PCSource = 1;    // PC vem do ALUOut do ciclo 1
                    break;

                case 11: // LW
                case 15: // SW
                    sinais->ulaOp = 0;    // ADD - calcula endereço
                    sinais->UlaFonte = 2; // Imediato = offset
                    break;

                case 2: // JUMP
                    sinais->PCWrite = 1;
                    sinais->PCSource = 2; // Jump address
                    break;
            }
            break;

        case 3: // MEM
            if(opcode == 11) { // LW
                sinais->LerMem = 1;
                sinais->IorD = 1; // Endereço vem da ULA
            } else if(opcode == 15) { // SW
                sinais->EscMem = 1;
                sinais->IorD = 1;
            }
            break;

        case 4: // WB
            if(opcode == 0) { // Tipo R
                sinais->EscReg = 1;
                sinais->MemParaReg = 0; // resultado vem da ULA
            } else if(opcode == 4) { // Addi
                sinais->EscReg = 1;
                sinais->MemParaReg = 0; // resultado vem da ULA
            } else if(opcode == 11) { // LW
                sinais->EscReg = 1;
                sinais->MemParaReg = 1; // resultado vem da memória
            }
            break;
    }
}

//----------------------------------------Execução (EX, MEM, WB)-----------------------------------------------

void lerRegistradores(int *reg, int8_t rs, int8_t rt, int8_t *valRs, int8_t *valRt){
    *valRs = reg[rs];
    *valRt = reg[rt];
}

void escreveRegistrador(int *reg, int8_t rd, int8_t valor, int EscReg){
    if(EscReg){
        reg[rd] = valor;
    }
}

int8_t ULA(int op1, int op2, int ulaOp, int *zero, int *overflow){
    int resultado = 0;
    *overflow = 0;
    int8_t res_8bit;

    switch(ulaOp){
        case 0: // ADD, LW/SW , ADDI
            resultado = op1 + op2;

                res_8bit = (int8_t)resultado;

                if(resultado != res_8bit){
                    *overflow = 1;
                }
            break;

        case 2: // SUB, BEQ
            resultado = op1 - op2;

                res_8bit = (int8_t)resultado;

                if(resultado != res_8bit){
                    *overflow = 1;
                }
            break;

        case 4: // AND
            resultado = op1 & op2;
            break;

        case 5: // OR
            resultado = op1 | op2;
            break;

        default:
            printf("\nOperação da ULA inválida!\n");
    }
    //FECHA SE FOR OVERFLOW
    if(*overflow == 1){
    printf("\n OVERFLOW!\n");
    exit(1);
}

    // flag zero
    if(resultado == 0){
        *zero = 1;
    } else {
        *zero = 0;
    }

    return resultado;
}

void writeBack(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado) {
    if (instrucao->opcode == 0) { // Tipo R
        bReg[instrucao->rd] = estado->ULASaida;
    } else if (instrucao->opcode == 4) { // addi
        bReg[instrucao->rt] = estado->ULASaida;
    } else if (instrucao->opcode == 11) { // LW
        bReg[instrucao->rt] = estado->MDR;
    }
}

/*
int executaInstrucao(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado) {
    int zero = 0;
    switch(instrucao->opcode) {
        case 0: // Tipo R
            switch(instrucao->funct) {
                case 0: estado->ULASaida = estado->A + estado->B; break;
                case 2: estado->ULASaida = estado->A - estado->B; break;
                case 4: estado->ULASaida = estado->A & estado->B; break;
                case 5: estado->ULASaida = estado->A | estado->B; break;
            }
            break;
        case 4: // addi
            estado->ULASaida = estado->A + instrucao->imm;
            break;
        case 8: // beq
            zero = (estado->A == estado->B);
            break;
        case 11: // lw
        case 15: // sw
            estado->ULASaida = estado->A + instrucao->imm;
            break;
    }
    return zero;
}

//-------------------------------------------Controle de fluxo-------------------------------------------------

void run(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado) {

    while (*pc < 256 && memoria[*pc].memoria != 0) {
        step(memoria, bReg, sinais, pc, estatInst, estado);
    }

    printf("\nFim das instruções!\n");
}

// Lógica errada - está executando um monociclo. Cada step deve executar um ciclo. (Comecei mais ou menos ali para ter uma ideia de como deve rodar a partir do estado sempre)
void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado) {

    if (*pc >= 256 || memoria[*pc].memoria == 0) {
        printf("\nFim das instruções!\n");
        return;
    }

    printf("\nPC = %d | Memória = %s\n", *pc, memoria[*pc].mem);

    switch (*(estado->estadoEtapa)) {
        case 0: // IF
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 0, sinais);
            buscaInstrucao(memoria, pc, estado);

            *(estado->estadoEtapa) = 1;

            break;

        case 1: // ID
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 1, sinais);
            decodificaInstrucao(estado, bReg, &memoria[*pc]);

            // Decide próximo estado conforme opcode
            switch (memoria[*pc].opcode) {
                case 0:  *(estado->estadoEtapa) = 7; break; // Tipo R
                case 4:  *(estado->estadoEtapa) = 2; break; // addi
                case 8:  *(estado->estadoEtapa) = 9; break; // beq
                case 11: *(estado->estadoEtapa) = 2; break; // lw
                case 15: *(estado->estadoEtapa) = 2; break; // sw
                case 2:  *(estado->estadoEtapa) = 10; break; // jump
            }
            break;

        case 2: // EX tipo I (lw/sw/addi)
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 2, sinais);
            executaInstrucao(&memoria[*pc], sinais, bReg, estado);
            if (memoria[*pc].opcode == 11) *(estado->estadoEtapa) = 3; // lw → MEM
            else if (memoria[*pc].opcode == 15) *(estado->estadoEtapa) = 5; // sw → MEM
            else if (memoria[*pc].opcode == 4) *(estado->estadoEtapa) = 6; // addi → WB
            break;

        case 3: // MEM lw
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 3, sinais);
            acessoMemoria(&memoria[*pc], sinais, bReg, estado, memoria);
            *(estado->estadoEtapa) = 4; // próximo é WB
            break;

        case 4: // WB lw
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 4, sinais);
            writeBack(&memoria[*pc], sinais, bReg, estado);
            *(estado->estadoEtapa) = 0; // volta para busca
            (*estatInst).lw++;
            (*estatInst).total++;
            break;

        case 5: // MEM sw
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 3, sinais);
            acessoMemoria(&memoria[*pc], sinais, bReg, estado, memoria);
            *(estado->estadoEtapa) = 0; // termina
            (*estatInst).sw++;
            (*estatInst).total++;
            break;

        case 6: // WB addi
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 4, sinais);
            writeBack(&memoria[*pc], sinais, bReg, estado);
            *(estado->estadoEtapa) = 0;
            (*estatInst).addi++;
            (*estatInst).total++;
            break;

        case 7: // EX tipo R
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 2, sinais);
            executaInstrucao(&memoria[*pc], sinais, bReg, estado);
            *(estado->estadoEtapa) = 8;
            break;

        case 8: // WB tipo R
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 4, sinais);
            writeBack(&memoria[*pc], sinais, bReg, estado);
            *(estado->estadoEtapa) = 0;
            (*estatInst).tipoR++;
            (*estatInst).total++;
            break;

        case 9: // EX beq
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 2, sinais);
            int zero = executaInstrucao(&memoria[*pc], sinais, bReg, estado);
            programCounter(pc, sinais, &memoria[*pc], zero, estado);
            *(estado->estadoEtapa) = 0;
            (*estatInst).beq++;
            (*estatInst).total++;
            break;

        case 10: // EX jump
            unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, 2, sinais);
            programCounter(pc, sinais, &memoria[*pc], 0, estado);
            *(estado->estadoEtapa) = 0;
            (*estatInst).j++;
            (*estatInst).total++;
            break;
    }
}

/*   Parte abaixo ok
decodificaInstrucao(estado, bReg);

// Contabiliza estatísticas
memoria[*pc].decodificado = 1;
    switch(memoria[*pc].tipoInst){
        case tipoI:
            switch(memoria[*pc].opcode){
                case 4: (*estatInst).addi++; break;
                case 8: (*estatInst).beq++; break;
                case 11: (*estatInst).lw++; break;
                case 15: (*estatInst).sw++; break;
            }
            (*estatInst).tipoI++;
            break;
        case tipoJ:
            (*estatInst).j++;
            (*estatInst).tipoJ++;
            break;
        case tipoR:
            switch(memoria[*pc].funct){
                case 0: (*estatInst).add++; break;
                case 2: (*estatInst).sub++; break;
                case 4: (*estatInst).and++; break;
                case 5: (*estatInst).or++; break;
            }
            (*estatInst).tipoR++;
            break;
    }

    (*estatInst).total++;
}
*/

//-----------------------------------------------Impressões----------------------------------------------------

void imprimeBancoRegistradores(int *reg){
    printf("________________________\n");
    printf(" Banco de Registradores \n");
    printf("________________________\n");
    printf(" Registrador |   Valor  \n");
    printf("________________________\n");

    for(int i=0;i<8;i++){
        printf("      %d      |     %d    \n",i, reg[i]);
        printf("_____________|__________\n");
    }
    printf("\n");
}

void imprimeEstatistica(estatInstrucoes estatInst){
    printf("\n========================================\n");
    printf("      Estatísticas de instruções\n");
    printf("========================================\n");
    printf("Total executadas: %d\n", estatInst.total);

    printf("\nPor tipo:\n");
    printf("Tipo R: %d\n", estatInst.tipoR);
    printf("Tipo I: %d\n", estatInst.tipoI);
    printf("Tipo J: %d\n", estatInst.tipoJ);

    printf("\nDetalhamento por instrução:\n");
    printf("R -> add: %d | sub: %d | and: %d | or: %d\n",
           estatInst.add, estatInst.sub, estatInst.and, estatInst.or);
    printf("I -> addi: %d | beq: %d | lw: %d | sw: %d\n",
           estatInst.addi, estatInst.beq, estatInst.lw, estatInst.sw);
    printf("J -> j: %d\n", estatInst.j);
    printf("========================================\n\n");
}

void imprimeInstrucao(MemoriaUnificada *memoria, int pc) {
    if((memoria)[pc].decodificado==0){
        decodifica(&memoria[pc]);
    }

    switch(memoria[pc].opcode){
        case 0: // opcode = 0000

            if(memoria[pc].funct==0){
                printf("add $%d, $%d, $%d", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
            }
            else if((memoria)[pc].funct==2){
                printf("sub $%d, $%d, $%d", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
            }
            else if((memoria)[pc].funct==4){
                printf("and $%d, $%d, $%d", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
            }
            else if((memoria)[pc].funct==5){
                printf("or $%d, $%d, $%d", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
            }
            break;

        case 2: // opcode = 0010 - J
            int x=12;
            printf("j %d%*s", (memoria)[pc].addr, x, "");

            break;

        case 4: // opcode = 0100 - Addi
            printf("addi $%d, $%d, %d", (memoria)[pc].rt, (memoria)[pc].rs, (memoria)[pc].imm);

            break;

        case 8: // opcode = 1000 - BEQ
            printf("beq $%d, $%d, %d", (memoria)[pc].rs, (memoria)[pc].rt, (memoria)[pc].imm);

            break;

        case 11: // opcode = 1011 - lw
            printf("lw $%d, %d($%d)", (memoria)[pc].rt, (memoria)[pc].imm, (memoria)[pc].rs);

            break;

        case 15: // opcode = 1111 - sw
            printf("sw $%d, %d($%d)", (memoria)[pc].rt, (memoria)[pc].imm, (memoria)[pc].rs);

            break;
    }
}

void imprimeMemorias(MemoriaUnificada *memoria){
    int opt, x;
    do{
        printf("\n1. Memória de instruções\n2. Memória de dados\n");
        printf("\nSelecione uma das opções acima: ");
        scanf("%d", &opt);

        switch(opt){
            case 1:
	            x = 70;

                printf("\n%*sMemória de Instruções:\n\n", x, "");

                for (int linha = 0; linha < 64; linha++) {
                    printf(" %3d: %16s: ", linha, memoria[linha].mem);
                    imprimeInstrucao(memoria, linha);

                    printf("\t %3d: %16s: ", linha + 64, memoria[linha + 64].mem);
                    imprimeInstrucao(memoria, linha + 64);

                    printf("\n");
                }
                printf("\n");
                break;

            case 2:

                x = 20;

                printf("\n%*sMemória de Dados:\n\n", x, "");

                for (int linha = 0; linha < 32; linha++) {
                    printf("%3d: %3d\t %3d: %3d\t %3d: %3d\t %3d: %3d\n",
                    128 + linha, memoria[128 + linha].dado,
                    128 + linha + 32, memoria[128 + linha + 32].dado,
                    128 + linha + 64, memoria[128 + linha + 64].dado,
                    128 + linha + 96, memoria[128 + linha + 96].dado);
                }
                printf("\n");
                break;

            default:
                printf("Opção inválida! Por favor, selecione uma das opções disponíveis.\n");
        }
    }while(opt<1 || opt>2);
}


//-----------------------------------------------Salvamentos---------------------------------------------------

void salvaASM(MemoriaUnificada *memoria, int qntdInst) {
    int pc = 0;
    char nomeASM[50]={0}, nome[20], extensao[] = ".asm", resposta;

    printf("\nDigite o nome do arquivo que deseja salvar (.asm): ");
    fgets(nome, sizeof(nome),stdin);
    nome[strcspn(nome,"\n")]='\0';
    int indice=1;

    strcat(nomeASM,nome);
    strcat(nomeASM,extensao);

    // Verifica se o arquivo existe
    while (access(nomeASM, F_OK) != -1) {
        printf("\nJá existe um arquivo com o nome %s, deseja sobrescrever? (s/n): ", nomeASM);
        scanf(" %c", &resposta);

        if (resposta == 's' || resposta == 'S') {
            break;
        } else if (resposta == 'n' || resposta == 'N') {
            snprintf(nomeASM, sizeof(nomeASM), "%s_%d%s", nome, indice, extensao);
            indice++;
        } else {
            printf("\nOpção inválida. Tente novamente.\n");
        }
    }

    arquivo = fopen(nomeASM, "w");

    if (arquivo == NULL) {
        printf("\nErro ao criar arquivo\n");
        return;
    }

    while(pc <= qntdInst - 1){
        if((memoria)[pc].decodificado==0){
            decodificaInst(&memoria[pc]);
        }


        switch(memoria[pc].opcode){
            case 0: // opcode = 0000

                if(memoria[pc].funct==0){
                    fprintf(arquivo,"add $%d, $%d, $%d\n", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
                }
                else if((memoria)[pc].funct==2){
                    fprintf(arquivo,"sub $%d, $%d, $%d\n", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
                }
                else if((memoria)[pc].funct==4){
                    fprintf(arquivo,"and $%d, $%d, $%d\n", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
                }
                else if((memoria)[pc].funct==5){
                    fprintf(arquivo,"or $%d, $%d, $%d\n", (memoria)[pc].rd, (memoria)[pc].rs, (memoria)[pc].rt);
                }
                break;

            case 2: // opcode = 0010 - J
                fprintf(arquivo,"j %d\n", (memoria)[pc].addr);

                break;

            case 4: // opcode = 0100 - Addi
                fprintf(arquivo,"addi $%d, $%d, %d\n", (memoria)[pc].rt, (memoria)[pc].rs, (memoria)[pc].imm);

                break;

            case 8: // opcode = 1000 - BEQ
                fprintf(arquivo,"beq $%d, $%d, %d\n", (memoria)[pc].rs, (memoria)[pc].rt, (memoria)[pc].imm);

                break;

            case 11: // opcode = 1011 - lw
                fprintf(arquivo,"lw $%d, %d($%d)\n", (memoria)[pc].rt, (memoria)[pc].imm, (memoria)[pc].rs);

                break;
            case 15: // opcode = 1111 - sw
                fprintf(arquivo,"sw $%d, %d($%d)\n", (memoria)[pc].rt, (memoria)[pc].imm, (memoria)[pc].rs);

                break;
        }

        pc++;
    }

    fclose(arquivo);

    printf("\nArquivo '%s' salvo!\n",nomeASM);
}

/*
void salvaDAT(int *memDados){
    char nomeDAT[50]={0}, nome[20], extensao[] = ".dat", resposta;

    printf("\nDigite o nome do arquivo que deseja salvar (.dat): ");
    fgets(nome, sizeof(nome),stdin);
    nome[strcspn(nome,"\n")]='\0';
    int indice=1;

    strcat(nomeDAT,nome);
    strcat(nomeDAT,extensao);

    // Verifica se o arquivo existe
    while (access(nomeDAT, F_OK) != -1) {
        printf("\nJá existe um arquivo com o nome %s, deseja sobrescrever? (s/n): ", nomeDAT);
        scanf(" %c", &resposta);

        if (resposta == 's' || resposta == 'S') {
            break;
        } else if (resposta == 'n' || resposta == 'N') {
            snprintf(nomeDAT, sizeof(nomeDAT), "%s_%d%s", nome, indice, extensao);
            indice++;
        } else {
            printf("\nOpção inválida. Tente novamente.\n");
        }
    }

    arquivo = fopen(nomeDAT,"w");

    if (arquivo == NULL) {
        printf("\nErro ao criar arquivo\n");
        return;
    }

    for(int i=0;i<256;i++){
        fprintf(arquivo,"%d\n",memDados[i]);
    }

    fclose(arquivo);

    printf("\nArquivo '%s' salvo!\n",nomeDAT);
}


//------------------------------------------------Histórico----------------------------------------------------

void salvaEstado(historico *hist, int pc, int *bReg, estatInstrucoes *estatInst, MemoriaUnificada *mem){
    if(hist->topo >= MAX_HIST) return;

    regEstado *e = &hist->estados[hist->topo];

    e->pc = pc;

    for(int i=INI_DADOS;i<FIM_DADOS;i++)
        e->memDados[i] = mem[i].memoria;

    for(int i=0;i<8;i++)
        e->bReg[i] = bReg[i];

    e->estat = *estatInst; // <-- SALVA ESTATÍSTICAS

    hist->topo++;
}

void voltaInstrucao(historico *hist, int *pc, int *bReg, estatInstrucoes *estatInst){
    if(hist->topo <= 0){
        printf("\nSem histórico!\n");
        return;
    }

    hist->topo--;

    regEstado *e = &hist->estados[hist->topo];

    *pc = e->pc;

    //for(int i=0;i<256;i++)
     //   memDados[i] = e->memDados[i];

    for(int i=0;i<8;i++)
        bReg[i] = e->bReg[i];

    *estatInst = e->estat; // <-- RESTAURA ESTATÍSTICAS

    printf("\nVoltou uma instrução!\n");
    printf("PC atual: %d.\n", *pc);
}
*/
