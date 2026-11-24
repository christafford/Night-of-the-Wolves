#!/usr/bin/env bash
# Preview an encounter in an isolated copy of an earned checkpoint.
set -eu
case "${1:-}" in
    river) stage=3 ;;
    bandit) stage=4 ;;
    storm) stage=5 ;;
    *) echo "Usage: $0 {river|bandit|storm} [Unreal launch options]" >&2; exit 2 ;;
esac
shift
project_dir="$(cd -- "$(dirname -- "$0")/.." && pwd)"
launcher="$project_dir/Saved/MoonlitTrail-Linux/Linux/Night_of_the_Wolves.sh"
if [[ ! -x "$launcher" ]]; then
    echo "Package the Linux build into Saved/MoonlitTrail-Linux first." >&2
    exit 1
fi
preview_profile="$(mktemp -d "${TMPDIR:-/tmp}/wolves-preview.XXXXXXXX")"
trap 'rm -rf -- "$preview_profile"' EXIT
mkdir -p "$preview_profile/Saved/SaveGames"
cp "$project_dir/Tests/Fixtures/MoonlitTrail-CP${stage}-v1.sav" "$preview_profile/Saved/SaveGames/MoonlitTrail-v1.sav"
"$launcher" "$@" -SaveToUserDir "-UserDir=$preview_profile"
