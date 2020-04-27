all:
	gcc project.c -o project -lrt
	gcc child.c -o child -lrt
clean:
	rm -f project child
