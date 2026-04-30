#include <stdint.h>

#ifndef MINIMIPS_H
#define MINIMIPS_H

#define MAX_HIST 1000

extern FILE *arquivo;
extern FILE *arquivoMemDados;

// struct dos registradores de estado (IR, MDR, A, B e ULASaída)
typedef struct{
    uint16_t IR;
    uint16_t MDR; // Os 8 bits mais significativos (15-8) dos elementos da memória de dados serão preenchidos com 0s.
    int8_t A;
    int8_t B;
    int8_t ULASaida;
}regEstado;

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

// struct back
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
void run(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc,
         int *memDados, estatInstrucoes *estatInst, regEstado *estado);
void step(instrucao *memoria, int *bReg, sinaisUC *sinais, int *pc,
          int *memDados, estatInstrucoes *estatInst, regEstado *estado);
void imprimeEstatistica(estatInstrucoes estatInst);
void salvaASM(instrucao *memoria, int linhas);
void salvaDAT(int *memDados);

// MEMÓRIA DE INSTRUÇÕES
int contaLinhas(char *arq);
void lerMem(char *arq, instrucao **memoria, int linhas);
void imprimeMemorias(instrucao *memoria, int *memDados);
void imprimeInstrucao(instrucao *memoria, int pc);

// PROGRAM COUNTER (PC) / BUSCA
void buscaInstrucao(instrucao *memoria, int *pc, regEstado *estado);
void programCounter(int *pc, sinaisUC *sinais, instrucao *instrucao, int zero);

// DECODIFICAÇÃO
void decodificaInstrucao(regEstado *estado, int *bReg);   // multiciclo
void decodificaInst(instrucao *instrucao);                // utilitário (ASM)

// UNIDADE DE CONTROLE (UC)
void unidadeControleMulti(uint8_t opcode, uint8_t funct, int ciclo, sinaisUC *sinais);

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

// -------------------------------------------------------------------------

#endif
