help:
	@echo "Funcionalidades: "
	@echo "Digite make compile para compilar o arquivo."
	@echo "Digite make run para executar o arquivo compilador."
	@echo "Digite make compilerun para compilar e executar o arquivo de uma vez."
	@echo "Digite make rm para excluir o arquivo gerado."
compile:
	gcc -Wall -Wextra -g mainminimips.c -o exec

run:
	./exec

compilerun:
	gcc -Wall -Wextra -g mainminimips.c -o exec
	./exec

rm:
	rm -f exec
