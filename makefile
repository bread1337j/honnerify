.PHONY: compile run clean

BIN = bin
TARGET = $(BIN)/honnerify
BUILD = build
SRC = src
INCLUDE = include
OUTPUT = output

INCLUDES = -I$(INCLUDE)
CFLAGS = $(INCLUDES) -MMD -MP -lm 
CC = nvcc
ARGS = -t SmallTarget.png -s SmallSrc.png



DEPS := $(OBJECTS:.o=.d)

SOURCES := $(shell find $(SRC) -name '*.c')
HEADERS := $(shell find $(SRC) -name '*.h')
HEADERS := $(shell find $(INCLUDE) -name '*.h')
OBJECTS = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SOURCES))

run: compile $(OUTPUT)
	./$(TARGET) $(ARGS)

compile: $(TARGET)
	@echo $(OBJECTS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) 

$(BUILD)/%.o: $(SRC)/%.c $(BUILD)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(BUILD): 
	@mkdir -p build

$(OUTPUT): 
	@mkdir -p output
	@mkdir -p output/gif

clean: 
	rm -rf $(BUILD) $(BIN)

-include $(DEPS) 
