#!/bin/sh

ENFORCEMENT_COMMIT=7a88ce312a516cb2fe9c30cfe365ff3fa64c17b7
REMOTE_URL=git@github.com:Mbed-TLS/mbedtls.git

help () {
    cat <<EOF
Usage: $0 [OPTION]... BRANCH_NAME...
Update and force-push the given preview branches with the new code style.

You must run this from a Git worktree with a remote pointing to the official
Mbed TLS repository.
This script will fetch and push to that remote but will not modify the
Git worktree or local branches.

If something goes wrong, a Git worktree may be left in
../update-code-style-preview-*

Options:
  -c SHA        Commit that switches all.sh to code style enforcement mode
                (default: $ENFORCEMENT_COMMIT)
  -r REMOTE     Git remote name (default: autodetected from REMOTE_URL)
  -u REMOTE_URL Git remote URL (default: $REMOTE_URL)
EOF
}

set -eu

remote=

# update_branch BRANCH_NAME
update_branch () {
    worktree_name="update-code-style-preview-$1-$$"
    git worktree add "../$worktree_name" "$remote/$1"
    cd "../$worktree_name"
    # Hide diffs, keep errors (and uncrustify's progress output)
    ./scripts/code_style.py --fix >/dev/null
    git commit -a --signoff -m 'Switch to the new code style'
    git cherry-pick "$ENFORCEMENT_COMMIT"
    git push --force-with-lease "$REMOTE_URL" "HEAD:refs/heads/features/new-code-style/$1"
    cd "$OLDPWD"
    git worktree remove "../$worktree_name"
}

find_remote () {
    if ! root=$(git rev-parse --show-toplevel); then
        echo >&2 "Fatal: not a git worktree?"
        echo >&2 "Please run this script from a Git checkout of mbedtls."
        exit 2
    fi
    cd "$root"

    if [ -z "$remote" ]; then
        remote=$(git remote -v |
                     awk -v REMOTE_URL="$REMOTE_URL" '$2 == REMOTE_URL && $3 == "(push)" {print $1; exit}')
        if [ -z "$remote" ]; then
            echo >&2 "Fatal: o pushable Git remote found for $REMOTE_URL"
            echo >&2 "Please run this script from a Git checkout of mbedtls."
            exit 2
        fi
    fi
    git fetch "$remote"

    if [ "$(git cat-file -t "$ENFORCEMENT_COMMIT")" != "commit" ]; then
        echo >&2 "Fatal: code style enforcement commit not found."
        exit 2
    fi
}

if [ "${1-}" = "--help" ]; then
    help
    exit
fi
while getopts c:r:u: OPTLET; do
    case $OPTLET in
        c) ENFORCEMENT_COMMIT=$OPTARG;;
        r) remote=$OPTARG;;
        u) REMOTE_URL=$OPTARG;;
        \?) help >&2; exit 3;;
    esac
done
shift $((OPTIND - 1))

find_remote

if [ $# -eq 0 ]; then
    echo "$0: worktree OK. No branches specified."
    exit
fi

for branch in "$@"; do
    update_branch "$branch"
done
