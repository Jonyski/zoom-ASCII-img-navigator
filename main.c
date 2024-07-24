#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>

#define MAX_IMG_WIDTH 256

typedef enum {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	EMPTY
} Directions;

int imgWidth;
int imgHeight;
int zoomWidth;
int zoomHeight;
// default zoom position values
volatile int zoomX = 0;
volatile int zoomY = 0;
volatile int shouldUpdate = 1;


void zoomASCIIImg(char *filePath);
int getNumberOfLines(char *filePath);
int getNumberOfRows(char *filePath);
void renderWithZoom(char **img);
void *listenToMovement(void *arg);
char *getZoomedImg(char **img);
int getCtrlInput();
int isControlKey(Directions);

int main(int argc, char const *argv[]) {
	zoomASCIIImg("./ascii-images/EACH-map-pureASCII.txt");
	return 0;
}

void zoomASCIIImg(char *filePath) {
	// settings
	imgHeight = getNumberOfLines(filePath);
	imgWidth = getNumberOfRows(filePath);
	zoomWidth = 20;
	zoomHeight = 10;

	// image in a 2D array format
	char **img = malloc(imgHeight * sizeof(char *));

	FILE *ASCIIImgFile = fopen(filePath, "r");
	if (!ASCIIImgFile) {
		exit(1);
	}

	// filling the img contents
	char buffer[imgWidth * 4 + 1];
	int currLine = 0;
	while (fgets(buffer, sizeof(buffer), ASCIIImgFile) && currLine < imgHeight) {
		img[currLine] = malloc((strlen(buffer)) * sizeof(char));
		if (!img[currLine]) {
			exit(1);
		}
		strcpy(img[currLine], buffer);
		img[currLine][strlen(buffer)] = '\0';
		currLine++;
	}
	fclose(ASCIIImgFile);
	renderWithZoom(img);
	// Free the allocated memory
	for (int i = 0; i < imgHeight; i++) {
		free(img[i]);
	}
	free(img);

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
		if (file == NULL) {
			exit(1);
		}
		int maxLength = 0;
		int currentLength = 0;
		char ch;
		while ((ch = fgetc(file)) != EOF) {
			if (ch == '\n') {
				if (currentLength > maxLength) {
					maxLength = currentLength;
				}
				currentLength = 0; // Reset length for the next line
			} else {
				currentLength++;
			}
		}
		if (currentLength > maxLength) {
			maxLength = currentLength;
		}
		fclose(file);
		return maxLength;
}

void renderWithZoom(char **img) {
	struct termios old_conf, new_conf;
	tcgetattr(STDIN_FILENO, &old_conf);
	new_conf = old_conf;
	new_conf.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_conf);

	pthread_t movementThread;
	pthread_create(&movementThread, NULL, listenToMovement, NULL);

	char *zoomedImg;
	while(1) {
		if(shouldUpdate) {
			zoomedImg = getZoomedImg(img);
			system("clear");
			printf("%s", zoomedImg);
			free(zoomedImg);
			shouldUpdate = 0;
		}
	}

	pthread_join(movementThread, NULL);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_conf);
}

void *listenToMovement(void *arg) {
	while(1) {
		//Directions d = getCtrlInput();
		Directions d = getchar();
		switch(d) {
			case 'w':
				zoomY > 0 ? zoomY-- : 0;
				shouldUpdate = 1;
				break;
			case 's':
				zoomY < imgHeight - zoomHeight - 1 ? zoomY++ : 0;
				shouldUpdate = 1;
				break;
			case 'a':
				zoomX > 0 ? zoomX-- : 0;
				shouldUpdate = 1;
				break;
			case 'd':
				zoomX < imgWidth - zoomWidth - 1 ? zoomX++ : 0;
				shouldUpdate = 1;
				break;
			default:
				shouldUpdate = 0;
				break;
		}
		// switch(d) {
		// 	case KEY_UP:
		// 		zoomY > 0 ? zoomY-- : 0;
		// 		shouldUpdate = 1;
		// 		break;
		// 	case KEY_DOWN:
		// 		zoomY < imgHeight - zoomHeight - 1 ? zoomY++ : 0;
		// 		shouldUpdate = 1;
		// 		break;
		// 	case KEY_LEFT:
		// 		zoomX > 0 ? zoomX-- : 0;
		// 		shouldUpdate = 1;
		// 		break;
		// 	case KEY_RIGHT:
		// 		zoomX < imgWidth - zoomWidth - 1 ? zoomX++ : 0;
		// 		shouldUpdate = 1;
		// 		break;
		// 	case EMPTY:
		// 		shouldUpdate = 0;
		// 		break;
		// }
	}
}

char *getZoomedImg(char **img) {
	// the * 4 is to be able to hold emojis and such (i don't like using wide chars)
	char *zoomedImg = malloc(zoomWidth * zoomHeight * sizeof(char) * 4 + 1);
	for(int i = 0; i < zoomWidth * zoomHeight * 4 + 1; i++) {
		zoomedImg[i] = '\0';
	}

	for (int i = 0; i < zoomHeight; ++i){
		strncat(zoomedImg, &(img[zoomY + i][zoomX]), zoomWidth);
		zoomedImg[strlen(zoomedImg)] = '\n';
		zoomedImg[strlen(zoomedImg) + 1] = '\0';
	}
	return zoomedImg;
}

int getCtrlInput() {
	int ch = getchar();
		switch (ch) {
			case 'w':
				return UP;
			case 's':
				return DOWN;
			case 'd':
				return RIGHT;
			case 'a':
				return LEFT;
		}
	return EMPTY;
}

int isControlKey(Directions d) {
	switch(d) {
		case UP:
			return 1;
			break;
		case DOWN:
			return 1;
			break;
		case LEFT:
			return 1;
			break;
		case RIGHT:
			return 1;
			break;
		case EMPTY:
			return 0;
			break;
	}
	return 0;
}