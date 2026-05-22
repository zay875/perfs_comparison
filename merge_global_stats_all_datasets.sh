#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
binary_path="${script_dir}/merged_global_stats"
source_file="${script_dir}/merged_global_stats.cpp"
hexaly_root="${script_dir}/output_hexaly_files_logs"
mathopt_root="${script_dir}/output_mathopt_files_logs"
period="2025_04"
stats_file="mean_agg_res_stats.csv"
output_dir="${script_dir}/merged_stats"
single_output=""
auto_build=true
dataset_start=1
dataset_end=8

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Runs compare_stats on matching Hexaly/MathOpt CSV stats for datasets.

Options:
    --binary PATH         Path to compare_stats binary
                        Default: ${binary_path}
    --hexaly-root PATH    Root path for Hexaly outputs
                                                Default: ${hexaly_root}
    --mathopt-root PATH   Root path for MathOpt outputs
                                                Default: ${mathopt_root}
    --period NAME         Period subfolder under dataset id
                                                Default: ${period}
    --stats-file NAME     Stats CSV filename inside stats folder
                                                Default: ${stats_file}
    --output-dir PATH     Folder for merged CSV outputs
                                                Default: ${output_dir}
    --single-output NAME  Merge all datasets into one output CSV filename
                        (written under --output-dir)
    --dataset-start N     First dataset id (inclusive)
                                                Default: ${dataset_start}
    --dataset-end N       Last dataset id (inclusive)
                                                Default: ${dataset_end}
    --no-build            Do not auto-build compare_stats.cpp
  -h, --help            Show this help

Examples:
    $(basename "$0")
    $(basename "$0") --period 2025_05 --output-dir ./merged_csv
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --binary)
            binary_path="$2"
            shift 2
            ;;
        --hexaly-root)
            hexaly_root="$2"
            shift 2
            ;;
        --mathopt-root)
            mathopt_root="$2"
            shift 2
            ;;
        --period)
            period="$2"
            shift 2
            ;;
        --stats-file)
            stats_file="$2"
            shift 2
            ;;
        --dataset-start)
            dataset_start="$2"
            shift 2
            ;;
        --dataset-end)
            dataset_end="$2"
            shift 2
            ;;
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --single-output)
            single_output="$2"
            shift 2
            ;;
        --no-build)
            auto_build=false
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ "${auto_build}" == true ]]; then
    if [[ ! -f "${source_file}" ]]; then
        echo "Source file not found: ${source_file}" >&2
        exit 1
    fi
    echo "[INFO] Building compare_stats from ${source_file}"
    g++ "${source_file}" -o "${binary_path}"
fi

if [[ ! -x "${binary_path}" ]]; then
    echo "compare_stats binary not found or not executable: ${binary_path}" >&2
    echo "Pass --binary with a valid executable or remove --no-build." >&2
    exit 1
fi

mkdir -p "${output_dir}"

single_output_path=""
if [[ -n "${single_output}" ]]; then
    single_output_path="${output_dir}/${single_output}"
    : > "${single_output_path}"
fi

for ((dataset_id=dataset_start; dataset_id<=dataset_end; dataset_id++)); do
    hexaly_stats="${hexaly_root}/${dataset_id}/${period}/stats/${stats_file}"
    mathopt_stats="${mathopt_root}/${dataset_id}/${period}/stats/${stats_file}"
    merged_output="${output_dir}/merged_mean_agg_res_stats_stats_${dataset_id}.csv"

    if [[ -n "${single_output_path}" ]]; then
        merged_output="$(mktemp "${output_dir}/.tmp_merged_${dataset_id}_XXXXXX.csv")"
    fi

    if [[ ! -f "${hexaly_stats}" ]]; then
        echo "[WARN] Missing Hexaly stats file, skipping dataset ${dataset_id}: ${hexaly_stats}" >&2
        continue
    fi
    if [[ ! -f "${mathopt_stats}" ]]; then
        echo "[WARN] Missing MathOpt stats file, skipping dataset ${dataset_id}: ${mathopt_stats}" >&2
        continue
    fi

    echo "[INFO] Running dataset ${dataset_id}"
    echo "       Hexaly: ${hexaly_stats}"
    echo "       MathOpt: ${mathopt_stats}"
    echo "       Output: ${merged_output}"

    "${binary_path}" "${hexaly_stats}" "${mathopt_stats}" "${merged_output}"

    if [[ -n "${single_output_path}" ]]; then
        if [[ -s "${single_output_path}" ]]; then
            echo >> "${single_output_path}"
        fi
        echo "dataset_id,${dataset_id}" >> "${single_output_path}"
        cat "${merged_output}" >> "${single_output_path}"
        rm -f "${merged_output}"
    fi
done

if [[ -n "${single_output_path}" ]]; then
    echo "[INFO] All datasets processed. Aggregated output: ${single_output_path}"
else
    echo "[INFO] All datasets processed. Output directory: ${output_dir}"
fi