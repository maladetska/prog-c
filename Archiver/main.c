#include <stdio.h>
#include <stdlib.h>

void Create(char *Name, int argc, char *argv[]) {
    unsigned long long int nameSize[128];//размер имени файла
    FILE *Archive = fopen(Name, "wb");   //создаём наш архив (в двоичном виде)
    FILE *f;

    //записываем файлы, которые надо занести в архив
    for (int i = 4; i < argc; i++) {
        f = fopen(argv[i], "rb");//открываем (в двоичном виде) файл, записанный в argv[i]
        if (f == NULL) {         //даже если текущего файла не существует, архивируем другие
            continue;
        }
        fseek(f, 0, SEEK_END);     // переносим указатель в конец файла
        nameSize[i - 4] = ftell(f);// возвращаем текущее положение внутреннего указателя,
                                   // т.е. кол-во байт от начала до конца файла.
        fclose(f);
    }

    //записываем имена файлов и их размер
    for (int i = 0; i < argc - 4; i++) {
        fprintf(Archive, "Name: %s Size: %llu ", argv[i + 4], nameSize[i]);
    }

    fprintf(Archive, "#\n");

    for (int i = 4; i < argc; i++) {
        f = fopen(argv[i], "rb");//открываем (в двоичном виде) файл, записанный в argv[i]

        char *buffer = (char *) malloc(sizeof(char) * nameSize[i - 4]);//выделяем память
        size_t result = fread(buffer, 1, nameSize[i - 4], f);          //считываем файл в буфер
        fwrite(buffer, 1, result, Archive);                            //записываем содержимое файла в архив

        if (f == NULL) {//если файл не существует, то сообщаем это
            printf("Can't open file :( %s\n", argv[i]);
            continue;
        } else {
            printf("Archived! %s\n", argv[i]);
        }
    }
}

void Extract(char *File) {
    FILE *Archive = fopen(File, "rb");//открываем (в двоичном виде) File
    unsigned long long int now;
    unsigned long long int start = 0;//счётчик символов
    int chr;                         //символ
    while (!feof(Archive)) {
        //chr = ;
        start++;
        if (getc(Archive) == '\n') {
            break;
        }
    }

    fseek(Archive, 0, SEEK_SET);    // переносим указатель в конец файла
    char filen[128] = {0};          //символьный массив заполняем нулями
    unsigned long long int filesize;//размер распаковываемого файла
    FILE *f;
    while (fscanf(Archive, "Name: %s Size: %llu  ", filen, &filesize) != 0) {
        f = fopen(filen, "wb");//создаём двоичный файл для записи
        if (f == NULL) {
            break;
        }
        now = ftell(Archive);           //возвращает кол-во байт (от начала до конца) файла.
        fseek(Archive, start, SEEK_SET);// переносим указатель в конец файла
        start += filesize;
        while ((filesize--) > 0) {//извлекаем файл из архива
            chr = getc(Archive);
            putc(chr, f);
        }
        fseek(Archive, now, SEEK_SET);// переносим указатель в конец файла
        fclose(f);
    }

    printf("Extracted! \n");
}

void List(char *File) {
    FILE *f = fopen(File, "rb");//открываем (в двоичном виде) File
    char filen[128];
    while (1) {                //считываем файлы
        fscanf(f, "%s", filen);//"Name:" или '#'
        if (filen[0] == '#') {
            break;
        }
        fscanf(f, "%s", filen);//имя файла, содержащегося в архиве
        printf("%s ", filen);
        fscanf(f, "%s", filen);//"Size:"
        fscanf(f, "%s", filen);//размер файла, содержащегося в архиве
    }
    fclose(f);
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *Name;// указатель на имя файла
    printf("\n");

    for (int i = 0; i < argc; i++) {
        //--file FILE
        if (argv[i][2] == 'f') {
            Name = argv[i + 1];//записываем в указатель имя файла
        }
        //--create
        if (argv[i][2] == 'c') {
            Create(Name, argc, argv);
        }
        //--extract
        if (argv[i][2] == 'e') {
            Extract(Name);
        }
        //--list
        if (argv[i][2] == 'l') {
            List(Name);
        }
    }

    return 0;
}
