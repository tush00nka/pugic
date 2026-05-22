#!/bin/bash

# Скрипт для объединения всех .h и .c файлов в один текстовый файл
# Исключает nuklear.h из сборки
# Использование: ./collect_sources.sh [выходной_файл]

OUTPUT_FILE="${1:-sources.txt}"

# Очищаем выходной файл
> "$OUTPUT_FILE"

# Функция для добавления содержимого файла
add_file() {
    local file="$1"
    local filename=$(basename "$file")
    
    echo "*$filename*" >> "$OUTPUT_FILE"
    echo '```c' >> "$OUTPUT_FILE"
    cat "$file" >> "$OUTPUT_FILE"
    echo '```' >> "$OUTPUT_FILE"
    echo >> "$OUTPUT_FILE"
}

# Поиск и добавление всех .h файлов (исключая nuklear.h)
echo "Сбор .h файлов..."
find . -type f -name "*.h" | sort | while read -r file; do
    filename=$(basename "$file")
    if [[ "$filename" != "nuklear.h" ]]; then
        echo "  Добавлен: $file"
        add_file "$file"
    else
        echo "  Пропущен (исключён): $file"
    fi
done

# Поиск и добавление всех .c файлов
echo "Сбор .c файлов..."
find . -type f -name "*.c" | sort | while read -r file; do
    echo "  Добавлен: $file"
    add_file "$file"
done

echo "Готово! Все файлы объединены в $OUTPUT_FILE"