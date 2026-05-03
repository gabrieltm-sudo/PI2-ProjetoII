#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "minimips.h"

FILE *arquivo = NULL;
FILE *arquivoMemDados = NULL;

//---------------------------------------LEITURA E INICIALIZAÇÃO------------------------------------------------

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


//------------------------------------------Decodificação-------------------------------------------------

// Isso não acontece nesse momento. As etapas envolviam: 
// Busca da instrução - Armazenar no IR a instrução que o PC está apontando e fazer PC+1 - OK.
// Decodificação & leitura dos registradores RS e Rt & cálculo do endereço de desvio e Armazena em A e B Rs e Rt, respectivamente, armazena no UlaSaida o cálculo de PC + imediato

/* void programCounter(int *pc, sinaisUC *sinais, MemoriaUnificada *instrucao, int zero, regEstado *estado) { // MUDAR NOME - ProgramCounter não faz sentido.
    if (instrucao->opcode == 8 && zero) { // BEQ
        *pc = estado->ULASaida; 
    } else if (instrucao->opcode == 2) { // JUMP
        *pc = estado->IR & 0xFF; // 8 bits menos significativos
    }
}*/

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

    // chamar a ULA ao invés de fazer cálculo direto - Outra coisa, por favor fazer isso em uma função que não seja a decodificação da instrução para podermos utilizar apenas para isso a função, se possível
    estado->ULASaida = (estado->estadoAtual) + imm_signed;
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
            sinais->ControleUla = 3; // 011 - Faz operação de acordo com funct da instrução
            
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
            sinais->ControleUla = 2; // 010 - ULA faz subtração

            break;
        case 10: // 10º Estado - Jump
            sinais->PCEsc = 1;
            sinais->UlaFonteA = 0;
            sinais->PCFonte = 2; // 10 - Imediato vai para o PC
            
        break;
    }

    estado->proximoEstado = defineEstado(estado->estadoAtual, opcode);
}

int defineEstado(int estadoAtual, uint8_t opcode){

    switch(estadoAtual){
        case 0:
            return 1;
        case 1:
            switch(opcode){
                case 0: // Tipo R
                    return 7;
                case 2: // Jump
                    return 10;
                case 8: // BEQ
                    return 9;
                default: // Tipo I (addi, sw e lw)
                    return 2;
                    }
        case 2:
            switch(opcode){
                case 4: // addi
                    return 6;
                case 11: // lw
                    return 3;
                case 15: // sw
                    return 5;
                    }
        case 3:
            return 4;
        case 7:
            return 8;

        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            return 0;
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

// Não implementada ainda. Se quiser, pode apagar
/* void writeBack(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado) {
    if (instrucao->opcode == 0) { // Tipo R
        bReg[instrucao->rd] = estado->ULASaida;
    } else if (instrucao->opcode == 4) { // addi
        bReg[instrucao->rt] = estado->ULASaida;
    } else if (instrucao->opcode == 11) { // LW
        bReg[instrucao->rt] = estado->MDR;
    }
}*/

//-------------------------------------------Controle de fluxo-------------------------------------------------

void run(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado) {

    while (*pc < 256 && memoria[*pc].memoria != 0) {
        step(memoria, bReg, sinais, pc, estatInst, estado);
    }

    printf("\nFim das instruções!\n");
}

void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado) {
    int zero = 0;

    if (*pc >= 256 || memoria[*pc].memoria == 0) {
        printf("\nFim das instruções!\n");
        return;
    }
    
    printf("\n[ Estado atual: %d ]\n", estado->estadoAtual);
    printf("\nPC = %d\n", *pc);
    imprimeInstrucao(memoria, *pc);


        
    unidadeControleMulti(memoria[*pc].opcode, memoria[*pc].funct, estado, sinais);
    executaCiclo(memoria, sinais, bReg, estado, &zero, pc);

    memoria[*pc].decodificado = 1;
    
    // Contabiliza estatísticas
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
    estado->estadoAtual = estado->proximoEstado;
}

void executaCiclo(MemoriaUnificada *memoria, sinaisUC *sinais, int *bReg, regEstado *estado, int *zero, int *pc) {
    int op1, op2, novoPc, overflow;
    
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
        op2 = memoria[*pc].imm;
    
    if(sinais->PCFonte == 0)
        novoPc = estado->ULASaida;
    else if(sinais->PCFonte == 1)
        novoPc = memoria[*pc].imm;
        
    // Se estado x, faz...
    switch(estado->estadoAtual){
        case 0: // Busca
            buscaInstrucao(memoria, pc, estado);
        
            break;
        case 1: // Decodificação (Não apenas da instrução)
            decodificaInstrucao(estado, bReg, memoria);
            break;
        case 2: // Execução tipo I
            estado->ULASaida = ULA(op1, op2, sinais->ControleUla, zero, &overflow); // sinais->ControleUla são os sinais que enviará para o controle da Ula.

            break;
        case 3: // Execução LW - parte da finalização do SW, addi e tipo R

            break;
        case 4: // Finalização LW

            break;
        case 5: // Finalização SW
            
            break;
        case 6: // Finalização SW

            break;
        case 7: // Execução tipo R
            estado->ULASaida = ULA(op1, op2, sinais->ControleUla, zero, &overflow);
        
            break;
        case 8: // Finalização tipo R

            break;
        case 9: // BEQ

            if(sinais->branch == 1 && *zero == 1){
                *pc = novoPc;
            }
        
            break;
        case 10:
            if(sinais->PCEsc==1){
                *pc = novoPc;
            }
            break;
    }

    printf("\n[ Próximo estado: %d ]\n", estado->proximoEstado);
}

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
