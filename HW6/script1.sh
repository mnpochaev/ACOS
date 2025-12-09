#!/bin/bash
# проверка системы

echo "чек системы..."

if [ $(whoami) = "root" ]; then
    echo "root"
else
    echo "не root"
fi

if [ -f "/etc/passwd" ]; then
    echo "файл есть"
fi

echo "готово"