# Simulador MiniMIPS (8-bit, multiciclo)

Simulador acadêmico de uma arquitetura MiniMIPS de 8 bits, organizado em multiciclo. Implementado em C com interface por menu, suporta operações de carga/armazenamento, instruções aritméticas e de controle de fluxo.

**Autores**

- Alex Maron Vaz — https://github.com/AlexMaronVaz
- Adrian do Couto Martins — https://github.com/AdrianCouto
- Gabriel Torres Machado — https://github.com/gabrieltm-sudo

**Principais funcionalidades**

- Simulação de execução multiciclo de instruções MiniMIPS
- Suporte a leitura/escrita de memória (arquivo `memoria.mem`)
- Interface interativa por menu para carregar e executar programas

**Estrutura do repositório**

- `mainminimips.c` — ponto de entrada / interface do simulador
- `funcoes.c` — implementação das funções do simulador
- `minimips.h` — definições e protótipos
- `memoria.mem` — arquivo de memória com instruções e dados
- `Arquivos Importantes/instMiniMips.txt` — instruções e formato suportado
- `makefile` — regras de compilação e execução

**Requisitos**

- GCC
- Make

**Como compilar e executar**

Use as flags do `makefile` a partir do diretório do projeto:

```bash
make compile      # compila os binários
make run          # executa o simulador (./exec)
make compilerun   # compila e executa em sequência
make rm           # remove arquivos gerados
```

**Observações**

- Projeto para fins educacionais (disciplina Projeto Integrador II).
- Consulte `Arquivos Importantes/instMiniMips.txt` para o formato das instruções a serem carregadas na memória.