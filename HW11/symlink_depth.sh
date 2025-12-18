#!/usr/bin/env bash
set -euo pipefail

keep=0
if [[ "${1:-}" == "--keep" ]]; then
  keep=1
fi

workdir="$(mktemp -d /tmp/symlink_depth_bash.XXXXXX)"
echo "Рабочая директория: $workdir"
cd "$workdir"

: > a   # создать регулярный файл

prev="a"
max_ok=0
i=1

while true; do
  name="$(printf 'link_%03d' "$i")"
  ln -s "$prev" "$name"

  # именно "открытие файла": exec делает open()
  if exec {fd}<"$name" 2>/dev/null; then
    exec {fd}<&-
    max_ok="$i"
    prev="$name"
    i=$((i+1))
  else
    echo "ПРОВАЛ на глубине $i: открыть \"$name\" не удалось"
    echo "Максимальная глубина, при которой open() ещё успешен: $max_ok"
    break
  fi
done

if [[ "$keep" -eq 0 ]]; then
  cd /
  rm -rf "$workdir"
else
  echo "Файлы оставлены. Удалить можно так:"
  echo "  rm -rf $workdir"
fi