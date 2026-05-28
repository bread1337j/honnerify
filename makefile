.PHONY: compile run clean test

BIN     = bin
TARGET  = $(BIN)/honnerify
BUILD   = build
SRC     = src
INCLUDE = include
OUTPUT  = output

INCLUDES = -I$(INCLUDE)
CFLAGS   = $(INCLUDES) -MMD -MP -lm -O3
CC       = nvcc
#Total hip obliteration.
ARGS = -t SmallTarget.png -s SmallSrc.png -g -R -c -v
#ARGS = -t ReallySmall1.png -s ReallySmall2.png -c -g $(ARG)

_SOURCES  := $(shell find $(SRC) -name '*.c')
CUSOURCES := $(shell find $(SRC) -name '*.cu')

_OBJECTS  = $(patsubst $(SRC)/%.c,  $(BUILD)/%.o, $(_SOURCES))
CUOBJECTS = $(patsubst $(SRC)/%.cu, $(BUILD)/%.o, $(CUSOURCES))

SOURCES = $(_SOURCES) $(CUSOURCES)
OBJECTS = $(_OBJECTS) $(CUOBJECTS)

# Must be defined after OBJECTS so the substitution actually has something to work with
DEPS := $(OBJECTS:.o=.d)


run: compile | $(OUTPUT)
	./$(TARGET) $(ARGS)

compile: $(TARGET)
	@echo $(OBJECTS)

test:
	@echo $(SOURCES)
	@echo $(OBJECTS)

$(TARGET): $(OBJECTS) | $(BIN)
	nvcc $(CFLAGS) $(OBJECTS) -o $(TARGET)

# Use order-only prerequisite (| $(BUILD)) so the directory timestamp
# never causes object files to be considered out of date
$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	@mkdir -p $(@D)
	gcc $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: $(SRC)/%.cu | $(BUILD)
	@mkdir -p $(@D)
	nvcc $(CFLAGS) -c -o $@ $<

$(BUILD):
	@mkdir -p $(BUILD)

$(BIN):
	@mkdir -p $(BIN)

$(OUTPUT):
	@mkdir -p $(OUTPUT)
	@mkdir -p $(OUTPUT)/gif

clean:
	rm -rf $(BUILD) $(BIN)

-include $(DEPS)
