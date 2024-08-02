#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>

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
void renderWithZoom(wchar_t **img);
void *listenToMovement(void *arg);
wchar_t **getZoomedImg(wchar_t **img);
int getNumOfEmojis(wchar_t *str);
int getCtrlInput();
int isControlKey(Directions d);
void printZoomedImg(wchar_t **zoomedImg);
void freeZoomedImg(wchar_t **zoomedImg);

int main(int argc, char const *argv[]) {
	setlocale(LC_ALL, "");
	zoomASCIIImg("./ascii-images/EACH-map.txt");
	return 0;
}

void zoomASCIIImg(char *filePath) {
	// settings
	imgHeight = getNumberOfLines(filePath);
	imgWidth = getNumberOfRows(filePath);
	zoomWidth = 20;
	zoomHeight = 10;

	// image in a 2D array format
	wchar_t **img = malloc(imgHeight * sizeof(wchar_t *));

	FILE *ASCIIImgFile = fopen(filePath, "r");
	if (!ASCIIImgFile) {
		exit(1);
	}

	// filling the img contents
	wchar_t buffer[imgWidth + 1];

	for(int i = 0; i < imgWidth; i++) {
		buffer[i] = L'A';
	}
	buffer[imgWidth] = L'\0';

	int currLine = 0;
	while (fgetws(buffer, sizeof(buffer), ASCIIImgFile) && currLine < imgHeight) {
		img[currLine] = malloc((wcslen(buffer) + 1) * sizeof(wchar_t) );
		if (!img[currLine]) {
			exit(1);
		}
		wcscpy(img[currLine], buffer);
		img[currLine][wcslen(buffer)] = L'\0';
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
		wchar_t ch;
		while ((ch = fgetwc(file)) != EOF) {
			if (ch == L'\n') {
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

void renderWithZoom(wchar_t **img) {
	struct termios old_conf, new_conf;
	tcgetattr(STDIN_FILENO, &old_conf);
	new_conf = old_conf;
	new_conf.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_conf);

	pthread_t movementThread;
	pthread_create(&movementThread, NULL, listenToMovement, NULL);

	wchar_t **zoomedImg;
	while(1) {
		if(shouldUpdate) {
			zoomedImg = getZoomedImg(img);
			system("clear");
			printZoomedImg(zoomedImg);
			shouldUpdate = 0;
			freeZoomedImg(zoomedImg);
		}
	}

	pthread_join(movementThread, NULL);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_conf);
}

void *listenToMovement(void *arg) {
	while(1) {
		Directions d = getchar();
		switch(d) {
			case 'w':
				if(zoomY > 0) {
					zoomY--;
					shouldUpdate = 1;
					break;
				}
				shouldUpdate = 0;
				break;
			case 's':
				if(zoomY < imgHeight - zoomHeight) {
					zoomY++;
					shouldUpdate = 1;
					break;
				}
				shouldUpdate = 0;
				break;
			case 'a':
				if(zoomX > 0) {
					zoomX--;
					shouldUpdate = 1;
					break;
				}
				shouldUpdate = 0;
				break;
			case 'd':
				if(zoomX < imgWidth - zoomWidth - 1) {
					zoomX++;
					shouldUpdate = 1;
					break;
				}
				shouldUpdate = 0;
				break;
			default:
				shouldUpdate = 0;
				break;
		}
	}
}

wchar_t **getZoomedImg(wchar_t **img) {
	wchar_t **zoomedImg = malloc(zoomHeight * sizeof(wchar_t *));

	if(zoomedImg == NULL) {
		printf("couldn't malloc zoomedImg");
		exit(1);
	}

	for(int i = 0; i < zoomHeight; i++) {
		zoomedImg[i] = malloc((zoomWidth + 2) * sizeof(wchar_t));
		zoomedImg[i][0] = L'\0';
	}

	for(int i = 0; i < zoomHeight; i++) {
		// how many emojis are to the left of our view
		int emojiCount = getNumOfEmojis(img[zoomY + i]);
		for(int i = 0; i < emojiCount; i++){
			wcscat(zoomedImg[i], L"  ");
		}
		wcsncat(zoomedImg[i], &(img[zoomY + i][zoomX]), zoomWidth - emojiCount);
		int lastPos = wcslen(zoomedImg[i]);
		if(zoomedImg[i][lastPos - 1] != L'\n') {
			wcscat(zoomedImg[i], L"\n");
		}
		// zoomedImg[pos] = L'\n';
		// zoomedImg[pos + 1] = L'\0';		
	}

	return zoomedImg;
}

int getNumOfEmojis(wchar_t *str) {
	int count = 0; 
	for(int i = 0; i < zoomX; i++) {
		if(str[i] >= 0x1F300 && str[i] <= 0x1FAF6) {
			count++;
		}
	}
	return count;
}

void printZoomedImg(wchar_t **zoomedImg) {
	for(int i = 0; i < zoomHeight; i++){
		wprintf(L"%ls", zoomedImg[i]);
	}
}

void freeZoomedImg(wchar_t **zoomedImg) {
	for(int i = 0; i < zoomHeight; i++){
		free(zoomedImg[i]);
	}
	//free(zoomedImg);
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