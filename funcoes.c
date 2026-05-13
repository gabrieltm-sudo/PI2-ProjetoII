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
        return 1;
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
        memUnificada[j].decodificado = 0;
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


void acessoMemoria(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado, MemoriaUnificada *memoria) {
    if (instrucao->opcode == 11) { // LW
        estado->MDR = memoria[estado->ULASaida].dado;
    } else if (instrucao->opcode == 15) { // SW
        memoria[estado->ULASaida].dado = estado->B;
    }
}


//----------------------------------------------BUSCA (IF)-----------------------------------------------------

void buscaInstrucao(MemoriaUnificada *memoria, int *pc, regEstado *estado) {
    // Carrega instrução no IR
    estado->IR = memoria[*pc].memoria;

    printf("\n==========================================\n");
    printf("Busca\n");
    printf("==========================================\n");
    printf("IR: %s\n", memoria[*pc].mem);
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
            estado->ULASaida = pc + estado->imm + 1;
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

void unidadeControleMulti(uint8_t opcode, uint8_t funct, regEstado *estado, sinaisUC *sinais) {
    // Zera sinais
    *sinais = (sinaisUC){0};

// Devo ver se realmente preciso dar igual zero nos sinais zerados pois eles são zerados acima - Gabriel
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
            sinais->RegDst = 1; // ? Acredito ser don't care pois o regEsc é zero nesse momento

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

int8_t ULA(int op1, int op2, int ControleUla, int *zero, int *overflow){
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

    while (*pc < 256 && memoria[*pc].memoria != 0) {
        step(memoria, bReg, sinais, pc, estatInst, estado);
    }

    printf("\n==========================================\n");
    printf("Fim das instruções\n");
    printf("==========================================\n");
}

void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc,
    estatInstrucoes *estatInst, regEstado *estado) {
    int zero = 0;
    
    if (*pc >= 256 || memoria[*pc].memoria == 0) {
        printf("\n==========================================\n");
        printf("Fim das instruções\n");
        printf("==========================================\n");
        return;
    }
    
    printf("\n[Estado atual] %d\n", estado->estadoAtual);
    printf("[PC] %d\n", *pc);
    
    uint8_t opcodeAtual = (estado->IR >> 12) & 0xF;
    uint8_t functAtual = estado->IR & 0x7;
    // Controle e execução do ciclo
    unidadeControleMulti(opcodeAtual, functAtual, estado, sinais);
    executaCiclo(memoria, sinais, bReg, estado, &zero, pc);

    // Imprime usando a assinatura correta
    if(estado->estadoAtual==1){
        imprimeInstrucao(memoria, *pc - 1, estado, bReg);
        printf("\n");
    }

    memoria[*pc - 1].decodificado = 1;

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

    defineEstado(&estado->estadoAtual, opcodeAtual);
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

    switch(estado->estadoAtual){
        case 0: // Busca
            printf("\n[IF] Busca\n");
            buscaInstrucao(memoria, pc, estado);
            operacaoULA = ULAcontrole(sinais->ControleUla, estado->funct);
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow);
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
                    printf("Tipo R | opcode = %d rs = %d rt = %d rd = %d funct = %d\n", estado->opcode, estado->rs, estado->rt, estado->rd, estado->funct);
                    break;

                case 2: // Tipo J
                    printf("Tipo J | opcode = %d addr = %d\n", estado->opcode, estado->addr);
                    break;

                default: // Tipo I
                    printf("Tipo I | opcode = %d rs = %d rt = %d imm = %d branch = %d\n", estado->opcode, estado->rs, estado->rt, estado->imm, estado->ULASaida);
                    break;
            }
            break;
        case 2: // Execução tipo I
            printf("\n[EX] Tipo I\n");
            operacaoULA = ULAcontrole(sinais->ControleUla, estado->funct);
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow);
            printf("[ULA] Resultado = %d\n", estado->ULASaida);
            break;
        case 3: // LW - leitura memória
            printf("\n[MEM] LW\n");
            acessoMemoria(&memoria[*pc - 1], sinais, bReg, estado, memoria);
            printf("[MEMD] mem[%d] -> MDR = %d\n", estado->ULASaida, estado->MDR);
            break;
        case 4: // LW - write back
            printf("\n[WB] LW\n");
            if(sinais->EscReg){
                bReg[estado->rt] = estado->MDR;   // Reg[rt] ← MDR
                printf("[BREG] $%d = %d\n", estado->rt, estado->MDR);
            }
            break;

        case 5: // SW - Write Memory
            printf("\n[MEM] SW\n");
            if(sinais->EscMem){
                memoria[estado->ULASaida].dado = estado->B;  // Mem[ULASaida] ← B
                printf("[MEMD] mem[%d] = %d\n", estado->ULASaida, estado->B);
            }
            break;

        case 6: // ADDI - Finalização
            printf("\n[WB] ADDI\n");
            if(sinais->EscReg){
                bReg[estado->rt] = estado->ULASaida;  // Reg[rt] ← resultado da ULA
                printf("[BREG] $%d = %d\n", estado->rt, estado->ULASaida);
            }
            break;

        case 7: // Execução tipo R
            printf("\n[EX] Tipo R\n");
            operacaoULA = ULAcontrole(sinais->ControleUla, estado->funct);
            estado->ULASaida = ULA(op1, op2, operacaoULA, zero, &overflow);
            printf("[ULA] Resultado = %d\n", estado->ULASaida);
            break;

        case 8: // Tipo R - Write Back
            printf("\n[WB] Tipo R\n");
            if(sinais->EscReg){
                bReg[estado->rd] = estado->ULASaida;  // Reg[rd] ← resultado da ULA
                printf("[BREG] $%d = %d\n", estado->rd, estado->ULASaida);
            }
            break;

        case 9: // BEQ
            int resultado;
            printf("\n[EX] BEQ\n");
            operacaoULA = ULAcontrole(sinais->ControleUla, estado->funct);
            resultado = ULA(op1, op2, operacaoULA, zero, &overflow);
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
                printf("add $%d, $%d, $%d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==2)
                printf("sub $%d, $%d, $%d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==4)
                printf("and $%d, $%d, $%d", estado->rd, estado->rs, estado->rt);
            else if(estado->funct==5)
                printf("or $%d, $%d, $%d", estado->rd, estado->rs, estado->rt);
            break;

        case 2: // Jump
            printf("j %d%*s", estado->addr, x, "");
            break;

        case 4: // Addi
            printf("addi $%d, $%d, %d", estado->rt, estado->rs, estado->imm);
            break;

        case 8: // BEQ
            printf("beq $%d, $%d, %d", estado->rs, estado->rt, estado->imm);
            break;

        case 11: // LW
            printf("lw $%d, %d($%d)", estado->rt, estado->imm, estado->rs);
            break;

        case 15: // SW
            printf("sw $%d, %d($%d)", estado->rt, estado->imm, estado->rs);
            break;
    }
}

void imprimeMemorias(MemoriaUnificada *memoria, regEstado *estado, int *bReg){
    int opt, x;
    do{
        printf("\n==========================================\n");
        printf("Impressão de memórias\n");
        printf("==========================================\n");
        printf("1. Memória de instruções\n");
        printf("2. Memória de dados\n");
        printf("Opção: ");
        scanf("%d", &opt);

        switch(opt){
            case 1:
	            x = 30;

                printf("\n%*sMemória de instruções\n\n", x, "");

                for (int linha = 0; linha < 64; linha++) {
                    //1ª coluna
                    estado->IR = memoria[linha].memoria;
                    decodificaInstrucao(linha, estado, bReg);

                    printf(" %3d | %16s | ", linha, memoria[linha].mem);
                    imprimeInstrucao(memoria, linha, estado, bReg);

                    //2ª coluna
                    estado->IR = memoria[linha+64].memoria;
                    decodificaInstrucao(linha+64, estado, bReg);

                    printf("\t %3d | %16s | ", linha+64, memoria[linha+64].mem);
                    imprimeInstrucao(memoria, linha+64, estado, bReg);

                    printf("\n");
                }
                break;

            case 2:

                x = 20;

                printf("\n%*sMemória de dados\n\n", x, "");

                for (int linha = 0; linha < 32; linha++) {
                    printf("%3d | %3d\t %3d | %3d\t %3d | %3d\t %3d | %3d\n",
                    128 + linha, memoria[128 + linha].dado,
                    128 + linha + 32, memoria[128 + linha + 32].dado,
                    128 + linha + 64, memoria[128 + linha + 64].dado,
                    128 + linha + 96, memoria[128 + linha + 96].dado);
                }
                printf("\n");
                break;

            default:
                printf("[ERRO] Opção inválida.\n");
        }
    }while(opt<1 || opt>2);
}

//-----------------------------------------------Salvamentos---------------------------------------------------

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

void inicializaHistorico(Historico *hist) {
    hist->primeiro = NULL;
    hist->ultimo = NULL;
    hist->atual = NULL;
}

void salvaEstado(Historico *hist, int pc, int *bReg, estatInstrucoes *estatInst, regEstado *reg) {
    Estado *novo = malloc(sizeof(Estado));
    novo->pc = pc;

    for(int i=0;i<8;i++) {
        novo->bReg[i] = bReg[i];
    }

    novo->estat = *estatInst;
    novo->estadoAtual = reg->estadoAtual;
    novo->anterior = hist->ultimo;
    novo->proximo = NULL;
    novo->IR = reg->IR;

    if(hist->ultimo) {
        hist->ultimo->proximo = novo;
    } else {
        hist->primeiro = novo;
    }

    hist->ultimo = novo;
    hist->atual = novo;
}

void voltaInstrucao(Historico *hist, int *pc, int *bReg, estatInstrucoes *estatInst, regEstado *reg) {
    if(hist->atual && hist->atual->anterior) {
        hist->atual = hist->atual->anterior;
        *pc = hist->atual->pc;

        for(int i=0;i<8;i++) {
            bReg[i] = hist->atual->bReg[i];
        }

        *estatInst = hist->atual->estat;
        reg->estadoAtual = hist->atual->estadoAtual;
        reg->IR = hist->atual->IR;

        if (reg->estadoAtual == 1) {
            decodificaInstrucao(*pc, reg, bReg);
        }

        printf("\n[BACK] PC=%d | estado=%d\n", *pc, reg->estadoAtual);
    } else {
        printf("\n[INFO] Não há instrução anterior.\n");
    }
}

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