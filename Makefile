all: node myapp sorter

myapp: root.c
	gcc root.c -o myapp

node: compare.o merge.o node.o
	gcc compare.o node.o merge.o -o node

compare.o: compare.c
	gcc -c compare.c

node.o: node.c
	gcc -c node.c

merge.o: merge.c
	gcc -c merge.c

sort.o: sort_func.c
	gcc -c sort_func.c

root.o: root.c
	gcc -c root.c

sorter: sorter.o sort_func.o compare.o
	gcc sorter.o sort_func.o compare.o -o sorter

sorter.o: sorter.c
	gcc -c sorter.c

