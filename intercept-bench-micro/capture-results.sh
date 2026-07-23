#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RESULTS_DIR="$SCRIPT_DIR/results"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
RUN_DIR="$RESULTS_DIR/$TIMESTAMP"

usage()
{
	echo "usage: $0 {host|local|intercept|all}" >&2
	exit 1
}

runner_for_mode()
{
	case "$1" in
	host)
		echo "./run-host.sh"
		;;
	local)
		echo "./run-guest-local.sh"
		;;
	intercept)
		echo "./run-guest-intercept.sh"
		;;
	*)
		return 1
		;;
	esac
}

write_metadata()
{
	mode="$1"
	meta="$2"
	git_rev="unknown"

	if git_rev_full=$(git -C "$SCRIPT_DIR/.." rev-parse HEAD 2>/dev/null); then
		git_rev="$git_rev_full"
	fi

	{
		echo "timestamp=$TIMESTAMP"
		echo "mode=$mode"
		echo "cwd=$SCRIPT_DIR"
		echo "git_rev=$git_rev"
		echo "date=$(date --iso-8601=seconds 2>/dev/null || date)"
	} >"$meta"
}

extract_csv()
{
	mode="$1"
	log="$2"
	csv="$3"

	awk -v mode="$mode" '
	BEGIN {
		print "mode,name,iterations,total_ns,avg_ns,p50_ns,p95_ns,min_ns,max_ns,ops_per_sec,mib_per_sec"
	}
	$1 == "BENCH" {
		delete kv
		for (i = 2; i <= NF; ++i) {
			split($i, pair, "=")
			kv[pair[1]] = pair[2]
		}
		printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
			mode,
			kv["name"],
			kv["iterations"],
			kv["total_ns"],
			kv["avg_ns"],
			kv["p50_ns"],
			kv["p95_ns"],
			kv["min_ns"],
			kv["max_ns"],
			kv["ops_per_sec"],
			kv["mib_per_sec"]
	}
	' "$log" >"$csv"
}

run_mode()
{
	mode="$1"
	runner=$(runner_for_mode "$mode") || usage
	log="$RUN_DIR/$mode.log"
	csv="$RUN_DIR/$mode.csv"
	meta="$RUN_DIR/$mode.meta"

	echo "capturing $mode -> $RUN_DIR" >&2
	write_metadata "$mode" "$meta"

	set +e
	(
		cd "$SCRIPT_DIR"
		"$runner"
	) >"$log" 2>&1
	rc=$?
	set -e

	extract_csv "$mode" "$log" "$csv"

	echo "saved log: $log" >&2
	echo "saved csv: $csv" >&2
	if [ "$rc" -ne 0 ]; then
		echo "runner failed for mode=$mode exit=$rc" >&2
		return "$rc"
	fi
}

[ $# -eq 1 ] || usage
mkdir -p "$RUN_DIR"

case "$1" in
all)
	combined_csv="$RUN_DIR/all.csv"
	first=1
	for mode in host local intercept; do
		run_mode "$mode"
		if [ "$first" -eq 1 ]; then
			cat "$RUN_DIR/$mode.csv" >"$combined_csv"
			first=0
		else
			tail -n +2 "$RUN_DIR/$mode.csv" >>"$combined_csv"
		fi
	done
	echo "saved combined csv: $combined_csv" >&2
	;;
host|local|intercept)
	run_mode "$1"
	;;
*)
	usage
	;;
esac
