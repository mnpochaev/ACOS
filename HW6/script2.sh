#!/bin/bash
# счетчик

i=1
while [ $i -le 3 ]; do
    echo "шаг $i"
    sleep 1
    i=$((i+1))
done

echo "все"