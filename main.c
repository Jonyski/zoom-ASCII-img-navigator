#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>

#define MAX_IMG_WIDTH 256

int imgWidth;
int imgHeight;
int zoomWidth;
int zoomHeight;
// default zoom position values
volatile int zoomX = 0;
volatile int zoomY = 0;


void zoomASCIIImg(char *filePath);
int getNumberOfLines(char *filePath);
int getNumberOfRows(char *filePath);
void renderWithZoom(char *img[]);
void *listenToMovement(void *arg);
char *getZoomedImg(char**img);

int main(int argc, char const *argv[]) {
	zoomASCIIImg("./ascii-images/EACH-map.txt");
	return 0;
}

void zoomASCIIImg(char *filePath) {
	imgHeight = getNumberOfLines(filePath);
	imgWidth = getNumberOfRows(filePath);
	zoomWidth = 20;
	zoomHeight = 10;
	char img[imgHeight - 1][imgWidth - 1];
	renderWithZoom(img);

}


int getNumberOfLines(char *filePath) {
	FILE *file = fopen(filePath, "r");
	char buffer[MAX_IMG_WIDTH];
	int numOfLines = 0;
	while(fgets(buffer, MAX_IMG_WIDTH, file)) {
		numOfLines++;
	}
	fclose(file);
	return numOfLines;
}

int getNumberOfRows(char *filePath) {
	FILE *file = fopen(filePath, "r");
	char buffer[MAX_IMG_WIDTH];
	int maxNumOfRows = 0;
	for(int i = 0; i < imgHeight; i++) {
		int numOfRows = 0;
		while(fgetc(file) != '\n') {
			numOfRows++;
		}
		if(numOfRows > maxNumOfRows) maxNumOfRows = numOfRows;
	}
	fclose(file);
	return maxNumOfRows;
}

void renderWithZoom(char *img[]) {
	struct termios old_conf, new_conf;
	tcgetattr(STDIN_FILENO, &old_conf);
	new_conf = old_conf;
	new_conf.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_conf);

	pthread_t movementThread;
	pthread_create(&movementThread, NULL, listenToMovement, NULL);

	char *zoomedImg;
	while(1) {
		zoomedImg = getZoomedImg(img);
		printf("%s", zoomedImg);
		free(zoomedImg);
	}

	pthread_join(movementThread, NULL);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_conf);
}

void *listenToMovement(void *arg) {
	while(1) {
		int c;
		if(c = (int) getch()) {
			switch(c) {
				case KEY_UP:
					zoomY > 0 ? zoomY-- : 0;
				case KEY_DOWN:
					zoomY < imgHeight - zoomHeight - 1 ? zoomY++ : 0;
				case KEY_LEFT:
					zoomX > 0 ? zoomX-- : 0;
				case KEY_RIGHT:
					zoomX < imgWidth - zoomWidth - 1 ? zoomX++ : 0;
			}
		}
	}
}

char *getZoomedImg(char **img) {
	// the * 4 is to be able to hold emojis and such (i don't like using wide chars)
	char *zoomedImg = malloc(zoomWidth * zoomHeight * sizeof(char) * 4 + 1);
	for (int i = 0; i < zoomHeight; ++i){
		strncat(zoomedImg, img[zoomY] + zoomX, zoomWidth);
	}
	return zoomedImg;
}