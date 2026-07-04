CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRC = $(wildcard src/*.cpp)
OBJDIR = obj
OBJ = $(patsubst src/%.cpp, $(OBJDIR)/%.o, $(SRC))
TARGET = shiro

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) src/*.o $(TARGET)

.PHONY: all clean