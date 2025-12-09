#!/bin/bash
# меню

while true; do
    echo
    echo "1 - инфо"
    echo "2 - кол-во файлов" 
    echo "3 - выход"
    read -p "выбор: " c
    
    if [ "$c" = "1" ]; then
        whoami
        uname
    elif [ "$c" = "2" ]; then
        ls | wc -l
    elif [ "$c" = "3" ]; then
        echo "выход"
        break
    else
        echo "не то"
    fi
done