help:
	@echo "Funcionalidades: "
	@echo "Digite make compile para compilar o arquivo."
	@echo "Digite make run para executar o arquivo compilador."
	@echo "Digite make compilerun para compilar e executar o arquivo de uma vez."
	@echo "Digite make rm para excluir os arquivos gerados."
compile:
	gcc -c -g funcoes.c -o funcoes
	gcc -Wall -Wextra -g mainminimips.c funcoes -o exec

run:
	./exec

compilerun:
	gcc -c -g funcoes.c -o funcoes
	gcc -Wall -Wextra -g mainminimips.c funcoes -o exec
	./exec

rm:
	rm -f funcoes
	rm -f exec
	rm -f *.asm
