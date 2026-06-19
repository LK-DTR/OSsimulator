# Makefile de entrega. Encaminha para o CMake, pois o projeto usa Qt/QML
# (geração de moc e do módulo QML), que exigem o sistema de build do Qt.
# Uso: make (compila) | make run | make clean

BUILD_DIR := build

.PHONY: all build run clean

all: build

build:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/simulador_so

clean:
	rm -rf $(BUILD_DIR)
