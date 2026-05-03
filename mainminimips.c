#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "minimips.h"

int main(){

    int opcao, pc = 0, qntdInst = 0;

    estatInstrucoes estatInst = {0};
    
    regEstado regEstado;
    sinaisUC sinais;

    MemoriaUnificada memoria[TAM_MEMORIA] = {0};

    //historico hist;
    //hist.topo = 0;

    int *bReg = inicializaBReg();

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
                //Carregar Memórias
                char arq[20];
                printf("\nDigite o nome do arquivo da memória (.mem): ");

                fgets(arq, sizeof(arq), stdin);
                arq[strcspn(arq, "\n")] = '\0';

                qntdInst = lerMemUnificada(arq, memoria);
                break;

            /*case 2:
                //Carregar Mem de Dados
                char arqMem[20];
                printf("\nDigite o nome do arquivo da memória de dados (.dat): ");

                fgets(arqMem, sizeof(arqMem), stdin);
                arqMem[strcspn(arqMem, "\n")] = '\0';

                lerMemDados(arqMem, memoria, linhas);

                break;
            */
            case 3:
                // Imprimir memórias (tanto instruções quanto dados)
                imprimeMemorias(memoria);

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
                            imprimeMemorias(memoria);
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
                salvaASM(memoria, qntdInst);
                break;

            case 7:
                // Salvar .dat
                // salvaDAT(memDados);
                break;

            case 8:
                // salvaEstado(&hist, pc, bReg, &estatInst, memoria);
                run(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);
                break;

            case 9:
                //Executa instrução (step)
                // salvaEstado(&hist, pc, bReg, &estatInst, memoria);
                step(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);
                break;

            case 10:
                //Voltar instrução (back)
                //voltaInstrucao(&hist, &pc, bReg, &estatInst);
                break;

            case 0:
                //Sair
                free(bReg);
                // free(memoria);
                //free(memDados);
                printf("\nSaindo do programa...\n");
                return 0;

            default:
                printf("\nOpção inválida!\n");
                break;
        }
    }

    return 0;
}
