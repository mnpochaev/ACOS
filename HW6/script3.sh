#!/bin/bash
# калькулятор

check() {
    [[ $1 =~ ^[0-9]+$ ]] || echo "не число"
}

sum() {
    check $1
    check $2
    echo "$1+$2=$(($1+$2))"
}

sum 2 3
sum 5 1