#!/bin/bash
# запуск всего

echo "запускаю..."

[ -f "script1.sh" ] && ./script1.sh
[ -f "script2.sh" ] && ./script2.sh
[ -f "script3.sh" ] && ./script3.sh

echo "все запустил"