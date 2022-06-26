#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

char* input_file; //input_file.bmp - monochrome picture in BMP
char* dir_name = ""; //name of the directory for storing game generations 

//File header (BitMapFileHeader)
typedef struct {
	char bfType[2]; //Defines the file type (must be BMP)
	uint32_t bfSize; //Defines the size of the bitmap file, in bytes
	uint16_t bfReserved1; //Reserved; must be zero
	uint16_t bfReserved2; //Reserved; must be zero
	uint32_t bfOffBits; //Defines the offset from the beginning of the bitmap bits in the BITMAPFILEHEADER structure, in bytes
    
} BITMAPFILEHEADER;  

//Information header (BitMapInfoHeader)
typedef struct {
	uint32_t biSize; //Defines the number of bytes required for the structure
	int32_t biWidth; //Defines the width of the bitmap, in pixels
	int32_t biHeight; //Defines the height of the bitmap, in pixels
	uint16_t biPlanes; //Defines the number of planes of the target device. This value must be set to 1
	uint16_t biBitCount; //Defines the number of bits per pixel
	uint32_t biCompression; //Defines the compression type for a compressed bottom-up bitmap
	uint32_t biSizeImage; //Defines the size of the image, in bytes
	int32_t biXPelsPerMeter; //Defines the horizontal resolution for the receiving device of the bitmap, in pixels per meter
	int32_t biYPelsPerMeter; //Defines the vertical resolution for the receiving device of the bitmap, in pixels per meter
	uint32_t biClrUsed; //Defines the number of color indexes in the color table that are actually used by the dot pic
	uint32_t biClrImportant; //Defines the number of color indexes that are required to show a bitmap on the screen
} BITMAPINFOHEADER;

//reading the file header
BITMAPFILEHEADER readBITMAPFILEHEADER(FILE* F) {
	BITMAPFILEHEADER Header;
	
	fread(&Header.bfType, 1, 2, F); //read a file of 1 * 2 bytes in a cell with the address header.bfType
	fread(&Header.bfSize, 4, 1, F); //read a 4*1 byte file into a cell with the address header.bfSize
	fread(&Header.bfReserved1, 2, 1, F); //read a 2*1 byte file into a cell with the address header.bfReserved1
	fread(&Header.bfReserved2, 2, 1, F); //read a 2*1 byte file into a cell with the address header.bfReserved2
	fread(&Header.bfOffBits, 4, 1, F); //read a 4*1 byte file into a cell with the address header.bfOffBits
	
	return Header;
}

//reading the information header
BITMAPINFOHEADER readBITMAPINFOHEADER(FILE* F) {
	BITMAPINFOHEADER infoHeader;
	
	fread(&infoHeader.biSize, 4, 1, F); //read a 4*1 byte file into a cell with the address header.biSize
	fread(&infoHeader.biWidth, 4, 1, F); //read a 4*1 byte file into a cell with the address header.biWidth
	fread(&infoHeader.biHeight, 4, 1, F); //read a 4*1 byte file into a cell with the address header.biHeight
	fread(&infoHeader.biPlanes, 2, 1, F); //read a 2*1 byte file into a cell with the address header.biPlanes
	fread(&infoHeader.biBitCount, 2, 1, F); //считываем файл размером 2*1 байт в ячейку с адресом header.biBitCount
	fread(&infoHeader.biCompression, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biCompression
	fread(&infoHeader.biSizeImage, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biSizeImage
	fread(&infoHeader.biXPelsPerMeter, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biXPelsPerMeter
	fread(&infoHeader.biYPelsPerMeter, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biYPelsPerMeter
	fread(&infoHeader.biClrUsed, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biClrUsed
	fread(&infoHeader.biClrImportant, 4, 1, F); //считываем файл размером 4*1 байт в ячейку с адресом header.biClrImportant
	
	return infoHeader;
}
 
//Структура RGBQUAD, описывающая триплет RGB  
typedef struct {
	int32_t rgbBlue;
	int32_t rgbGreen;
	int32_t rgbRed;
	int32_t rgbReserved; //Зарезервирован; должен быть нуль.
} RGBQUAD;

//инициализация массива пикселей
RGBQUAD** pixelArray(FILE* F, BITMAPINFOHEADER infoHeader) {
	RGBQUAD** pixels = (RGBQUAD**) malloc(infoHeader.biHeight * sizeof(RGBQUAD*));
	
	for(int i = 0; i < infoHeader.biHeight; ++i) {
		pixels[i] = (RGBQUAD*) malloc(infoHeader.biWidth * sizeof(RGBQUAD));
	}
	
	//cчитываем байты цветов (с верхнего левого угла до правого нижнего)    
	for (int i = infoHeader.biHeight - 1; i >= 0; i--) { 
		for (int j = 0; j < infoHeader.biWidth; j++) {
			pixels[i][j].rgbBlue = getc(F);
			pixels[i][j].rgbGreen = getc(F);
			pixels[i][j].rgbRed = getc(F);
		}
		if (infoHeader.biWidth % 4 != 0) {
			getc(F);
		}
	}
	
	return pixels;
}

//Проверка на "живую" клетку (чёрный пискель)
int isBlack(RGBQUAD pixel) {
	if(pixel.rgbBlue==0 && pixel.rgbGreen == 0 && pixel.rgbRed == 0) {
		return 1;
	}
	return 0;
}

//преобразовываем цвета к одному
RGBQUAD** from_map_to_rgb(int** in, BITMAPINFOHEADER infoHeader) {
	RGBQUAD** pixels = (RGBQUAD**) malloc(infoHeader.biHeight * sizeof(RGBQUAD*)); //выделяем память
	
	for(int i = 0; i < infoHeader.biHeight; ++i) {
		pixels[i] = (RGBQUAD*) malloc(infoHeader.biWidth * sizeof(RGBQUAD)); //выделяем память 
	}
	
	for (int i = infoHeader.biHeight - 1; i >= 0; i--) {
		for (int j = 0 ; j < infoHeader.biWidth; j++) {
			if(in[i][j] == 1) { //чёрный цвет
				pixels[i][j].rgbBlue = 0;
				pixels[i][j].rgbGreen = 0;
				pixels[i][j].rgbRed = 0;
			} else { //белый цвет
				pixels[i][j].rgbBlue = 255;
				pixels[i][j].rgbGreen = 255;
				pixels[i][j].rgbRed = 255;
			}   
		}
	}
	return pixels;
}

//создадим карту из чёрных и белых клеток
int** map(RGBQUAD** pixels, BITMAPINFOHEADER infoHeader) {
	
	int** map = (int**) malloc(infoHeader.biHeight * sizeof(int*)); //выделяем память
	
	for(int i = 0; i < infoHeader.biHeight; ++i) {
		map[i] = (int*) malloc(infoHeader.biWidth * sizeof(int)); //выделяем память
	}
	
	//проверяем каждую клетку на "жизнь" 
	for (int i = 0; i < infoHeader.biHeight; i++) {
		for (int j = 0; j < infoHeader.biWidth; j++) { 
			map[i][j] = (isBlack(pixels[i][j]) ? 1 : 0);
		}
	} 
	
	return map;
}

//создаём BMP
void makeBmp(FILE* F, BITMAPFILEHEADER Header, BITMAPINFOHEADER infoHeader, RGBQUAD** pixels) {	
	fwrite(&Header.bfType, 1, 2, F); //записываем содержимое ячейки Header.bfType в файл
	fwrite(&Header.bfSize, 4, 1, F); //записываем содержимое ячейки Header.bfSize в файл
	fwrite(&Header.bfReserved1, 2, 1, F); //записываем содержимое ячейки Header.bfReserved1 в файл
	fwrite(&Header.bfReserved2, 2, 1, F); //записываем содержимое ячейки Header.bfReserved2 в файл
	fwrite(&Header.bfOffBits, 4, 1, F); //записываем содержимое ячейки Header.bfOffBits в файл
	fwrite(&infoHeader.biSize, 4, 1, F); //записываем содержимое ячейки Header.biSize в файл
	fwrite(&infoHeader.biWidth, 4, 1, F); //записываем содержимое ячейки Header.biWidth в файл
	fwrite(&infoHeader.biHeight, 4, 1, F); //записываем содержимое ячейки Header.biHeight в файл
	fwrite(&infoHeader.biPlanes, 2, 1, F); //записываем содержимое ячейки Header.biPlanes в файл
	fwrite(&infoHeader.biBitCount, 2, 1, F); //записываем содержимое ячейки Header.biBitCount в файл
	fwrite(&infoHeader.biCompression, 4, 1, F); //записываем содержимое ячейки Header.biCompression в файл
	fwrite(&infoHeader.biSizeImage, 4, 1, F); //записываем содержимое ячейки Header.biSizeImage в файл
	fwrite(&infoHeader.biXPelsPerMeter, 4, 1, F); //записываем содержимое ячейки Header.biXPelsPerMeter в файл
	fwrite(&infoHeader.biYPelsPerMeter, 4, 1, F); //записываем содержимое ячейки Header..biYPelsPerMeter в файл
	fwrite(&infoHeader.biClrUsed, 4, 1, F); //записываем содержимое ячейки Header.biClrUsed в файл
	fwrite(&infoHeader.biClrImportant, 4, 1, F); //записываем содержимое ячейки Header.biClrImportant в файл
	
	//присваиваем каждой клетке соответствующий набор rgb
	for (int i = infoHeader.biHeight - 1; i >= 0; i--) {
		for (int j = 0 ; j < infoHeader.biWidth; j++) {
			putc(pixels[i][j].rgbBlue, F); 
			putc(pixels[i][j].rgbGreen, F);
			putc(pixels[i][j].rgbRed, F);
		}
		if (infoHeader.biWidth % 4 != 0) {
			putc(0, F);
		}
	}
}

int main(int argc, char* argv[]) {
	int max_iter = 15; //кол-во итераций 
	int dump_freq = 1; //частота сохранений
	
	for (int i = 1; i < argc; i += 2) {
		if (strcmp(argv[i], "--input") == 0) { 
			input_file = argv[i + 1];
		}
		if (strcmp(argv[i], "--output") == 0) {
			dir_name = argv[i + 1];
		}
		if (strcmp(argv[i], "--max_iter") == 0) {
			sscanf(argv[i + 1], "%d", &max_iter);
		}		
		if (strcmp(argv[i], "--dump_freq") == 0) {
			sscanf(argv[i + 1], "%d", &dump_freq);
		}
	}

	FILE* F = fopen(input_file, "r"); //открываем File

	if (F == NULL) { //проверка на существование вводного файла
		return 0;
	}

	//считываем заголовок файла
	BITMAPFILEHEADER Header = readBITMAPFILEHEADER(F);

	//считываем информационный заголовок
	BITMAPINFOHEADER infoHeader = readBITMAPINFOHEADER(F);
 
	RGBQUAD** pixelsrgb = pixelArray(F, infoHeader); //создаём массив пикселей
	int** pixels = map(pixelsrgb, infoHeader); //создаём карту
	int** new_pixels = map(pixelsrgb, infoHeader); ////создаём карту для будущего ответа

	fclose(F);

	char str[50]; 
	char path[100];
	int up, down, left, right;
	int neighbours; 

	for (int k = 0; k < max_iter; k++) {
		for (int i = 0; i < infoHeader.biHeight; i++) {
			//сосед сверху
			if (i != 0) {
				up = i-1;
			} else {  
				up = infoHeader.biHeight-1;
			}
			//сосед снизу    
			if (i != infoHeader.biHeight - 1) {
				down = i+1; 
			} else {
				down = 0;
			}

			for (int j = 0; j < infoHeader.biWidth; j++) {
				//сосед слева
				if (j != 0) {
					left = j-1;
				} else {
					left = infoHeader.biWidth-1;
				}
				//сосед справа
				if (j != infoHeader.biWidth - 1) {
					right = j+1;
				} else {    
					right = 0;
				}
				
				//считаем кол-во "живых" соседей
				neighbours = pixels[up][left]+ pixels[up][j] + pixels[up][right] + pixels[i][left] +
				             pixels[i][right] + pixels[down][left] + pixels[down][j] + pixels[down][right];
				//если текущая клетка "жива",..
				if (pixels[i][j] == 1) {
					//...но кол-во соседей не удовлетворяёет условию, то "убиваем" эту клетку,..
					if (neighbours < 2 || neighbours > 3) {
						new_pixels[i][j] = 0;
					//...иначе оставляем жить    
					} else {
						new_pixels[i][j] = 1;
					}
				}
				//если текущая клетка "мертва",..
				else {
					//...но кол-во соседей соответствует условию, то "оживляем" её,..
					if (neighbours == 3) {
						new_pixels[i][j] = 1;
					//...иначе оставляем "мёртвой"    
					} else {
						new_pixels[i][j] = 0;
					}
				}
			}
		}
		
		//переносим полученные значения в первоначальную карту
		for (int i = 0; i < infoHeader.biHeight; i++) {{
			for (int j = 0; j < infoHeader.biWidth; j++)  
				pixels[i][j] = new_pixels[i][j];
			}
		}
		
		if (k % dump_freq == 0) { 
			memset(str, 0, 50); //инициализируем str
			memset(path, 0, 100); //инициализируем path
			
			//получаем название поколения игры = k + dir_name
			//где k - номер итерации, dir_name - название директории
			sprintf(str, "%d.bmp", k); 
			strcpy(path, dir_name); 
			strcat(path, str);
			
			F = fopen(path, "w"); //создаём файл для записи
			
			//ответ данной итерации
			makeBmp(F, Header, infoHeader, from_map_to_rgb(pixels, infoHeader));
			
			fclose(F);
		}
	}

	return 0;
}
