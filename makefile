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
#Total hip obliteration.
ARGS = -t SmallHonner.png -s RescaledTarget.png -d
# ARGS = -t ReallySmall1.png -s ReallySmall2.png -d



DEPS := $(OBJECTS:.o=.d)

_SOURCES := $(shell find $(SRC) -name '*.c')
CUSOURCES := $(shell find $(SRC) -name '*.cu')
HEADERS := $(shell find $(SRC) -name '*.h')
HEADERS := $(shell find $(INCLUDE) -name '*.h')
_OBJECTS = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(_SOURCES))
CUOBJECTS = $(patsubst $(SRC)/%.cu, $(BUILD)/%.o, $(CUSOURCES))

SOURCES = $(_SOURCES) $(CUSOURCES)
OBJECTS = $(_OBJECTS) $(CUOBJECTS)



run: compile $(OUTPUT)
	./$(TARGET) $(ARGS)

compile: $(TARGET)
	@echo $(OBJECTS)

test: 
	@echo $(SOURCES)
	@echo $(OBJECTS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN)
	nvcc $(CFLAGS) $(OBJECTS) -o $(TARGET) 

$(BUILD)/%.o: $(SRC)/%.c $(BUILD)
	@mkdir -p $(@D)
	gcc $(CFLAGS) -c -o $@ $< 

$(BUILD)/%.o: $(SRC)/%.cu $(BUILD)
	@mkdir -p $(@D)
	nvcc $(CFLAGS) -c -o $@ $< 

$(BUILD): 
	@mkdir -p build

$(OUTPUT): 
	@mkdir -p output
	@mkdir -p output/gif

clean: 
	rm -rf $(BUILD) $(BIN)

-include $(DEPS) 
