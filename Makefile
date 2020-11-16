CXX      := -g++
CXXFLAGS := -Wall -Wno-psabi -Werror -Wextra -Wno-implicit-fallthrough
LDFLAGS  := -L/usr/lib -lbluetooth -L/usr/local/lib -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)
APP_DIR  := $(BUILD)
RELEASE_DIR  := $(BUILD)/release
TARGET   := govee-gateway
ifeq ($(MQTT),1)
CXXFLAGS += -DMQTT
LDFLAGS  += -lpaho-mqttpp3 -lpaho-mqtt3a
endif

INCLUDE  := -I. -I./src -I/usr/local/include
SRC      :=                      \
   $(wildcard *.cpp) \
   $(wildcard src/*.cpp)         \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
         := $(OBJECTS:.o=.d)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@echo "Test version in ${APP_DIR}"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

$(RELEASE_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(RELEASE_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release doc info

all: clean
all: build $(APP_DIR)/$(TARGET)

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug:
	@echo "Debug version in ${APP_DIR}"
debug: CXXFLAGS += -DDEBUG -g
debug: all

release:
	@echo "Release version in ${RELEASE_DIR}"
release: clean
release: CXXFLAGS += -O3 -Wno-array-bounds -Wno-stringop-truncation
release: build $(RELEASE_DIR)/$(TARGET)

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf doc/*

doc:
	@echo "Building documentation"
	@doxygen ./doxygen.config

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
