#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

HOOK_DIR="$(git rev-parse --git-path hooks)"
mkdir -p "$HOOK_DIR"

cat >"$HOOK_DIR/pre-commit" <<'HOOK'
#!/usr/bin/env bash

set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

mapfile -t staged < <(git diff --cached --name-only --diff-filter=ACMR)
((${#staged[@]})) || exit 0

dirty=()
for file in "${staged[@]}"; do
    if ! git diff --quiet -- "$file"; then
        dirty+=("$file")
    fi
done

if ((${#dirty[@]})); then
    echo "arc pre-commit: staged files also have unstaged edits; format them manually first:" >&2
    printf '  %s\n' "${dirty[@]}" >&2
    exit 1
fi

./tools/format.sh -- "${staged[@]}"
git add -- "${staged[@]}"
HOOK

cat >"$HOOK_DIR/pre-push" <<'HOOK'
#!/usr/bin/env bash

set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

./tools/format.sh --check
./tools/check-repo.sh
HOOK

chmod +x "$HOOK_DIR/pre-commit" "$HOOK_DIR/pre-push"

echo "installed arc git hooks in $HOOK_DIR"
