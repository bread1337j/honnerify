.PHONY: compile run clean

BIN = bin
TARGET = $(BIN)/honnerify
BUILD = build
SRC = src
INCLUDE = include

INCLUDES = -I$(INCLUDE)
CFLAGS = $(INCLUDES) -lSDL3 -MMD -MP -lm -fopenmp
ARGS = honner.png SmallTarget.png

DEPS := $(OBJECTS:.o=.d)

SOURCES := $(shell find $(SRC) -name '*.c')
HEADERS := $(shell find $(SRC) -name '*.h')
HEADERS := $(shell find $(INCLUDE) -name '*.h')
OBJECTS = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SOURCES))

run: compile
	./$(TARGET) $(ARGS)

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
