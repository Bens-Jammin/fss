#!/usr/bin/env bash
# Turn the timings FSS prints to stderr during `fss init` into a JSON blob for
# the report generator.
#
# The lines look like (note the run of spaces, and that timings go to stderr):
#   FSCrawl:                2058 ms (67%)
#
# usage: collect_timings.sh <raw-output> <benchmark> <os> <out.json>

set -euo pipefail

if [ "$#" -ne 4 ]; then
    echo "usage: $0 <raw-output> <benchmark> <os> <out.json>" >&2
    exit 2
fi

raw_file=$1
benchmark=$2
os=$3
out=$4

raw=$(cat "$raw_file")

# Grab the first run of digits following a line that starts with "Label:".
# awk rather than `grep -oP`: PCRE support is not something we can rely on in
# the grep that ships with Git Bash on the Windows runners.
field() {
    printf '%s\n' "$raw" | awk -v key="^$1:" '
        $0 ~ key {
            if (match($0, /[0-9]+/)) {
                print substr($0, RSTART, RLENGTH)
                exit
            }
        }'
}

fscrawl=$(field FSCrawl)
insert=$(field insertFileEntries)
metadata=$(field update_metadata_table)
total=$(field Total)

fscrawl=${fscrawl:-unknown}
insert=${insert:-unknown}
metadata=${metadata:-unknown}
total=${total:-unknown}

if [ "$total" = "unknown" ]; then
    echo "warning: no timings parsed from $raw_file" >&2
    printf '%s\n' "$raw" >&2
fi

jq -n \
    --arg os "$os" \
    --arg benchmark "$benchmark" \
    --arg fscrawl_ms "$fscrawl" \
    --arg insert_ms "$insert" \
    --arg metadata_ms "$metadata" \
    --arg total_ms "$total" \
    --arg output "$raw" \
    '{
        os: $os,
        benchmark: $benchmark,
        fscrawl_ms: $fscrawl_ms,
        insert_ms: $insert_ms,
        metadata_ms: $metadata_ms,
        total_ms: $total_ms,
        output: $output
    }' > "$out"

cat "$out"
