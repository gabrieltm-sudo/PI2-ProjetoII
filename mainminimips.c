#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "minimips.h"

int main(){
    int opcao, pc = 0, qntdInst = 0, verificaMem = 0;
    estatInstrucoes estatInst = {0};
    regEstado regEstado;
    sinaisUC sinais;
    MemoriaUnificada memoria[TAM_MEMORIA] = {0};

    Historico *hist = malloc(sizeof(Historico));
    hist->topo = NULL;
    int *bReg = inicializaBReg();

    salvaEstado(hist, pc, bReg, estatInst, &regEstado, memoria);

    while (1) {
        printf("\nMenu:\n\n");
        printf("1. Carregar Memória de Instruções (.mem)\n");
        printf("2. Imprimir memórias (instruções e dados)\n");
        printf("3. Imprimir Banco de Registradores\n");
        printf("4. Imprimir todo o Simulador\n");
        printf("5. Salvar .asm\n");
        printf("6. Salvar .mem\n");
        printf("7. Executa programa (run)\n");
        printf("8. Executa um ciclo (step)\n");
        printf("9. Volta um ciclo (back)\n");
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

                verificaMem = lerMemUnificada(arq, memoria);
                if(verificaMem!=0){
                    pc = 0;
                    memset(&regEstado, 0, sizeof(regEstado));
                    memset(bReg, 0, sizeof(int)*8);
                    memset(&estatInst, 0, sizeof(estatInst));
                    regEstado.estadoAtual = 0;

                    limpaHistorico(hist);
                    salvaEstado(hist, pc, bReg, estatInst, &regEstado, memoria);

                    qntdInst = verificaMem;
                    verificaMem = 1;
                }
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
                if(verificaMem)
                    run(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);
                else
                    printf("\nCarregue um .mem primeiro.\n");
                break;

            case 8: // Step
                if (verificaMem) {
                        salvaEstado(hist, pc, bReg, estatInst, &regEstado, memoria);
                        step(memoria, bReg, &sinais, &pc, &estatInst, &regEstado);

                } else {
                    printf("\nCarregue um .mem primeiro.\n");
                }
                break;

            case 9: { // Back
                if (verificaMem) {
                    Estado *snap = voltaEstado(hist);
                    if (snap) {
                        restauraEstado(&pc, bReg, &estatInst, &regEstado, memoria, snap);
                        liberaEstado(snap);
                    }
                } else {
                    printf("\nCarregue um .mem primeiro.\n");
                }
                break;
            }

            case 10: // Reset
                resetSimulador(memoria, &pc, bReg, &estatInst, &regEstado);
                limpaHistorico(hist);
                pc = 0;
                salvaEstado(hist, pc, bReg, estatInst, &regEstado, memoria);
                printf("\nReset feito com sucesso!\n");
                break;

            case 0:
                // Sair
                limpaHistorico(hist);
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
