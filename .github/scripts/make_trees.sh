#!/usr/bin/env bash
# Create a synthetic benchmark tree for FSS to index.
#
# Portable across Linux and the Git Bash / MSYS2 shell used on Windows runners.
#
# This deliberately does NOT build the tree with one big brace-expanded
# `mkdir -p a/{1..10}/{1..10}/...` invocation. That expands to ~100k paths,
# and Windows caps a command line at 32767 characters, so the single-shot form
# works on Linux and hard-fails on Windows. Directories are therefore created in
# batches of $BATCH, and files are created with shell builtins (`: >`) which
# spawn no process at all.
#
# usage: make_trees.sh <root> <balanced|wide_deep>

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <root> <balanced|wide_deep>" >&2
    exit 2
fi

root=$1
shape=$2

case "$shape" in
    balanced|wide_deep) ;;
    *)
        echo "unknown tree shape: $shape" >&2
        exit 2
        ;;
esac

BATCH=500
batch=()

# mkdir is not a bash builtin, so a 100k-directory tree means 100k process
# spawns on Windows. Amortise them.
queue_dir() {
    batch+=("$1")
    if [ "${#batch[@]}" -ge "$BATCH" ]; then
        mkdir -p "${batch[@]}"
        batch=()
    fi
}

flush_dirs() {
    if [ "${#batch[@]}" -gt 0 ]; then
        mkdir -p "${batch[@]}"
        batch=()
    fi
}

# create_file uses the `:` builtin with a redirect: no fork, no exec.
create_file() {
    : > "$1"
}

tree="$root/$shape"
mkdir -p "$tree"

case "$shape" in
balanced)
    # 10-way branching, 5 levels deep -> 10^5 = 100,000 leaf directories,
    # 1 file per leaf. The needle sits in the very last directory.
    for a in {01..10}; do
        for b in {01..10}; do
            for c in {01..10}; do
                for d in {01..10}; do
                    prefix="$tree/$a/$b/$c/$d"
                    for e in {01..10}; do
                        queue_dir "$prefix/$e"
                    done
                done
            done
        done
    done
    flush_dirs

    for a in {01..10}; do
        for b in {01..10}; do
            for c in {01..10}; do
                for d in {01..10}; do
                    prefix="$tree/$a/$b/$c/$d"
                    for e in {01..10}; do
                        create_file "$prefix/$e/file"
                    done
                done
            done
        done
    done

    create_file "$tree/10/10/10/10/10/NEEDLE"
    ;;

wide_deep)
    # 100 x 100 directories, 10 files per leaf -> 10,000 directories and
    # 100,000 files. The needle sits in the very last leaf.
    for a in {001..100}; do
        for b in {001..100}; do
            queue_dir "$tree/$a/$b"
        done
    done
    flush_dirs

    for a in {001..100}; do
        for b in {001..100}; do
            leaf="$tree/$a/$b"
            for f in {01..10}; do
                create_file "$leaf/file_$f"
            done
        done
    done

    create_file "$tree/100/100/NEEDLE"
    ;;
esac

dirs=$(find "$tree" -type d | wc -l)
files=$(find "$tree" -type f | wc -l)
echo "tree=$shape dirs=$dirs files=$files"
find "$tree" -name NEEDLE -print
