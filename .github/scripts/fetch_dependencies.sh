#!/usr/bin/env bash

# shellcheck disable=SC2046

cache_dir="${HOME}/apt-cache"

set -x

mkdir -p "$cache_dir"
sudo apt -o Dir::Cache::Archives="$cache_dir" update >/dev/null 2>&1
sudo apt -o Dir::Cache::Archives="$cache_dir" install -y clang $(cat apt-deps.txt) >/dev/null 2>&1

sudo rm -rf "${cache_dir}/partial" "${cache_dir}/lock"
