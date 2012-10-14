

TARGET := download

all: $(TARGET)

clean:
	rm -f $(TARGET)

$(TARGET):
	g++ main.cpp -o $(TARGET) -lcurl

