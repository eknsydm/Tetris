build:
	gcc -Wall -lSDL2 ./src/*.c* -o game
run:
	SDL_VIDEODRIVER=wayland ./game
clean:
	rm game
