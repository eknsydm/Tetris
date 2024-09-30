build:
	gcc -Wall -lSDL2_ttf -lSDL2 ./src/*.c* -o game
run:
	SDL_VIDEODRIVER=wayland ./game
clean:
	rm game
