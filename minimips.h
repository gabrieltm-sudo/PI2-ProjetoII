#include <stdint.h>

#ifndef MINIMIPS_H
#define MINIMIPS_H

#define MAX_HIST 1000
#define TAM_MEMORIA 256
#define INI_INST 0
#define FIM_INST 127
#define INI_DADOS 128
#define FIM_DADOS 255

extern FILE *arquivo;
extern FILE *arquivoMemDados;

// enum das instruções
enum inst{
    tipoI, tipoJ, tipoR, tipoDado
};

// struct dos registradores de estado (IR, MDR, A, B e ULASaída) e campos das instruções
typedef struct{
    // Registradores internos multiciclo
    uint16_t IR;
    uint16_t MDR;
    int8_t A;
    int8_t B;
    int ULASaida;
    int estadoAtual;

    // Campos decodificados
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t funct;
    int8_t imm;
    uint8_t addr;
    enum inst tipoInst;
} regEstado;

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
    uint8_t RegDst;
    uint8_t UlaFonteA;
    uint8_t UlaFonteB;
    uint8_t MemParaReg;
    uint8_t EscReg;
    uint8_t EscMem;
    uint8_t LerMem;
    uint8_t ControleUla;
    uint8_t IouD;
    uint8_t IREsc;
    uint8_t PCEsc;
    // uint8_t PCWriteCond;
    uint8_t PCFonte;
}sinaisUC;

typedef struct {
    char mem[17];
    enum inst tipoInst;
    uint16_t memoria;
    int8_t dado; // valor do dado (se for memória de dados)
} MemoriaUnificada;

//step back
typedef struct Estado {
    int pc;
    int bReg[8];
    estatInstrucoes estat;
    int estadoAtual;
    regEstado *estado;
    MemoriaUnificada *memoria;
    // --------------------------------------
    struct Estado *anterior;
} Estado;

typedef struct {
    Estado *topo;     // posição atual
} Historico;

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
void run(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado);
void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado);
void imprimeEstatistica(estatInstrucoes estatInst);
void salvaASM(MemoriaUnificada *memoria, int qntdInst, regEstado *estado,int *bReg);
void salvaMem(MemoriaUnificada *memoria, int qntdInst);

// MEMÓRIA
int lerMemUnificada(char *arq, MemoriaUnificada *memUnificada);
void escreveMemDados(MemoriaUnificada *memUnificada, int endereco, int8_t valor);
void acessoMemoria(regEstado *estado, MemoriaUnificada *memoria);
void imprimeMemorias(MemoriaUnificada *memoria, int *bReg);
void imprimeInstrucao(MemoriaUnificada *memoria, int pc, regEstado *estado,int *bReg);

// PROGRAM COUNTER (PC) / BUSCA
void buscaInstrucao(MemoriaUnificada *memoria, int *pc, regEstado *estado);

// DECODIFICAÇÃO
void decodificaInstrucao(int pc, regEstado *estado, int *bReg);

// UNIDADE DE CONTROLE (UC)
void unidadeControleMulti(regEstado *estado, sinaisUC *sinais);
void defineEstado(int *estadoAtual, uint8_t opcode);

// BANCO DE REGISTRADORES (BREG)
int *inicializaBReg();
void imprimeBancoRegistradores(int *reg);

// EXECUÇÃO
void executaCiclo(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado, int *zero, int *pc);
int8_t extensorBit(int8_t imm);

// ULA (UNIDADE LÓGICA E ARITMÉTICA)
int ULA(int op1, int op2, int ulaOp, int *zero, int *overflow, regEstado *estado);

// HISTÓRICO
void salvaEstado(Historico *h, int pc, int *bReg, estatInstrucoes estat, regEstado *reg, MemoriaUnificada *memoria);
Estado* voltaEstado(Historico *h);
void restauraEstado(int *pc, int *bReg, estatInstrucoes *estat, regEstado *reg, MemoriaUnificada *memoria, Estado *snap);
void liberaEstado(Estado *e);
void limpaHistorico(Historico *h);
// -------------------------------------------------------------------------

//RESET DO SIMULADOR
void resetSimulador(MemoriaUnificada *memoria, int *pc, int *bReg, estatInstrucoes *estatInst, regEstado *estado);
// -------------------------------------------------------------------------

#endif
