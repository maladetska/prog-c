#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// каждый фрейм содержит метаданные
struct metadata_item {
    unsigned char encoding;
    unsigned char name[5];
    unsigned char *value;
    int size;
};

// вся метаинформация
struct metadata {
    struct metadata_item item[84];// ID3v2 определяет 84 типа фреймов.
    int count;                    // кол-во инфы
};

// структура заголовка ID3v2
struct id3header {             // *поле* - *смещение, байт* - *длина, байт*
    unsigned char signature[3];// сигнатура - 0 - 3
    unsigned char version;     // версия - 3 - 2
    unsigned char subversion;  // субверсия
    unsigned char flags;       // флаги - 5 - 1
    unsigned char size[4];     // размер - 6 - 4
};

// структура заголовка фрейма ID3v2 (тег состоит из нескольких фреймов)
struct id3tag {            // *поле* - *смещение, байт* - *длина, байт*
    unsigned char id[4];   // идентификатор (тип фрейма) - 0 - 4
    unsigned char size[4]; // размер - 3 - 4
    unsigned char flags[2];// флаги - 5 - 2
};

// получаем размер
int GET_SIZE(unsigned char size[4], int bits) {
    return (int) (size[3] + (size[2] << bits) + (size[1] << (bits * 2)) + (size[0] << (bits * 3)));
}

// вывод содержания заголовка
void HEADER_OUTPUT(struct id3header *header) {
    printf("Signature: %c%c%c\n", header->signature[0], header->signature[1], header->signature[2]);
    printf("Version: %d\n", header->version);
    printf("Subversion: %d\n", header->subversion);
    printf("Flags: a = %d, b = %d, c = %d\n", (header->flags >> 7) && 1, (header->flags >> 6) && 1, (header->flags >> 5) && 1);
    printf("Size: %d\n\n", GET_SIZE(header->size, 7));
}

// чтение метаинформации
void READ_METADATA(struct id3header *header, struct metadata *data, FILE *file) {
    struct id3tag tag;
    data->count = 0;// кол-во заполненных фреймов
    int flag = 1;   // флаг

    while (flag == 1) {
        struct metadata_item *item = &(data->item[data->count]);
        fread(&tag, sizeof(struct id3tag), 1, file);// считываем файл в tag
        item->size = GET_SIZE(tag.size, 7);         // присваем размер
        strncpy(item->name, tag.id, 4);             // записываем название фрейма в метаинфу
        *(item->name + 4) = '\0';
        if (item->size != 0) {                              // пока элементы не закончатся
            item->value = malloc(item->size);               // выделяем память для значения фрейма
            if (item->value) {                              // если фрейм существует
                fread(&(item->encoding), 1, 1, file);       // кодируем байт в байт
                fread(item->value, item->size - 1, 1, file);// считываем значение фрейма из файла
                *(item->value + item->size - 1) = '\0';
                data->count++;// считаем кол-во заполненных фреймов
            } else {
                free(item);//чистим заимствованную память
            }
        } else {
            flag = 0;//обнуляем, если обрабоnали все метаданные
        }
    }
}

// вывод метаинформации файла
void METADATA_OUTPUT(struct metadata *data) {
    for (int i = 0; i < data->count; i++) {
        struct metadata_item *item = &(data->item[i]);
        printf("%s: %s\n", item->name, item->value);//Вывод названия фрейма и его значения
    }
}

// задаём значение
void SET_VALUE(char *name, char *value, struct metadata *data) {
    for (int i = 0; i < data->count + 1; i++) {
        struct metadata_item *item = &(data->item[i]);
        if (strcmp(item->name, name) == 0) {            // если название фрейма совпадает с заданным
            free(item->value);                          // очищаем начальное значение
            item->size = strlen(value) + 1;             // меняем размер для нового значения
            item->value = malloc(item->size);           // выделяем память для значения
            strncpy(item->value, value, item->size - 1);// записываем новое значение в существующий фрейм
            *(item->value + item->size - 1) = '\0';
            return;
        }
    }
}

// сохранение
void SAVE_METADATA(struct id3header *header, struct metadata *data, char *filepath) {
    struct id3tag tag;
    FILE *file;
    int total_size = 0;

    for (int i = 0; i < data->count; i++) {
        struct metadata_item *item = &(data->item[i]);
        total_size += 10 + item->size;
    }

    total_size = 0;
    file = fopen(filepath, "r+b");
    fwrite(header, sizeof(struct id3header), 1, file);

    for (int i = 0; i < data->count; i++) {
        struct metadata_item *item = &(data->item[i]);// создаём структуру
        strncpy(tag.id, item->name, 4);               // копируем название фрейма в tag.id
        int size = item->size;                        // инициализируем размер

        // работаем с размером
        for (int j = 3; j >= 0; j--) {
            tag.size[j] = size & ((1 << 7) - 1);
            size = size >> 7;
        }

        fwrite(&tag, sizeof(struct id3tag), 1, file);// записываем в файл содержимое tag
        fwrite(&(item->encoding), 1, 1, file);       // записываем в файл содержимое item->encoding
        fwrite(item->value, item->size - 1, 1, file);// записываем в файл содержимое item->value
        total_size += 10 + item->size;
    }

    int left_size = GET_SIZE(header->size, 7) - total_size;
    for (int i = 0; i < left_size; i++) {
        unsigned char ch = 0;
        fwrite(&ch, 1, 1, file);// записываем в файл содержимое сh
    }

    fclose(file);// заркываем файл
    printf("Done!\n");
}

// вывод запрошенного фрейма
void TAG_OUTPUT(char *name, struct metadata *data) {
    for (int i = 0; i < data->count; i++) {
        struct metadata_item *item = &(data->item[i]);  // задаём очередную структуру
        if (strcmp(item->name, name) == 0) {            //если название фрейма совпадает с заданным,..
            printf("%s: %s\n", item->name, item->value);// то его значение выводится
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    FILE *MP3file;          // наш mp3-файл
    struct id3header header;// заголовок
    struct metadata data;   // данные

    char *filepath = 0;//путь к файлу
    char *tag = 0;     // тег
    char *value = 0;   // значение

    //флаги команд
    enum {
        show = 1,
        get = 2,
        set = 3,
    };
    int com = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][2] == 'f') {
            //	if (i + 1 < argc)
            filepath = argv[i] + 11;
        } else if (argv[i][2] == 's' && argv[i][3] == 'h' && com == 0) {
            com = show;
        } else if (argv[i][2] == 'g' && com == 0) {
            com = get;
            tag = argv[i] + 6;
        } else if (argv[i][2] == 's' && argv[i][3] == 'e' && com == 0) {
            com = set;
            tag = argv[i] + 6;
        } else if (argv[i][2] == 'v' && (com == 0 || set)) {
            value = argv[i] + 8;
        }
    }

    // путь по файлу
    if (filepath) {
        MP3file = fopen(filepath, "rb");                     // открываем файл
        fread(&header, sizeof(struct id3header), 1, MP3file);// считываем заголовок
        READ_METADATA(&header, &data, MP3file);              // считываем метаинформацию
    }

    // выполняем требуемые действия (show, get or set)
    if (com == show) {
        HEADER_OUTPUT(&header);// вывод заголовка
        METADATA_OUTPUT(&data);// вывод всей имеющейся метаинформации
    } else if (com == get && tag) {
        TAG_OUTPUT(tag, &data);// вывод запрошенного фрейма
    } else if (com == set && tag && value) {
        SET_VALUE(tag, value, &data);           //
        SAVE_METADATA(&header, &data, filepath);// сохраняем новую (введённую) метаинформацию
    }

    fclose(MP3file);//заркываем файл
}
