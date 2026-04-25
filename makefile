.PHONY: compile run clean

BIN = bin
TARGET = $(BIN)/termslides
BUILD = build
SRC = src

INCLUDES = -Iinclude -I$(SRC)/gui
CFLAGS = $(INCLUDES) -lncurses -MMD -MP

DEPS := $(OBJECTS:.o=.d)

SOURCES := $(shell find $(SRC) -name '*.c')
HEADERS := $(shell find $(SRC) -name '*.h')
OBJECTS = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SOURCES))

run: compile
	./$(TARGET)

compile: $(TARGET)
	@echo $(OBJECTS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN)
	gcc $(CFLAGS) $(OBJECTS) -o $(TARGET) $(HEADERS)

$(BUILD)/%.o: $(SRC)/%.c $(BUILD)
	@mkdir -p $(@D)
	gcc $(CFLAGS) -c -o $@ $< 

$(BUILD): 
	@mkdir -p build

clean: 
	rm -rf $(BUILD) $(BIN)

-include $(DEPS) 
