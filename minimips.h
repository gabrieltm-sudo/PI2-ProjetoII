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

// struct dos registradores de estado (IR, MDR, A, B e ULASaída)
typedef struct{
    // Registradores internos multiciclo
    uint16_t IR;
    uint16_t MDR;
    int8_t A;
    int8_t B;
    int8_t ULASaida;
    int proximoEstado;
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

    // FSM multiciclo
    int estadoEtapa;
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
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t funct;
    int8_t imm;
    uint8_t addr;
    int8_t dado; // valor do dado (se for memória de dados)
    int decodificado;
} MemoriaUnificada;

// struct back
typedef struct {
    int pc;
    uint16_t memDados[128]; //TIRAR MEM DADOS POIS AGORA É UNIFICADO - É mantido pois os dados são algo que depende do estado.
    int bReg[8];
    estatInstrucoes estat;
} estado; //estadoAnterior

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
void run(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado);
void step(MemoriaUnificada *memoria, int *bReg, sinaisUC *sinais, int *pc, estatInstrucoes *estatInst, regEstado *estado);
void imprimeEstatistica(estatInstrucoes estatInst);
void salvaASM(MemoriaUnificada *memoria, int qntdInst, regEstado *estado,int *bReg);
void salvaDAT(int *memDados);

// MEMÓRIA
int lerMemUnificada(char *arq, MemoriaUnificada *memUnificada);
void escreveMemDados(MemoriaUnificada *memUnificada, int endereco, int8_t valor);
int8_t retornaMemoria(int *memDados, uint8_t enderecoULA);
void acessoMemoria(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado, MemoriaUnificada *memoria);

// void contaLinhas(char *arq, int *qtInst, int *qtDados);
//      Essas duas provavelmente serão apagadas pois não são mais utilizadas
// void lerMemDados(char *arq, MemoriaUnificada *memUnificada, int linhas);
void imprimeMemorias(MemoriaUnificada *memoria, regEstado *estado, int *bReg);
void imprimeInstrucao(MemoriaUnificada *memoria, int pc, regEstado *estado,int *bReg);

// PROGRAM COUNTER (PC) / BUSCA
void buscaInstrucao(MemoriaUnificada *memoria, int *pc, regEstado *estado);
void programCounter(int *pc, sinaisUC *sinais, MemoriaUnificada *instrucao, int zero, regEstado *estado);
// DECODIFICAÇÃO
void decodificaInstrucao(int pc, regEstado *estado, int *bReg);   // multiciclo

// UNIDADE DE CONTROLE (UC)
void unidadeControleMulti(uint8_t opcode, uint8_t funct, regEstado *estado, sinaisUC *sinais);
int defineEstado(int estadoAtual, uint8_t opcode);

// BANCO DE REGISTRADORES (BREG)
int *inicializaBReg();
void lerRegistradores(int *reg, int8_t rs, int8_t rt, int8_t *valRs, int8_t *valRt);
void escreveRegistrador(int *reg, int8_t rd, int8_t valor, int EscReg);
void imprimeBancoRegistradores(int *reg);

// EXECUÇÃO
void executaCiclo(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado, int *zero, int *pc);
int8_t extensorBit(int8_t imm);
void writeBack(MemoriaUnificada *instrucao, sinaisUC *sinais, int *bReg, regEstado *estado);
// ULA (UNIDADE LÓGICA E ARITMÉTICA)
int8_t ULA(int op1, int op2, int ulaOp, int *zero, int *overflow);

// HISTÓRICO
void salvaEstado(historico *hist, int pc, int *bReg, estatInstrucoes *estatInst, MemoriaUnificada *mem);
void voltaInstrucao(historico *hist, int *pc, int *bReg, estatInstrucoes *estatInst);

// -------------------------------------------------------------------------

#endif
