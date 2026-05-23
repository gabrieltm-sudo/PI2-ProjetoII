#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "minimips.h"

FILE *arquivo = NULL;
FILE *arquivoMemDados = NULL;

//---------------------------------------LEITURA E INICIALIZAÇÃO------------------------------------------------

int lerMemUnificada(char *arq, MemoriaUnificada *memUnificada) {
    FILE *arquivo = fopen(arq, "r");

    if (arquivo == NULL) {
        printf("\n[ERRO] Não foi possível abrir o arquivo .mem.\n");
        return 0;
    }

    char leitura[64];
    char valor[17];
    int lerDados = 0;
    int i = 0;
    int qtInst = 0;
    int end = 0;

    // Inicializa toda a memória com zeros
    for (int j = 0; j < 256; j++) {
        strcpy(memUnificada[j].mem, "0000000000000000");
        memUnificada[j].memoria = 0;
        memUnificada[j].dado = 0;
    }

    while (fgets(leitura, sizeof(leitura), arquivo)) {
        leitura[strcspn(leitura, "\n")] = '\0';

        if (strcmp(leitura, ".data") == 0) {
            lerDados = 1;
            continue;
        }

        if (lerDados == 0) {
            if (i >= FIM_INST) {
                printf("\n[ERRO] Memória de instruções cheia.\n");
                break;
            }
            strcpy(memUnificada[i].mem, leitura);
            memUnificada[i].memoria = (uint16_t) strtoul(leitura, NULL, 2);

            qtInst++;
            i++;
        } else {
            if (sscanf(leitura, "%d:%16s", &end, valor) == 2) {
                if (end >= INI_DADOS && end <= FIM_DADOS) {
                    strcpy(memUnificada[end].mem, valor);
                    memUnificada[end].memoria = (uint16_t) strtoul(valor, NULL, 2);
                    memUnificada[end].dado = (int8_t) memUnificada[end].memoria;
                } else {
                    printf("\n[ERRO] Endereço de dado inválido: %d\n", end);
                }
            } else {
                printf("\n[ERRO] Linha de dado inválida: %s\n", leitura);
            }
        }
    }

    fclose(arquivo);

    printf("\n==========================================\n");
    printf("Memória carregada\n");
    printf("Instruções encontradas: %d\n", qtInst);
    printf("\n==========================================\n");

    return qtInst;
}


int *inicializaBReg(){
    return calloc(8, sizeof(int));
}


void escreveMemDados(MemoriaUnificada *memUnificada, int endereco, int8_t valor) {
    if (endereco >= 128 && endereco < 256) {
        memUnificada[endereco].dado = (int8_t) valor;
    } else {
        printf("\n[ERRO] Não foi possível escrever na memória.\n");
    }
}



void acessoMemoria(regEstado *estado, MemoriaUnificada *memoria) {
    
    if(estado->ULASaida>=128 && estado->ULASaida<=255){

        if (estado->opcode == 11) { // LW
            
            estado->MDR = memoria[estado->ULASaida].dado;
            
        }else if (estado->opcode == 15) { // SW
            
            int endereco = estado->ULASaida;
            
            memoria[endereco].dado = (int8_t)estado->B;
            
            // deixa os 8 primeiros bits em 0
            memoria[endereco].memoria = (uint16_t)((uint8_t)estado->B);
            
            for(int i = 15; i >= 0; i--){
                memoria[endereco].mem[15 - i] = ((memoria[endereco].memoria >> i) & 1) + '0';
            }
            
            memoria[endereco].mem[16] = '\0';
        }
    }else{
        printf("\n[Erro] Erro ao acessar a memória. Endereço inválido\n");
    }
}
    
    
    //----------------------------------------------BUSCA (IF)-----------------------------------------------------
    
void buscaInstrucao(MemoriaUnificada *memoria, int *pc, regEstado *estado) {
    // Carrega instrução no IR
    estado->IR = memoria[*pc].memoria;

    printf("\n==========================================\n");
    printf("Busca\n");
    printf("==========================================\n");
}


//------------------------------------------Decodificação-------------------------------------------------

// Decodifica a instrução guardada no IR e carrega registradores
void decodificaInstrucao(int pc, regEstado *estado, int *bReg){
    uint16_t instr = estado->IR;

    estado->opcode = instr >> 12;

    switch(estado->opcode){
        case 0: // Tipo R
            estado->tipoInst = tipoR;
            estado->rs = (instr >> 9) & 0x7;
            estado->rt = (instr >> 6) & 0x7;
            estado->rd = (instr >> 3) & 0x7;
            estado->funct = instr & 0x7;
            break;

        case 2: // Tipo J
            estado->tipoInst = tipoJ;
            estado->addr = instr & 0xFF;
            break;

        default: // Tipo I
            estado->tipoInst = tipoI;
            estado->rs = (instr >> 9) & 0x7;
            estado->rt = (instr >> 6) & 0x7;
            estado->imm = instr & 0x3F;
            estado->imm = extensorBit(estado->imm);
            break;
    }

    if(estado->tipoInst != tipoJ){
        estado->A = bReg[estado->rs];
        estado->B = bReg[estado->rt];
    }


}

int8_t extensorBit(int8_t imm){
    imm = imm<<2;
    imm = imm>>2;

    return imm;
}

//---------------------------------------Unidade de Controle (UC)----------------------------------------------

void unidadeControleMulti(regEstado *estado, sinaisUC *sinais) {
    // Zera sinais
    *sinais = (sinaisUC){0};

    switch(estado->estadoAtual) {
        case 0: //  Estado 0 - Busca
            sinais->LerMem = 1;
            sinais->IouD = 0;
            sinais->IREsc = 1;

            // Calcula PC + 1
            sinais->UlaFonteB = 1; // 01 (Usa constante 1)
            sinais->UlaFonteA = 0; // PC vai para a ULA
            sinais->ControleUla = 0; // ULA faz soma
            sinais->PCEsc = 1;  // Atualiza PC
            sinais->PCFonte = 0; // PC vem da saída da ULA

            sinais->IouD = 0; // Memória acessa valor apontado pelo PC
            sinais->RegDst = 1; // don't care

            break;
        case 1: // Estado 1 - Decodificação
            sinais->PCEsc = 0;

            sinais->RegDst = 1;
            // BEQ
            sinais->UlaFonteA = 0; // PC vai para a ULA
            sinais->UlaFonteB = 2; // 10 - Imm extendido
            sinais->ControleUla = 0; // ULA faz soma pois calcula PC + imm extendido para o BEQ

            break;
        case 2: // 2º Estado - Execução tipo I - cálculo do endereço base+deslocamento ou rs + imm
            sinais->UlaFonteA = 1; // rs
            sinais->UlaFonteB = 2; // 10 - Imm extendido
            sinais->ControleUla = 0; // Faz soma

            break;
        case 3: // 3º Estado - Acesso à memória (LW)
            sinais->IouD = 1; // Acessa memória de dados
            sinais->EscMem = 0; // Não escreve na memória
            sinais->UlaFonteB = 2; // 10
            sinais->UlaFonteA = 1;

            break;
        case 4: // 4º Estado - Finalização LW
            sinais->IouD = 1;
            sinais->MemParaReg = 1;
            sinais->EscReg = 1;
            sinais->UlaFonteB = 2;
            sinais->UlaFonteA = 1;

            break;
        case 5: // 5º Estado - Acesso à memória (SW)
            sinais->IouD = 1;
            sinais->EscMem = 1;
            sinais->UlaFonteB = 2;
            sinais->UlaFonteA = 1;

            break;
        case 6: // 6º Estado - addi
            sinais->EscReg = 1;
            sinais->UlaFonteB = 2;
            sinais->UlaFonteA = 1;
            sinais->ControleUla = 0; // Ula faz soma

            break;
        case 7: // 7º Estado - Execução tipo R
            sinais->RegDst = 1;
            sinais->UlaFonteA = 1;
            sinais->UlaFonteB = 0;
            sinais->ControleUla = 2; // 011 - Faz operação de acordo com funct da instrução

            break;
        case 8: // 8º Estado - Término da tipo R
            sinais->RegDst = 1;
            sinais->EscReg = 1;
            sinais->MemParaReg = 0;

            break;
        case 9: // 9º Estado - Término BEQ
            sinais->PCFonte = 1; // PC recebe o endereço calculado no estado 1
            sinais->branch = 1;
            sinais->UlaFonteA = 1;
            sinais->UlaFonteB = 0;
            sinais->ControleUla = 1; // 010 - ULA faz subtração

            break;
        case 10: // 10º Estado - Jump
            sinais->PCEsc = 1;
            sinais->UlaFonteA = 0;
            sinais->PCFonte = 2; // 10 - Imediato vai para o PC

        break;
    }

}

void defineEstado(int *estadoAtual, uint8_t opcode){

    switch(*estadoAtual){
        case 0:
            *estadoAtual = 1;
            break;
        case 1:
            switch(opcode){
                case 0: // Tipo R
                    *estadoAtual = 7;
                    break;
                case 2: // Jump
                    *estadoAtual = 10;
                    break;
                case 8: // BEQ
                    *estadoAtual = 9;
                    break;
                default: // Tipo I (addi, sw e lw)
                    *estadoAtual = 2;
                    break;
            }
            break;
        case 2:
            switch(opcode){
                case 4: // addi
                    *estadoAtual = 6;
                    break;
                case 11: // lw
                    *estadoAtual = 3;
                    break;
                case 15: // sw
                    *estadoAtual = 5;
                    break;
                default:
                    *estadoAtual = 0;
                    break;
            }
            break;
        case 3:
            *estadoAtual = 4;
            break;
        case 7:
            *estadoAtual = 8;
            break;

        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            *estadoAtual = 0;
            break;
    }
}

//----------------------------------------Execução (EX, MEM, WB)-----------------------------------------------

int ULA(int op1, int op2, int ControleUla, int *zero, int *overflow, regEstado *estado){
    int resultado = 0;
    *overflow = 0;
    int8_t res_8bit;

    switch(ControleUla){
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
    if(*overflow){
        printf("\nOVERFLOW!\n");
            if(estado->opcode != 11 && estado->opcode != 15){
                exit(1);
            }
        }
    
    // flag zero
    if(resultado == 0){
        *zero = 1;
    } else {
        *zero = 0;
    }

    return resultado;
}

int ULAcontrole(int ControleUla, int funct){

    switch(ControleUla){
        case 0:
            return 0; // ADD, LW/SW , ADDI

        case 1:
            return 2;  // SUB, BEQ

        case 2: // utiliza e respeita o funct
            switch(funct){
                case 0:
                    return 0; //ADD

                case 2:
                    return 2; //SUB

                case 4:
                    return 4; //AND

                case 5:
                    return 5; //OR

                default:
                    printf("\nFunct inválido!\n");
                    exit(1);
            }
            default:
                printf("\nALUop inválido!\n");
                exit(1);
    }
}

//-------------------------------------------Controle de fluxo-------------------------------------------------

void run(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado) {

    while (estado->estadoAtual != 0 || (*pc < 256 && memoria[*pc].memoria != 0)) {
        step(memoria, bReg, sinais, pc, estatInst, estado);
    }

    printf("\n==========================================\n");
    printf("Fim das instruções\n");
    printf("==========================================\n");
}

void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc,
    estatInstrucoes *estatInst, regEstado *estado) {
     int zero = 0;

    if (estado->estadoAtual == 0 && (*pc >= 256 || memoria[*pc].memoria == 0)) {
        printf("\n==========================================\n");
        printf("Fim das instruções\n");
        printf("==========================================\n");
        return;
    }

    printf("\n[Estado atual] %d\n", estado->estadoAtual);
    printf("[PC] %d\n", *pc);

    // Controle e execução do ciclo
    unidadeControleMulti(estado, sinais);
    executaCiclo(memoria, sinais, bReg, estado, &zero, pc);

    // Estatísticas
    if(estado->estadoAtual == 1){

        switch(estado->tipoInst){
        case tipoI:
            switch(estado->opcode){
                case 4: estatInst->addi++; break;
                case 8: estatInst->beq++; break;
                case 11: estatInst->lw++; break;
                case 15: estatInst->sw++; break;
            }
            estatInst->tipoI++;
            break;
        case tipoJ:
            estatInst->j++;
            estatInst->tipoJ++;
            break;
        case tipoR:
            switch(estado->funct){
                case 0: estatInst->add++; break;
                case 2: estatInst->sub++; break;
                case 4: estatInst->and++; break;
                case 5: estatInst->or++; break;
            }
            estatInst->tipoR++;
            break;
        }

        estatInst->total++;
    }

    defineEstado(&estado->estadoAtual, estado->opcode);
    printf("[Próximo estado] %d\n", estado->estadoAtual);
}

void executaCiclo(MemoriaUnificada *memoria, sinaisUC *sinais, int *bReg,regEstado *estado, int *zero, int *pc) {
    int op1, op2, novoPc, overflow, operacaoULA;

    // MUXs do UlaFonte e PCFonte
    if(sinais->UlaFonteA == 0)
        op1 = *pc;
    else if(sinais->UlaFonteA == 1)
        op1 = estado->A;

    if(sinais->UlaFonteB == 0)
        op2 = estado->B;
    else if(sinais->UlaFonteB == 1)
        op2 = 1;
    else if(sinais->UlaFonteB == 2)
        op2 = estado->imm; // usa instrução já buscada

    if(sinais->PCFonte == 0)
        novoPc = estado->ULASaida;
    else if(sinais->PCFonte == 2)
        novoPc = estado->addr;

    operacaoULA = ULAcontrole(sinais->ControleUla, estado->funct);

    printf("\n==========================================================\n");
    printf(" Ciclo atual | Estado: %d | PC: %d\n", estado->estadoAtual, *pc);
    printf("==========================================================\n");

    switch(estado->estadoAtual){
        case 0: // Busca
            printf("\n[IF] Busca de instrução\n");
            buscaInstrucao(memoria, pc, estado);
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow, estado);
            if(sinais->PCEsc){
                *pc = estado->ULASaida;
                printf("[PC] Atualizado para %d\n", *pc);
            }
            break;
        case 1: // Decodificação
            printf("\n[ID] Decodificação\n");
            decodificaInstrucao(*pc - 1, estado, bReg);

            switch(estado->opcode){
                case 0: // Tipo R
                    printf("Tipo R  | opcode: %d | rs: %d | rt: %d | rd: %d | funct: %d\n", estado->opcode, estado->rs, estado->rt, estado->rd, estado->funct);
                    break;

                case 2: // Tipo J
                    printf("Tipo J  | opcode: %d | addr: %d\n", estado->opcode, estado->addr);
                    break;

                default: // Tipo I
                    printf("Tipo I  | opcode: %d | rs: %d | rt: %d | imm: %d\n", estado->opcode, estado->rs, estado->rt, estado->imm);
                    break;
            }

            imprimeInstrucao(memoria, *pc - 1, estado, bReg);
            printf("\n");


            estado->ULASaida = ULA(*pc, (extensorBit(estado->IR & 0x3F)+1),  operacaoULA, zero, &overflow, estado);

            printf("[EX] Endereço de desvio calculado: %d\n", estado->ULASaida);
            break;
        case 2: // Execução tipo I
            printf("\n[EX] Execução tipo I\n");
         
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow, estado);
            printf("[ULA] Resultado = %d\n", estado->ULASaida);

            break;
        case 3: // LW - leitura memória
            printf("\n[MEM] LW - leitura de memória\n");
            acessoMemoria(estado, memoria);
            printf("[MEMD] mem[%d] -> MDR = %d\n", estado->ULASaida, estado->MDR);



            break;
        case 4: // LW - write back
            printf("\n[WB] LW - escrita no banco\n");
            if(sinais->EscReg){
                bReg[estado->rt] = estado->MDR;   // Reg[rt] ← MDR
                printf("[BREG] $%d = %d\n", estado->rt, estado->MDR);
            }
            break;

        case 5: // SW - Write Memory
            printf("\n[MEM] SW - escrita em memória\n");
            if(sinais->EscMem){
                acessoMemoria(estado, memoria);
                printf("[MEMD] mem[%d] = %d\n", estado->ULASaida, estado->B);
            }

            break;

        case 6: // ADDI - Finalização
            printf("\n[WB] ADDI - escrita no banco\n");
            if(sinais->EscReg){
                bReg[estado->rt] = estado->ULASaida;  // Reg[rt] ← resultado da ULA
                printf("[BREG] $%d = %d\n", estado->rt, estado->ULASaida);
            }

            break;

        case 7: // Execução tipo R
            printf("\n[EX] Execução tipo R\n");
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow, estado);
            printf("[ULA] Resultado = %d\n", estado->ULASaida);

            break;

        case 8: // Tipo R - Write Back
            printf("\n[WB] Tipo R - escrita no banco\n");
            if(sinais->EscReg){
                bReg[estado->rd] = estado->ULASaida;  // Reg[rd] ← resultado da ULA
                printf("[BREG] $%d = %d\n", estado->rd, estado->ULASaida);
            }

            break;

        case 9: // BEQ
            int resultado;
            printf("\n[EX] BEQ\n");
            resultado = ULA(op1, op2, operacaoULA, zero, &overflow, estado);
            if((sinais->branch == 1 && *zero == 1) || sinais->PCEsc == 1){ // Verificar
                *pc = estado->ULASaida;
                printf("[BRANCH] tomado -> PC = %d\n", *pc);
            } else {
                printf("[BRANCH] não tomado\n");
            }
            break;
        case 10: // Jump
            printf("\n[EX] Jump\n");
            if(sinais->PCEsc == 1){
                *pc = novoPc;
                printf("[PC] Atualizado para %d\n", *pc);
            }

            break;
    }

    printf("\n-----------------------------\n");
    printf(" Registradores temporários\n");
    printf("-----------------------------\n");
    printf("IR       : ");
    for(int i = 15; i >= 0; i--) {
        printf("%d", (estado->IR >> i) & 1);
    }
    printf("\n");
    printf("MDR      : %d\n", estado->MDR);
    printf("A        : %d\n", estado->A);
    printf("B        : %d\n", estado->B);
    printf("ULASaida : %d\n", estado->ULASaida);
    printf("-----------------------------\n");

}

//-----------------------------------------------Impressões----------------------------------------------------

void imprimeBancoRegistradores(int *reg){
    printf("\n==========================================\n");
    printf("Banco de registradores\n");
    printf("==========================================\n");
    printf("Reg  | Valor\n");
    printf("----------------\n");

    for(int i=0;i<8;i++){
        printf("$%d   | %d\n", i, reg[i]);
    }
    printf("\n");
}

void imprimeEstatistica(estatInstrucoes estatInst){
    printf("\n==========================================\n");
    printf("Estatísticas de instruções\n");
    printf("==========================================\n");
    printf("Total executadas: %d\n", estatInst.total);

    printf("\nPor tipo\n");
    printf("R: %d\n", estatInst.tipoR);
    printf("I: %d\n", estatInst.tipoI);
    printf("J: %d\n", estatInst.tipoJ);

    printf("\nPor instrução\n");
    printf("add : %d | sub : %d | and : %d | or : %d\n",
           estatInst.add, estatInst.sub, estatInst.and, estatInst.or);
    printf("addi: %d | beq: %d | lw  : %d | sw  : %d\n",
           estatInst.addi, estatInst.beq, estatInst.lw, estatInst.sw);
    printf("j: %d\n", estatInst.j);
    printf("==========================================\n\n");
}

void imprimeInstrucao(MemoriaUnificada *memoria, int pc, regEstado *estado, int *bReg) {
    int x = 8;

    switch(estado->opcode){
        case 0: // Tipo R
            if(estado->funct==0)
                printf("add $%d, $%d, $%-4d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==2)
                printf("sub $%d, $%d, $%-4d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==4)
                printf("and $%d, $%d, $%-4d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==5)
                printf("or $%d, $%d, $%-5d", estado->rd, estado->rs, estado->rt);
            break;

        case 2: // Jump
            printf("j %-15d", estado->addr);
            break;

        case 4: // Addi
            printf("addi $%d, $%d, %-4d", estado->rt, estado->rs, estado->imm);
            break;

        case 8: // BEQ
            printf("beq $%d, $%d, %-5d", estado->rs, estado->rt, estado->imm);
            break;

        case 11: // LW
            printf("lw $%d, %d($%d)%-5s", estado->rt, estado->imm, estado->rs,"");
            break;

        case 15: // SW
            printf("sw $%d, %d($%d)%-5s", estado->rt, estado->imm, estado->rs,"");
            break;
    }
}

void imprimeMemorias(MemoriaUnificada *memoria, int *bReg){
    regEstado temp;
    printf("\n==========================================\n");
    printf("Memória\n");
    printf("==========================================\n");

    printf("\n %-3s |   %-16s|   %-18s|| %-3s |   %-16s|   %-18s|| %-3s |   %-16s|  %-8s|| %-3s |   %-16s| %-8s\n",
        "End", "Memória", "Instrução",
        "End", "Memória", "Instrução",
        "End", "Memória", "Dado",
        "End", "Memória", "Dado");
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    for (int linha = 0; linha < 64; linha++) {
     int linhaInst1 = linha;
     int linhaInst2 = linha + 64;
     int linhaDado1 = 128 + linha;
     int linhaDado2 = 128 + linha + 64;

     temp.IR = memoria[linhaInst1].memoria;
     decodificaInstrucao(linhaInst1, &temp, bReg);
     printf(" %3d | %16s | ", linhaInst1, memoria[linhaInst1].mem);
        imprimeInstrucao(memoria, linha, &temp, bReg);

     temp.IR = memoria[linhaInst2].memoria;
     decodificaInstrucao(linhaInst2, &temp, bReg);
     printf(" || %3d | %16s | ", linhaInst2, memoria[linhaInst2].mem);
     imprimeInstrucao(memoria, linhaInst2, &temp, bReg);

     printf(" || %3d | %16s | %8d || %3d | %16s | %8d\n",
         linhaDado1, memoria[linhaDado1].mem, memoria[linhaDado1].dado,
         linhaDado2, memoria[linhaDado2].mem, memoria[linhaDado2].dado);
    }

    printf("\n");
}

//-----------------------------------------------Salvamentos---------------------------------------------------
void salvaMem(MemoriaUnificada *memoria, int qntdInst) {
    int pc = 0;
    char nomeMEM[50] = {0}, nome[40] = {0}, extensao[] = ".mem", resposta;

    printf("\nNome do arquivo .mem: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = '\0';

    int indice = 1;

    snprintf(nomeMEM, sizeof(nomeMEM), "%s%s", nome, extensao);

    while (access(nomeMEM, F_OK) != -1) {
        printf("\nArquivo '%s' já existe. Sobrescrever? (s/n): ", nomeMEM);
        scanf(" %c", &resposta);

        if (resposta == 's' || resposta == 'S') {
            break;
        } else if (resposta == 'n' || resposta == 'N') {
            snprintf(nomeMEM, sizeof(nomeMEM), "%s_%d%s", nome, indice, extensao);
            indice++;
        } else {
            printf("\n[ERRO] Opção inválida. Tente novamente.\n");
        }
    }

    arquivo = fopen(nomeMEM, "w");

    if (arquivo == NULL) {
        printf("\n[ERRO] Não foi possível criar o arquivo.\n");
        return;
    }

    for (int i = 0; i < qntdInst && i < 128; i++) {
        fprintf(arquivo, "%s\n", memoria[i].mem);
    }

    fprintf(arquivo, ".data\n");

    for (int addr = 128; addr < 256; addr++) {
        if (memoria[addr].dado != 0 || strcmp(memoria[addr].mem, "0000000000000000") != 0) {
            fprintf(arquivo, "%d:%s\n", addr, memoria[addr].mem);
        }
    }

    fclose(arquivo);

    printf("\nArquivo salvo: %s\n", nomeMEM);
}

void salvaASM(MemoriaUnificada *memoria, int qntdInst, regEstado *estado,int *bReg) {
    int pc = 0;
    char nomeASM[50]={0}, nome[20], extensao[] = ".asm", resposta;

    printf("\nNome do arquivo .asm: ");
    fgets(nome, sizeof(nome),stdin);
    nome[strcspn(nome,"\n")]='\0';
    int indice=1;

    strcat(nomeASM,nome);
    strcat(nomeASM,extensao);

    // Verifica se o arquivo existe
    while (access(nomeASM, F_OK) != -1) {
        printf("\nArquivo '%s' já existe. Sobrescrever? (s/n): ", nomeASM);
        scanf(" %c", &resposta);

        if (resposta == 's' || resposta == 'S') {
            break;
        } else if (resposta == 'n' || resposta == 'N') {
            snprintf(nomeASM, sizeof(nomeASM), "%s_%d%s", nome, indice, extensao);
            indice++;
        } else {
            printf("\n[ERRO] Opção inválida. Tente novamente.\n");
        }
    }

    arquivo = fopen(nomeASM, "w");

    if (arquivo == NULL) {
        printf("\n[ERRO] Não foi possível criar o arquivo.\n");
        return;
    }

    while(pc <= qntdInst - 1){
        // Carrega a instrução no IR
        estado->IR = memoria[pc].memoria;

        // Decodifica a instrução atual
        decodificaInstrucao(pc, estado, bReg);

        // Agora imprime usando os campos de estado
        switch(estado->opcode){
            case 0: // Tipo R
                if(estado->funct==0){
                    fprintf(arquivo,"add $%d, $%d, $%d\n", estado->rd, estado->rs, estado->rt);
                }
                else if(estado->funct==2){
                    fprintf(arquivo,"sub $%d, $%d, $%d\n", estado->rd, estado->rs, estado->rt);
                }
                else if(estado->funct==4){
                    fprintf(arquivo,"and $%d, $%d, $%d\n", estado->rd, estado->rs, estado->rt);
                }
                else if(estado->funct==5){
                    fprintf(arquivo,"or $%d, $%d, $%d\n", estado->rd, estado->rs, estado->rt);
                }
                break;

            case 2: // Jump
                fprintf(arquivo,"j %d\n", estado->addr);
                break;

            case 4: // Addi
                fprintf(arquivo,"addi $%d, $%d, %d\n", estado->rt, estado->rs, estado->imm);
                break;

            case 8: // BEQ
                fprintf(arquivo,"beq $%d, $%d, %d\n", estado->rs, estado->rt, estado->imm);
                break;

            case 11: // LW
                fprintf(arquivo,"lw $%d, %d($%d)\n", estado->rt, estado->imm, estado->rs);
                break;

            case 15: // SW
                fprintf(arquivo,"sw $%d, %d($%d)\n", estado->rt, estado->imm, estado->rs);
                break;
        }

        pc++;
    }

    fclose(arquivo);

    printf("\nArquivo salvo: %s\n", nomeASM);
}

//------------------------------------------------Histórico----------------------------------------------------

Historico* criaHistorico() {
    Historico *h = (Historico *)malloc(sizeof(Historico));
    h->topo=NULL;
    return h;
}

void salvaEstado(Historico *h, int pc, int *bReg, estatInstrucoes estat, regEstado *reg, MemoriaUnificada *memoria) {
    Estado *novo = malloc(sizeof(Estado));
    if (!novo) return;

    novo->pc = pc;
    memcpy(novo->bReg, bReg, sizeof(int)*8);
    novo->estat = estat;
    novo->estadoAtual = reg->estadoAtual;

    novo->estado = malloc(sizeof(regEstado));
    if (!novo->estado) { free(novo); return; }
    *novo->estado = *reg;

    novo->memoria = malloc(sizeof(MemoriaUnificada) * TAM_MEMORIA);
    if (!novo->memoria) {
        free(novo->estado);
        free(novo);
        return;
    }
    memcpy(novo->memoria, memoria, sizeof(MemoriaUnificada) * TAM_MEMORIA);

    novo->anterior = h->topo;
    h->topo = novo;
}

Estado* voltaEstado(Historico *h) {
    if (h->topo == NULL || h->topo->anterior == NULL) {
        printf("\nNão há mais estados para voltar.\n");
        return NULL;
    }

    Estado *removido = h->topo;
    h->topo = removido->anterior;
    removido->anterior = NULL;
    
    printf("\nPC: %d\n", removido->pc);
    printf("Estado atual: %d\n\n", removido->estadoAtual);
    printf("\n-----------------------------\n");
    printf(" Registradores temporários\n");
    printf("-----------------------------\n");
    printf("IR       : ");
    for(int i = 15; i >= 0; i--) {
        printf("%d", (removido->estado->IR >> i) & 1);
    }
    printf("\n");
    printf("MDR      : %d\n", removido->estado->MDR);
    printf("A        : %d\n", removido->estado->A);
    printf("B        : %d\n", removido->estado->B);
    printf("ULASaida : %d\n", removido->estado->ULASaida);
    printf("-----------------------------\n");

    return removido;
}

void liberaEstado(Estado *e) {
    if (e) {
        free(e->estado);
        free(e->memoria);
        free(e);
    }
}

void restauraEstado(int *pc, int *bReg, estatInstrucoes *estat,
                    regEstado *reg, MemoriaUnificada *memoria, Estado *snap) {
    *pc = snap->pc;
    memcpy(bReg, snap->bReg, sizeof(int)*8);
    *estat = snap->estat;
    *reg = *snap->estado;
    memcpy(memoria, snap->memoria, sizeof(MemoriaUnificada) * TAM_MEMORIA);
}

void limpaHistorico(Historico *h) {
    while (h->topo) {
        Estado *tmp = h->topo;
        h->topo = tmp->anterior;
        liberaEstado(tmp);
    }
}

//------------------------------------------------Reset----------------------------------------------------

void resetSimulador(MemoriaUnificada *memoria, int *pc, int *bReg, estatInstrucoes *estatInst, regEstado *estado) {
    // Zera PC
    *pc = 0;

    // Zera banco de registradores
    for (int i = 0; i < 8; i++) {
        bReg[i] = 0;
    }

    // Zera memória de dados (mantém instruções)
    for (int i = 128; i < 256; i++) {
        memoria[i].dado = 0;
        memoria[i].memoria = 0;
        strcpy(memoria[i].mem, "0000000000000000");
    }

    // Zera estatísticas
    memset(estatInst, 0, sizeof(estatInstrucoes));

    // Reset estado da UC
    estado->estadoAtual = 0;
    estado->IR = 0;
    estado->MDR = 0;
    estado->A = 0;
    estado->B = 0;
    estado->ULASaida = 0;

}
