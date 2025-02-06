CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -pthread

LDFLAGS := -pthread

INCLUDEDIRS := $(shell find inc -type d -print | sed 's/^/-I/')
DEPENDDIRS := -Isubmodules/json/single_include/nlohmann/

SOURCES := $(shell find ./src -name "*.cpp")

OBJECTS := $(SOURCES:.cpp=.o)

TARGET := core

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "🔗 Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	@echo "✂️  Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDEDIRS) $(DEPENDDIRS) -c $< -o $@

re: fclean all

start: run

run: all
	./$(TARGET)

ren: re run

clean:
	@echo "🧹 Cleaning up..."
	rm -f $(OBJECTS)

fclean: clean
	@echo "🗑️ Removing $(TARGET)..."
	rm -f $(TARGET)

.PHONY: all clean fclean re run ren start
