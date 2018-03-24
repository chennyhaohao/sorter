all: node myapp quicksort shellsort bubblesort

myapp: root.c
	gcc root.c -lm -o myapp

node: compare.o merge.o node.o
	gcc compare.o node.o merge.o -lm -o node

quicksort: quicksort.o compare.o sort_func.o
	gcc quicksort.o compare.o sort_func.o -o quicksort

shellsort: shellsort.o compare.o sort_func.o
	gcc shellsort.o compare.o sort_func.o -o shellsort

bubblesort: bubblesort.o compare.o sort_func.o
	gcc bubblesort.o compare.o sort_func.o -o bubblesort

quicksort.o: quicksort.c
	gcc -c quicksort.c

shellsort.o: shellsort.c
	gcc -c shellsort.c

bubblesort.o: bubblesort.c
	gcc -c bubblesort.c

compare.o: compare.c
	gcc -c compare.c

node.o: node.c
	gcc -c node.c

merge.o: merge.c
	gcc -c merge.c

sort_func.o: sort_func.c
	gcc -c sort_func.c

root.o: root.c
	gcc -c root.c


