#!/bin/sh

set -eu

APP_DIR=$1
MODE=$2
RUNS=${BENCH_RUNS:-5}
RESULTS_DIR="$APP_DIR/results"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
RUN_DIR="$RESULTS_DIR/$TIMESTAMP"

[ "$RUNS" -gt 0 ] || { echo "BENCH_RUNS must be positive" >&2; exit 1; }
case "$MODE" in host|local|intercept|all) ;; *) echo "usage: $0 APP_DIR {host|local|intercept|all}" >&2; exit 1 ;; esac
mkdir -p "$RUN_DIR"

metadata()
{
	{
		echo "timestamp=$TIMESTAMP"
		echo "runs=$RUNS"
		echo "host=$(hostname)"
		echo "uname=$(uname -a)"
		echo "cpu=$(awk -F: '/model name/ {print $2; exit}' /proc/cpuinfo 2>/dev/null | sed 's/^ *//')"
		echo "qemu=$(qemu-system-x86_64 --version 2>/dev/null | head -n 1 || echo unknown)"
		echo "config_sha256=$(sha256sum "$APP_DIR/.config" 2>/dev/null | awk '{print $1}' || echo unknown)"
		echo "git_rev=$(git -C "$APP_DIR/.." rev-parse HEAD 2>/dev/null || echo unknown)"
		echo "date=$(date --iso-8601=seconds 2>/dev/null || date)"
	} >"$RUN_DIR/metadata"
}

runner()
{
	case "$1" in
		host) echo run-host.sh ;;
		local) echo run-guest-local.sh ;;
		intercept) echo run-guest-intercept.sh ;;
		*) return 1 ;;
	esac
}

extract()
{
	run=$1
	mode=$2
	log=$3
	awk -v run="$run" -v mode="$mode" '
	BEGIN { print "run,mode,name,iterations,total_ns,avg_ns,p50_ns,p95_ns,min_ns,max_ns,ops_per_sec,mib_per_sec" }
	$1 == "BENCH" { count++; delete kv; for (i = 2; i <= NF; i++) { split($i, p, "="); kv[p[1]] = p[2] } printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", run,mode,kv["name"],kv["iterations"],kv["total_ns"],kv["avg_ns"],kv["p50_ns"],kv["p95_ns"],kv["min_ns"],kv["max_ns"],kv["ops_per_sec"],kv["mib_per_sec"] }
	END { if (count != 1) exit 1 }
	' "$log"
}

metadata

capture_mode()
{
	mode=$1
	runner=$(runner "$mode")
	csv="$RUN_DIR/$mode.csv"
	first=1
	for run in $(seq 1 "$RUNS"); do
		log="$RUN_DIR/$mode-$run.log"
		echo "capturing mode=$mode run=$run/$RUNS" >&2
		(
			cd "$APP_DIR"
			"./$runner"
		) >"$log" 2>&1
		if [ "$first" -eq 1 ]; then
			extract "$run" "$mode" "$log" >"$csv"
			first=0
		else
			extract "$run" "$mode" "$log" | tail -n +2 >>"$csv"
		fi
	done
}

case "$MODE" in
	all)
		for mode in host local intercept; do capture_mode "$mode"; done
		head -n 1 "$RUN_DIR/host.csv" >"$RUN_DIR/all.csv"
		for mode in host local intercept; do tail -n +2 "$RUN_DIR/$mode.csv" >>"$RUN_DIR/all.csv"; done
		;;
	*) capture_mode "$MODE"; ;;
esac

echo "results=$RUN_DIR" >&2
