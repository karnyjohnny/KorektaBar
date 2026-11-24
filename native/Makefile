CXX      ?= g++
WINDRES  ?= windres

# Domyślnie flagi przenośne (bezpieczne dla GitHub Actions / innych CPU).
# Budowę pod lokalny procesor (np. Celeron N3350 => -march=native / goldmont):
#   make -C native -j2 MARCH="-march=native -mtune=native"
MARCH    ?=

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/KorektaBar.exe

SRCS := \
	src/main.cpp \
	src/Config.cpp \
	src/TextFormatter.cpp \
	src/PolishAnalyzer.cpp \
	src/SpellCheckerWin.cpp \
	src/BackgroundWorker.cpp \
	src/D2DEditor.cpp \
	src/SettingsDialog.cpp \
	src/MainWindow.cpp

OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
RES_OBJ := $(BUILD_DIR)/KorektaBar.res.o

CXXFLAGS := -std=c++20 -O2 $(MARCH) -ffunction-sections -fdata-sections -DNDEBUG -Wall -Wextra -finput-charset=UTF-8 -DUNICODE -D_UNICODE -Isrc -Iresources
LDFLAGS  := -mwindows -municode -static -static-libgcc -static-libstdc++ -Wl,--gc-sections -s
LIBS     := -ld2d1 -ldwrite -ldwmapi -lcomctl32 -lshell32 -lole32 -luuid -luxtheme

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RES_OBJ): resources/KorektaBar.rc resources/resource.h resources/app.manifest ../assets/KorektaBar.ico | $(BUILD_DIR)
	$(WINDRES) -Iresources $< -O coff -o $@

$(TARGET): $(OBJS) $(RES_OBJ)
	$(CXX) $(OBJS) $(RES_OBJ) $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
