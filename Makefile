FeedHandler: main.cpp
	g++ -g -o feedHandler main.cpp -I. -std=c++17 

.PHONY: clean

clean:
	rm -f feedHandler
