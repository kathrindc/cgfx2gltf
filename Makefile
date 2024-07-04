.PHONY: all
all:
	gcc main.c -std=c99 -pedantic -o cgfx2gltf

clean:
	rm -f cgfx2gltf
