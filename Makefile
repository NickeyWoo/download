

TARGET := download

all: $(TARGET)

clean:
	rm -f $(TARGET)

$(TARGET): main.cpp
	g++ main.cpp -I/usr/include -o $(TARGET) -lboost_regex -lcurl

