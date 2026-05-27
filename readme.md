# Simulador de SO em C++ com Qt

## Visão Geral

Este projeto é uma aplicação em C++ com uma interface gráfica de usuário construída usando Qt e QML. Possui um motor de simulação (`simulador.hpp`) e depende de entradas de dados externos via arquivos CSV.

## Estrutura do Projeto

- `main.cpp`: O ponto de entrada da aplicação C++. Geralmente inicializa o motor da aplicação Qt, registra classes de backend do C++ no QML e carrega a interface frontend.
- `Main.qml`: O arquivo QML principal que define a interface gráfica do usuário.
- `simulador.hpp`: O arquivo de cabeçalho (header) que contém as definições e a lógica para o módulo de simulação.
- `example.csv`: Um arquivo de valores separados por vírgula (CSV) de exemplo, contendo dados usados ou processados pela aplicação.
- `CMakeLists.txt`: O arquivo de configuração de compilação do CMake que define os alvos (targets), vincula as bibliotecas do Qt e gerencia a compilação.
- `.gitignore`: Especifica arquivos intencionalmente não rastreados (como diretórios de compilação e binários) que o Git deve ignorar.

## Dependências

Para compilar e executar este projeto, você precisará ter o seguinte instalado em seu sistema:

- **Compilador C++**: Um compilador C++ padrão (ex: GCC, Clang ou MSVC) com suporte a C++11/17 ou superior.
- **CMake**: Versão 3.16 ou mais recente (verifique o seu `CMakeLists.txt` específico para saber a versão mínima exigida).
- **Qt Framework**: Qt 5 ou Qt 6 (especificamente os módulos Core, Gui, Qml e Quick).

### Instalando Dependências (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev
```

## Instruções de Compilação

1. Abra um terminal e navegue até a raiz do diretório do projeto.
2. Crie um novo diretório chamado `build` e entre nele:
    ```bash
    mkdir build
    cd build
    ```
3. Gere os arquivos de compilação usando o CMake:
    ```bash
    cmake ..
    ```
4. Compile o projeto:
    ```bash
    cmake --build .
    ```

## Executando a Aplicação

Assim que o processo de compilação for concluído com sucesso, um executável será gerado dentro do diretório `build`. Você pode executá-lo diretamente pelo terminal.

```bash
./simulador_so
```
