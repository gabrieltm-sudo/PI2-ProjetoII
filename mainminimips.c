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
    Historico hist;

    int *bReg = inicializaBReg();
    inicializaHistorico(&hist);

    salvaEstado(&hist, pc, bReg, &estatInst, &regEstado);

    while (1) {
        printf("\nMenu:\n\n");
        printf("1. Carregar Memória de Instruções (.mem)\n");
        printf("2. Imprimir memórias (instruções e dados)\n");
        printf("3. Imprimir Banco de Registradores\n");
        printf("4. Imprimir todo o Simulador\n");
        printf("5. Salvar .asm\n");
        printf("6. Salvar .mem\n");
        printf("7. Executa programa (run)\n");
        printf("8. Executa uma instrução (step)\n");
        printf("9. Volta uma instrução (back)\n");
        printf("10. Reset\n");
        printf("0. Sair\n\n");
        printf("Digite uma opção: ");

        scanf("%d", &opcao);
        getchar();

        switch (opcao) {
            case 1:
                //Carregar Memórias
                char arq[50];
                printf("\nDigite o nome do arquivo da memória (.mem): ");

                fgets(arq, sizeof(arq), stdin);
                arq[strcspn(arq, "\n")] = '\0';

                qntdInst = lerMemUnificada(arq, memoria);
                break;

            case 2:
                // Imprimir memórias (tanto instruções quanto dados)
                imprimeMemorias(memoria, bReg);

                break;

            case 3:
                //Imprimir Banco de Registradores
                imprimeBancoRegistradores(bReg);
                break;
            case 4:
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
                            imprimeMemorias(memoria, bReg);
                            break;
                        case 3:
                            imprimeEstatistica(estatInst);
                            break;
                        default:
                            printf("\nOpção inválida! Por favor selecione uma das opções disponíveis.\n");
                            break;
                    }
                }while(opcao<1 || opcao>3);

                break;

            case 5:
                // Salvar .asm
                salvaASM(memoria, qntdInst, &regEstado, bReg);
                break;

            case 6:
                salvaMem(memoria, qntdInst);
                break;

            case 7:
                // salvaEstado(&hist, pc, bReg, &estatInst, memoria);
                run(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);
                break;

            case 8: // Executa uma instrução (step)
                step(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);
                salvaEstado(&hist, pc, bReg, &estatInst, &regEstado);
                break; // Adicione este break

            case 9: // Volta uma instrução (back)
                voltaInstrucao(&hist, &pc, bReg, &estatInst, &regEstado, memoria);
                break; // Adicione este break

            case 10: // Reset
                resetSimulador(memoria, &pc, bReg, &estatInst, &regEstado);
                inicializaHistorico(&hist); // Limpa o histórico no reset
                salvaEstado(&hist, pc, bReg, &estatInst, &regEstado); // Salva estado inicial
                break;

            case 0:
                // Sair
                free(bReg);
                printf("\nSaindo do programa...\n");
                return 0;

            default:
                printf("\nOpção inválida!\n");
                break;
        }
    }

    return 0;
}
