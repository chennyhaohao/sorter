all: node myapp

myapp: root.c
	gcc root.c -o myapp

node: compare.o merge.o quicksort.o node.o
	gcc compare.o node.o merge.o quicksort.o -o node

compare.o: compare.c
	gcc -c compare.c

node.o: node.c
	gcc -c node.c

merge.o: merge.c
	gcc -c merge.c

quicksort.o: quicksort.c
	gcc -c quicksort.c

root.o: root.c
	gcc -c root.c
