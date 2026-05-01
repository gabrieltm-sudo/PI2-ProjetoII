#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "minimips.h"

int main(){
    regEstado estado;   // novo registrador de estado para multiciclo

    int pc = 0, opcao, linhas = 0;

    estatInstrucoes estatInst = {0};

    sinaisUC sinais;

    MemoriaUnificada *memoria = calloc(256, sizeof(MemoriaUnificada));

    historico hist;
    hist.topo = 0;

    int *bReg = inicializaBReg();
    int *memDados = inicializaMemDados();  //APAGAR MEMORIA DE DADOS

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

                lerMem(arq, memoria, linhas);
                break;

            case 2:
                //Carregar Mem de Dados
                char arqMem[20];
                printf("\nDigite o nome do arquivo da memória de dados (.dat): ");

                fgets(arqMem, sizeof(arqMem), stdin);
                arqMem[strcspn(arqMem, "\n")] = '\0';

                lerMemDados(arqMem, memoria, linhas);

                break;

            case 3:
                // Imprimir memórias (tanto instruções quando dados)
                imprimeMemorias(memoria, memDados);

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
                run(memoria, bReg, &sinais, &pc, memDados, &estatInst, &estado);
                break;

            case 9:
                //Executa instrução (step)
                salvaEstado(&hist, pc, memDados, bReg, &estatInst);
                step(memoria, bReg, &sinais, &pc, memDados, &estatInst, &estado);
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
