#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_HIST 1000

FILE *arquivo, *arquivoMemDados;

// struct das estatísticas
typedef struct{
    int tipoI;
    int tipoJ;
    int tipoR;
    int total;
    int add, sub, and, or, addi, beq, lw, sw, j;
} estatInstrucoes;

// struct de sinais
typedef struct{
    uint8_t branch;
    uint8_t jump;
    uint8_t IncPC;
    uint8_t RegDst;
    uint8_t UlaFonte;
    uint8_t MemParaReg;
    uint8_t EscReg;
    uint8_t EscMem;
    uint8_t ulaOp;
}sinaisUC;

// struct das instruções
enum inst{
    tipoI, tipoJ, tipoR
};

typedef struct {
    char mem[17];
    enum inst tipoInst;
    uint16_t instrucao;
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t funct;
    int8_t imm;
    uint8_t addr;
    int decodificado;
} instrucao;

typedef struct {
    int pc;
    int memDados[256];
    int bReg[8];
    estatInstrucoes estat;
} estado;

typedef struct {
    estado estados[MAX_HIST];
    int topo;
} historico;

/*Instruções:
Tipo R:
opcode: 4 bits (0000);
rs: 3 bits;
rt: 3 bits;
rd: 3 bits;
funct: 3 bits

Tipo I:
opcode: 4 bits;
rs (base/registrador fonte): 3 bits;
rt (destino/registrador fonte): 3 bits;
imediato: 6 bits (estendido);

Tipo J:
opcode: 4 bits;
End: 8 bits;
*/

// ------------------------------ PROTÓTIPOS -------------------------------

// MENU / CONTROLE DO SISTEMA
void imprimeEstatistica(estatInstrucoes estatInst);
void salvaASM(instrucao *memoria, int linhas);
void salvaDAT(int *memDados);
void run(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc, int *memDados, estatInstrucoes *estatInst);
void step(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc, int *memDados, estatInstrucoes *estatInst);

// MEMÓRIA
int contaLinhas(char *arq);
void lerMem(char *arq, instrucao **memoria, int linhas);
void imprimeMemorias(instrucao *memoria, int *memDados);
void imprimeInstrucao(instrucao *memoria, int pc);
void decodifica(instrucao *instrucao);

// PROGRAM COUNTER (PC) / BUSCA
void programCounter(int *pc, sinaisUC *sinais, instrucao *instrucao, int zero);


// DECODIFICAÇÃO
void decodificaInst(instrucao *instrucao);


// UNIDADE DE CONTROLE (UC)
void unidadeControle(instrucao *instrucao, sinaisUC *sinais);


// BANCO DE REGISTRADORES (BREG)
int *inicializaBReg();
void lerRegistradores(int *reg, int8_t rs, int8_t rt, int8_t *valRs, int8_t *valRt);
void escreveRegistrador(int *reg, int8_t rd, int8_t valor, int EscReg);
void imprimeBancoRegistradores(int *reg);


// EXECUÇÃO
int executaInstrucao(instrucao *instrucao, sinaisUC *sinais, int *bReg, int *memDados);
int8_t extensorBit(int8_t imm);


// ULA (UNIDADE LÓGICA E ARITMÉTICA)
int8_t ULA(int op1, int op2, int ulaOp, int *zero, int *overflow);

// MEMÓRIA DE DADOS
int *inicializaMemDados();
void lerMemDados(char *arqMem, int **memDados);
void escreveMemDados(int *memDados, int endereco, int8_t valor);
int8_t retornaMemoria(int *memDados, uint8_t enderecoULA);

// HISTÓRICO
void salvaEstado(historico *hist, int pc, int *memDados, int *bReg, estatInstrucoes *estatInst);
void voltaInstrucao(historico *hist, int *pc, int *memDados, int *bReg, estatInstrucoes *estatInst);

//----------------------------------------------------------------------

int main(){
    int pc = 0, opcao, linhas = 0;

    estatInstrucoes estatInst = {0};

    sinaisUC sinais;

    instrucao *memoria = NULL;

    historico hist;
    hist.topo = 0;

    int *bReg = inicializaBReg();
    int *memDados = inicializaMemDados();

    while (1) {
        printf("\nMenu:\n\n");
        printf("1. Carregar Memória de Instruções (.mem)\n");
        printf("2. Carregar Memória de Dados (.dat)\n");
        printf("3. Imprimir memórias (instruções e dados)\n");
        printf("4. Imprimir Banco de Registradores\n");
        printf("5. Imprimir todo o Simulador\n");
        printf("6. Salvar .asm\n");
        printf("7. Salvar .dat\n");
        printf("8. Executa programa (run)\n");
        printf("9. Executa uma instrução (step)\n");
        printf("10. Volta uma instrução (back)\n");
        printf("0. Sair\n\n");
        printf("Digite uma opção: ");

        scanf("%d", &opcao);
        getchar();

        switch (opcao) {
            case 1:
                //Carregar Mem de Instr.
                char arq[20];
                printf("\nDigite o nome do arquivo da memória de instruções (.mem): ");

                fgets(arq, sizeof(arq), stdin);
                arq[strcspn(arq, "\n")] = '\0';

                linhas = contaLinhas(arq);
                printf("\n%d Instruções carregadas!\n", linhas);

                lerMem(arq, &memoria, linhas);
                break;

            case 2:
                //Carregar Mem de Dados
                char arqMem[20];
                printf("\nDigite o nome do arquivo da memória de dados (.dat): ");

                fgets(arqMem, sizeof(arqMem), stdin);
                arqMem[strcspn(arqMem, "\n")] = '\0';

                lerMemDados(arqMem, &memDados);

                break;

            case 3:
                // Imprimir memórias (tanto instruções quando dados)
                imprimeMemorias(memoria,memDados);

                break;

            case 4:
                //Imprimir Banco de Registradores
                imprimeBancoRegistradores(bReg);
                break;
            case 5:
                //Imprimir simulador
                printf("\nImpressão do simulador:\n");
                do{
                    printf("\n1. Banco de Registradores\n2. Memórias (Dados ou Instruções)\n3. Estatísticas das Instruções)\n");
                    printf("\nSelecione uma das opções acima: ");
                    scanf("%d", &opcao);
                    switch(opcao){
                        case 1:
                            imprimeBancoRegistradores(bReg);
                            break;
                        case 2:
                            imprimeMemorias(memoria,memDados);
                            break;
                        case 3:
                            imprimeEstatistica(estatInst);
                            break;
                        default:
                            printf("Opção inválida! Por favor selecione uma das opções disponíveis.\n");
                            break;
                    }
                }while(opcao<1 || opcao>3);

                break;

            case 6:
                // Salvar .asm
                salvaASM(memoria, linhas);
                break;

            case 7:
                // Salvar .dat
                salvaDAT(memDados);
                break;

            case 8:
                //Executar programa (run)
                run(memoria, bReg, &sinais, &pc, memDados, &estatInst);
                break;

            case 9:
                //Executa instrução (step)
                salvaEstado(&hist, pc, memDados, bReg, &estatInst);
                step(memoria, bReg, &sinais, &pc, memDados, &estatInst);
                break;

            case 10:
                //Voltar instrução (back)
                voltaInstrucao(&hist, &pc, memDados, bReg, &estatInst);
                break;

            case 0:
                //Sair
                free(bReg);
                free(memoria);
                free(memDados);
                printf("\nSaindo do programa...\n");
                return 0;

            default:
                printf("\nOpção inválida!\n");
                break;
        }
    }

    return 0;
}

int contaLinhas(char *arq){
    arquivo = fopen(arq, "r");
    char ch;
    int count=0;

    if(arquivo==NULL){
        printf("\nAcesso negado!\n");
        return 0;
    }

    while((ch=fgetc(arquivo))!=EOF){
        if(ch=='\n'){
            count++;
        }
    }

    fclose(arquivo);
    return count;
}

// Leitura da memória
void lerMem(char *arq, instrucao **memoria, int linhas){
    *memoria = calloc(256, sizeof(instrucao));
    if(memoria == NULL){
    printf("\nMemoria não carregada!\n");
    return;
}
    arquivo = fopen(arq, "r");
    int i=0;
    char mem[17];
    
    if(arquivo==NULL){
        printf("\nPermissão negada!");
        return;
    }

    for(i=0;i<256;i++){
        if(linhas && fscanf(arquivo, "%16s", mem) != EOF){
            strcpy((*memoria)[i].mem, mem);
            (*memoria)[i].instrucao = strtoul(mem, NULL, 2);
        } else {
            strcpy((*memoria)[i].mem, "0000000000000000");
            (*memoria)[i].instrucao = 0;
        }
    }
    
    fclose(arquivo);
}

void imprimeMemorias(instrucao *memoria, int *memDados){
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

                    printf("\t %3d: %16s: ", linha + 128, memoria[linha + 128].mem);
                    imprimeInstrucao(memoria, linha + 128);

                    printf("\t %3d: %16s: ", linha + 192, memoria[linha + 192].mem);
                    imprimeInstrucao(memoria, linha + 192);

                    printf("\n");
                }
                printf("\n");
                break;

            case 2:

                x = 20;

                printf("\n%*sMemória de Dados:\n\n", x, "");
                    
                for (int linha = 0; linha < 64; linha++) {
                    printf("%3d: %3d\t %3d: %3d\t %3d: %3d\t %3d: %3d\n",
                    linha, memDados[linha],
                    linha + 64, memDados[linha + 64],
                    linha + 128, memDados[linha + 128],
                    linha + 192, memDados[linha + 192]);
                }
                printf("\n");
                break;

            default:
                printf("Opção inválida! Por favor, selecione uma das opções disponíveis.\n");
        }
    }while(opt<1 || opt>2);
}

int8_t extensorBit(int8_t imm){
    imm = imm<<2;
    //printf("\n%d", imm);      // 111111 = -1  <- 00111111 << 2 -> 111111100 >> 2 -> 11111111

    imm = imm>>2;
    //printf("\n%d", imm);

    return imm;
}

// RUN 
void run(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc, int *memDados, estatInstrucoes *estatInst){

    while(*pc < 256 && memoria[*pc].instrucao!=0){
        
        printf("\nPC = %d | Memória = %s\n", *pc, memoria[*pc].mem);
       
        // Decodifica
        decodificaInst(&memoria[*pc]);

        memoria[*pc].decodificado = 1;
        // Contabiliza estatística
        switch(memoria[*pc].tipoInst){
            case 0:
                switch(memoria[*pc].opcode){
                    case 4:
                        (*estatInst).addi++;
                        break;
                    case 8:
                        (*estatInst).beq++;
                        break;
                    case 11:
                        (*estatInst).lw++;
                        break;
                    case 15:
                        (*estatInst).sw++;
                        break;
                }
                (*estatInst).tipoI++;
                break;
            case 1:
                (*estatInst).j++;
                (*estatInst).tipoJ++;
                break;
            case 2:
            switch(memoria[*pc].funct){
                case 0:
                        (*estatInst).add++;
                        break;
                    case 2:
                        (*estatInst).sub++;
                        break;
                    case 4:
                        (*estatInst).and++;
                        break;
                    case 5:
                        (*estatInst).or++;
                        break;
                }
                (*estatInst).tipoR++;
                break;
        }

        // Controle
        unidadeControle(&memoria[*pc], sinais);

        // Executa
        int zero = executaInstrucao(&memoria[*pc], sinais, bReg, memDados);

        programCounter(pc, sinais, &memoria[*pc], zero);

        (*estatInst).total++;
    }

    if(*pc>=256 || memoria[*pc].instrucao==0){
        printf("\nFim das instruções!\n");
        if(memoria[*pc].instrucao==0)
            printf("\nMotivo: HALT (instrução 0000000000000000)\n");
        else
            printf("\nMotivo: Uso total da memória.\n");
        return;
    }
}

void step(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc, int *memDados, estatInstrucoes *estatInst){

    if(*pc>=256 || memoria[*pc].instrucao==0){
        printf("\nFim das instruções!\n");
        if(memoria[*pc].instrucao==0)
            printf("\nMotivo: NOP (instrução 0000000000000000)\n");
        else
            printf("\nMotivo: Uso total da memória.\n");
        return;
    }

    printf("\nPC = %d | Memória = %s\n", *pc, memoria[*pc].mem);

    // Decodifica
    decodificaInst(&memoria[*pc]);

    memoria[*pc].decodificado = 1;
    // Contabiliza estatística
    switch(memoria[*pc].tipoInst){
        case 0:
            switch(memoria[*pc].opcode){
                case 4:
                    (*estatInst).addi++;
                    break;
                case 8:
                    (*estatInst).beq++;
                    break;
                case 11:
                    (*estatInst).lw++;
                    break;
                case 15:
                    (*estatInst).sw++;
                    break;
            }
            (*estatInst).tipoI++;
            break;
        case 1:
            (*estatInst).j++;
            (*estatInst).tipoJ++;
            break;
        case 2:
        switch(memoria[*pc].funct){
            case 0:
                    (*estatInst).add++;
                    break;
                case 2:
                    (*estatInst).sub++;
                    break;
                case 4:
                    (*estatInst).and++;
                    break;
                case 5:
                    (*estatInst).or++;
                    break;
            }
            (*estatInst).tipoR++;
            break;
    }
    // Controle
    unidadeControle(&memoria[*pc], sinais);

    // Executa
    int zero = executaInstrucao(&memoria[*pc], sinais, bReg, memDados);

    //PC
    programCounter(pc, sinais, &memoria[*pc], zero);

    (*estatInst).total++;
}

// PC
void programCounter(int *pc, sinaisUC *sinais, instrucao *instrucao, int zero){
    
    // JUMP
    if((*sinais).jump == 1){
        *pc = (*instrucao).addr;
        printf("\nPC atual: %d.\n", *pc);
        return;
    }

    // BRANCH 
    if((*sinais).branch == 1 && zero == 1){
        *pc = *pc + (*instrucao).imm + 1;
        printf("\nPC atual: %d.\n", *pc);
        return;
    }

    // execução normal
    (*pc)++;
    printf("\n[ PC+1 ]\nPC atual: %d.\n", *pc);
}
// Decodificação
void decodificaInst(instrucao *instrucao){

    (*instrucao).opcode = (*instrucao).instrucao >> 12; // Pega os 4 bits do opcode

    switch((*instrucao).opcode){
    case 0:
        (*instrucao).tipoInst = tipoR;
        (*instrucao).rs = ((*instrucao).instrucao >> 9) & 0x7; // pega os 3 bits do rs (desloca 6 bits para a direita e pega os 3 mais significativos que ficaram)
        (*instrucao).rt = ((*instrucao).instrucao >> 6) & 0x7; // pega os 3 bits do rt
        (*instrucao).rd = ((*instrucao).instrucao >> 3) & 0x7; // pega os 3 bits do rd
        (*instrucao).funct = ((*instrucao).instrucao) & 0x7;
        printf("\n[ Tipo R ] \n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("rs: %d\n", (*instrucao).rs);
        printf("rt: %d\n", (*instrucao).rt);
        printf("rd: %d\n", (*instrucao).rd);
        printf("funct: %d\n", (*instrucao).funct);
        break;

    case 2:
        (*instrucao).tipoInst = tipoJ;
        (*instrucao).addr = ((*instrucao).instrucao) &0xFF; // pega os 8 bits do adress
        printf("\n[ Tipo J ]\n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("address: %d\n", (*instrucao).addr);
        break;

    default:
        (*instrucao).tipoInst = tipoI;
        (*instrucao).rs = ((*instrucao).instrucao >> 9) &0x7; // pega os 3 bits do rs
        (*instrucao).rt = ((*instrucao).instrucao >> 6) &0x7; // pega os 3 bits do rt
        (*instrucao).imm = ((*instrucao).instrucao) &0x3F; // pega os 6 bits do imediato (deve passar por um extensor antes da ULA)
        (*instrucao).imm = extensorBit((*instrucao).imm);
        printf("\n[ Tipo I ] \n");
        printf("opcode: %d\n", (*instrucao).opcode);
        printf("rs: %d\n", (*instrucao).rs);
        printf("rt: %d\n", (*instrucao).rt);
        printf("imediato: %d\n", (*instrucao).imm);
    }
}

//UC
void unidadeControle(instrucao *instrucao, sinaisUC *sinais){
    switch((*instrucao).opcode){
        case 0: // opcode = 0000
            (*sinais).RegDst = 1;
            (*sinais).EscReg = 1;
            (*sinais).UlaFonte = 0;
            (*sinais).ulaOp = (*instrucao).funct;
            (*sinais).EscMem = 0;
            (*sinais).MemParaReg = 1;
            (*sinais).jump = 0;
            (*sinais).branch = 0;

            break;

        case 2: // opcode = 0010 - J
            (*sinais).RegDst = 0;
            (*sinais).EscReg = 0;
            (*sinais).UlaFonte = 0;
            (*sinais).ulaOp = 0;
            (*sinais).EscMem = 0;
            (*sinais).MemParaReg = 0;
            (*sinais).jump = 1;
            (*sinais).branch = 0;

            break;

        case 4: // opcode = 0100 - Addi
            (*sinais).RegDst = 0;
            (*sinais).EscReg = 1;
            (*sinais).UlaFonte = 1;
            (*sinais).ulaOp = 0;
            (*sinais).EscMem = 0;
            (*sinais).MemParaReg = 1;
            (*sinais).jump = 0;
            (*sinais).branch = 0;

            break;

        case 8: // opcode = 1000 - BEQ 
            (*sinais).RegDst = 0;
            (*sinais).EscReg = 0;
            (*sinais).UlaFonte = 0;
            (*sinais).ulaOp = 2; // SUB
            (*sinais).EscMem = 0;
            (*sinais).MemParaReg = 0;
            (*sinais).jump = 0;
            (*sinais).branch = 1;

            break;

        case 11: // opcode = 1011 - lw // add
            (*sinais).RegDst = 0;
            (*sinais).EscReg = 1;
            (*sinais).UlaFonte = 1;
            (*sinais).ulaOp = 0; // add
            (*sinais).EscMem = 0;
            (*sinais).MemParaReg = 0;
            (*sinais).jump = 0;
            (*sinais).branch = 0;
            
            break;

        case 15: // opcode = 1111 - sw
            (*sinais).RegDst = 0;
            (*sinais).EscReg = 0;
            (*sinais).UlaFonte = 1;
            (*sinais).ulaOp = 0; // add
            (*sinais).EscMem = 1;
            (*sinais).MemParaReg = 0;
            (*sinais).jump = 0;
            (*sinais).branch = 0;

            break;
    }
}

int *inicializaBReg(){
    return calloc(8, sizeof(int));
}

void lerRegistradores(int *reg, int8_t rs, int8_t rt, int8_t *valRs, int8_t *valRt){
    *valRs = reg[rs];
    *valRt = reg[rt];
}

void escreveRegistrador(int *reg, int8_t rd, int8_t valor, int EscReg){
    if(EscReg){
        reg[rd] = valor;
    }
}

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

int *inicializaMemDados(){
    return calloc(256, sizeof(int));
}

void lerMemDados(char *arqMem, int **memDados) {
    int i=0;

    if (*memDados == NULL) {
        printf("\nErro ao alocar memória\n");
        return;
    }

    arquivoMemDados = fopen(arqMem, "r");
    if (arquivoMemDados == NULL) {
        printf("\nErro ao abrir o arquivo %s\n", arqMem);
        return;
    }

    for(i = 0; i < 256; i++) {
        fscanf(arquivoMemDados, "%d", &(*memDados)[i]);
    }

    printf("\nMemória carregada!\n");

    fclose(arquivoMemDados);
}

void escreveMemDados(int *memDados, int endereco, int8_t valor) {
    if (endereco >= 0 && endereco < 256) {
        memDados[endereco] = valor;
    } else {
        printf("\nErro ao escrever na memória.\n");
    }
}

int8_t retornaMemoria(int *memDados, uint8_t enderecoULA) {
    return memDados[enderecoULA];
}

int executaInstrucao(instrucao* instrucao, sinaisUC *sinais, int *bReg, int *memDados){
    int8_t  operador1, operador2, UlaResultado=0, regDst, dadoFinal=0, valorSW;
    int zero=0, overflow = 0;
    lerRegistradores(bReg, (*instrucao).rs, (*instrucao).rt, &operador1, &operador2);
    
    valorSW = operador2;
    
    if((*sinais).UlaFonte==1){
        operador2=(*instrucao).imm; 
    }

    UlaResultado = ULA(operador1, operador2, (*sinais).ulaOp, &zero, &overflow);

    if((*sinais).EscMem==1){
        escreveMemDados(memDados, (int)UlaResultado, valorSW);
        printf("\nSW: Valor %d guardado no endereço %d\n", valorSW, UlaResultado);
    }

    if((*sinais).MemParaReg==1){
        dadoFinal = UlaResultado;
    }else if((*sinais).EscReg==1){
        dadoFinal = retornaMemoria(memDados, (uint8_t)UlaResultado);
        printf("\nLW: Valor %d lido do endereço %d\n", dadoFinal, UlaResultado);
    }
    
    if((*sinais).RegDst==1){
        regDst = (*instrucao).rd;
    }else{
        regDst = (*instrucao).rt;
    }

    if((*sinais).EscReg==1){
        escreveRegistrador(bReg, regDst, dadoFinal, (*sinais).EscReg);
        printf("\nRegistrador a ser escrito: $%d com o valor %d\n", regDst, dadoFinal);
    }

    if((*sinais).branch==1 && zero == 1){ 
        printf("\nPulo condicional detectado\n");
    }
    return zero;
}

void salvaASM(instrucao *memoria, int linhas) {
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

    while(pc < linhas){
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

void salvaEstado(historico *hist, int pc, int *memDados, int *bReg, estatInstrucoes *estatInst){
    if(hist->topo >= MAX_HIST) return;

    estado *e = &hist->estados[hist->topo];

    e->pc = pc;

    for(int i=0;i<256;i++)
        e->memDados[i] = memDados[i];

    for(int i=0;i<8;i++)
        e->bReg[i] = bReg[i];

    e->estat = *estatInst; // <-- SALVA ESTATÍSTICAS

    hist->topo++;
}

void voltaInstrucao(historico *hist, int *pc, int *memDados, int *bReg, estatInstrucoes *estatInst){
    if(hist->topo <= 0){
        printf("\nSem histórico!\n");
        return;
    }

    hist->topo--;

    estado *e = &hist->estados[hist->topo];

    *pc = e->pc;

    for(int i=0;i<256;i++)
        memDados[i] = e->memDados[i];

    for(int i=0;i<8;i++)
        bReg[i] = e->bReg[i];

    *estatInst = e->estat; // <-- RESTAURA ESTATÍSTICAS

    printf("\nVoltou uma instrução!\n");
    printf("PC atual: %d.\n", *pc);
}

void imprimeInstrucao(instrucao *memoria, int pc) {
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

void decodifica(instrucao *instrucao){

    (*instrucao).opcode = (*instrucao).instrucao >> 12; // Pega os 4 bits do opcode

    switch((*instrucao).opcode){
    case 0:
        (*instrucao).tipoInst = tipoR;
        (*instrucao).rs = ((*instrucao).instrucao >> 9) & 0x7; // pega os 3 bits do rs (desloca 6 bits para a direita e pega os 3 mais significativos que ficaram)
        (*instrucao).rt = ((*instrucao).instrucao >> 6) & 0x7; // pega os 3 bits do rt
        (*instrucao).rd = ((*instrucao).instrucao >> 3) & 0x7; // pega os 3 bits do rd
        (*instrucao).funct = ((*instrucao).instrucao) & 0x7;
        break;

    case 2:
        (*instrucao).tipoInst = tipoJ;
        (*instrucao).addr = ((*instrucao).instrucao) &0xFF; // pega os 8 bits do adress
        break;

    default:
        (*instrucao).tipoInst = tipoI;
        (*instrucao).rs = ((*instrucao).instrucao >> 9) &0x7; // pega os 3 bits do rs
        (*instrucao).rt = ((*instrucao).instrucao >> 6) &0x7; // pega os 3 bits do rt
        (*instrucao).imm = ((*instrucao).instrucao) &0x3F; // pega os 6 bits do imediato (deve passar por um extensor antes da ULA)
        (*instrucao).imm = extensorBit((*instrucao).imm);
    }
}