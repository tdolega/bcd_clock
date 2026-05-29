#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
out="$repo_root/everything.md"

: > "$out"
printf "# Repo snapshot\n\n" >> "$out"

git -C "$repo_root" ls-files -z -- \
  '*.ino' '*.h' '*.hpp' '*.c' '*.cpp' '*.cc' '*.cxx' '*.S' '*.s' '*.asm' '*.sh' \
  'README.md' |
  sort -z |
  while IFS= read -r -d '' file; do
    lang="text"
    case "$file" in
      *.ino|*.h|*.hpp|*.c|*.cpp|*.cc|*.cxx) lang="cpp" ;;
      *.S|*.s|*.asm) lang="asm" ;;
      *.sh) lang="sh" ;;
      *.md) lang="md" ;;
    esac
    printf "## %s\n\n" "$file" >> "$out"
    printf '```%s\n' "$lang" >> "$out"
    cat "$repo_root/$file" >> "$out"
    printf '\n```\n\n' >> "$out"
  done
