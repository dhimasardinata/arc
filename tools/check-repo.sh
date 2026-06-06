#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

die() {
    echo "arc repo check failed: $*" >&2
    exit 1
}

git_files() {
    git ls-files "$@"
}

worktree_files() {
    git ls-files --cached --others --exclude-standard "$@"
}

grep_files() {
    local pattern="$1"
    shift
    local files=("$@")
    [[ ${#files[@]} -gt 0 ]] || return 1
    grep -nIE -- "$pattern" "${files[@]}"
}

load_arc_projects() {
    local target_var="$1"
    shift
    local output
    if ! output="$(./tools/arc-projects.py "$@")"; then
        die "tools/arc-projects.py failed"
    fi

    local -n projects_ref="$target_var"
    projects_ref=()
    if [[ -n "$output" ]]; then
        mapfile -t projects_ref <<< "$output"
    fi
}

workflow_line() {
    local needle="$1"
    awk -v needle="$needle" 'index($0, needle) { print NR; exit }' .github/workflows/build.yml
}

workflow_step_before() {
    local earlier="$1"
    local later="$2"
    local message="$3"
    local earlier_line
    local later_line
    earlier_line="$(workflow_line "$earlier")"
    later_line="$(workflow_line "$later")"
    if [[ -z "$earlier_line" || -z "$later_line" || "$earlier_line" -ge "$later_line" ]]; then
        die "$message"
    fi
}

git diff --check >/dev/null 2>&1 || die "working tree diff contains whitespace errors"
git diff --check --cached >/dev/null 2>&1 || die "staged diff contains whitespace errors"
mapfile -t whitespace_files < <(worktree_files)
if grep_files '[[:blank:]]$' "${whitespace_files[@]}" >/dev/null 2>&1; then
    grep_files '[[:blank:]]$' "${whitespace_files[@]}" || true
    die "tracked or untracked non-ignored files contain trailing whitespace"
fi

./tools/format.sh --check || die "format check failed; run ./tools/format.sh"
./tools/tool-tests.sh || die "tool tests failed"
./tools/profile-manifest-check.py || die "profile manifest check failed"
./tools/topology-check.py --quiet || die "topology check failed"
python3 tools/compile-fail-check.py || die "compile-fail contract check failed"
./tools/arc-prove.sh || die "formal spec check failed"
./tools/use-after-move-check.sh || die "use-after-move check failed"
./tools/safety-case-check.py || die "safety-case evidence check failed"
./tools/firmware-manifest.py --format json >/dev/null || die "firmware artifact manifest check failed"
./tools/evidence-index.py --format json \
    --require THIRD_PARTY_MANIFEST.json \
    THIRD_PARTY_MANIFEST.json >/dev/null || die "evidence index check failed"
./tools/source-manifest.py --format json >/dev/null || die "source manifest check failed"
./tools/third-party-manifest-check.py || die "third-party manifest check failed"
python3 tools/release-evidence.py --format json >/dev/null || die "release evidence manifest failed"
./tools/workflow-pins-check.py --format json >/dev/null || die "workflow action pin check failed"
./tools/workflow-policy-check.py --format json >/dev/null || die "workflow policy check failed"
./tools/evidence-workflow-check.py --format json >/dev/null || die "evidence workflow contract check failed"
./tools/npm-lock-check.py --format json >/dev/null || die "npm lockfile evidence check failed"
./tools/license-policy-check.py --format json >/dev/null || die "license policy evidence check failed"
./tools/secret-scan-check.py --format json >/dev/null || die "secret scan evidence check failed"
./tools/sbom.py --format json >/dev/null || die "SBOM generation failed"
./tools/provenance.py --format json THIRD_PARTY_MANIFEST.json >/dev/null || die "provenance generation failed"
go run tools/arc-audit.go -root . -all || die "realtime audit failed"

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "tracked sdkconfig or sdkconfig.old files are not allowed: $file"
    fi
done < <(git_files | grep -E '(^|/)sdkconfig(\.old)?$' || true)

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "sdkconfig.defaults.psram should not exist anymore: $file"
    fi
done < <(git_files | grep -E '(^|/)sdkconfig\.defaults\.psram$' || true)

if sed -n '/idf_component_register(/,/)/p' components/arc/CMakeLists.txt | grep -qw 'mbedtls'; then
    die "components/arc baseline must not require optional mbedTLS; use arc_requires(aes) or arc_requires(sha)"
fi

mapfile -t vscode_files < <(git_files .vscode || true)
for file in "${vscode_files[@]}"; do
    case "$file" in
        .vscode/settings.json|.vscode/extensions.json) ;;
        *) die ".vscode must only contain portable clangd settings: $file" ;;
    esac
done
if grep_files '/home/[^*]|/Users/[^*]|esp-[0-9]' "${vscode_files[@]}" >/dev/null 2>&1; then
    die ".vscode must not hardcode user-specific clangd toolchain paths"
fi

mapfile -t zed_files < <(git_files .zed || true)
for file in "${zed_files[@]}"; do
    case "$file" in
        .zed/settings.json) ;;
        *) die ".zed must only contain portable clangd settings: $file" ;;
    esac
done
if grep_files '/home/[^*]|/Users/[^*]|esp-[0-9]' "${zed_files[@]}" >/dev/null 2>&1; then
    die ".zed must not hardcode user-specific clangd toolchain paths"
fi

mapfile -t clangd_files < <(git_files | grep -E '(^|/)\.clangd$' || true)
for file in "${clangd_files[@]}"; do
    [[ "$file" == ".clangd" ]] || die "per-project .clangd files must stay generated/local: $file"
done
if grep_files 'MBEDTLS_CONFIG_FILE|TF_PSA_CRYPTO_USER_CONFIG_FILE|esp-idf/components/mbedtls|/\.espressif/|/home/|/Users/' "${clangd_files[@]}" >/dev/null 2>&1; then
    die ".clangd must not hardcode toolchain paths or optional mbedTLS include paths"
fi

mapfile -t docs < <(worktree_files README.md AGENTS.md docs .github examples)
if grep_files '^[[:space:]]*idf\.py set-target ' "${docs[@]}" >/dev/null 2>&1; then
    die "docs or workflows still tell users to run set-target; use ARC_TARGET through cmake/arc-idf.cmake"
fi

load_arc_projects project_dirs
project_cmakelists=()
for dir in "${project_dirs[@]}"; do
    if [[ "$dir" == "." ]]; then
        project_cmakelists+=("CMakeLists.txt")
    else
        project_cmakelists+=("$dir/CMakeLists.txt")
    fi
done

for file in "${project_cmakelists[@]}"; do
    if ! grep -qE 'arc-idf\.cmake' "$file"; then
        die "$file does not include cmake/arc-idf.cmake"
    fi
done

load_arc_projects buildable_dirs --buildable
for dir in "${buildable_dirs[@]}"; do
    if [[ ! -f "$dir/partitions_16mb.csv" ]]; then
        die "$dir must carry partitions_16mb.csv because root sdkconfig.defaults selects that custom table"
    fi
done

if ! grep -qE 'arc_target\(esp32s3\)' CMakeLists.txt; then
    die "root firmware CMakeLists.txt must declare arc_target(esp32s3)"
fi

load_arc_projects esp32s3_dirs --examples --target esp32s3
for dir in "${esp32s3_dirs[@]}"; do
    if ! grep -qE 'arc_target\(esp32s3\)' "$dir/CMakeLists.txt"; then
        die "$dir/CMakeLists.txt must declare arc_target(esp32s3)"
    fi
done

load_arc_projects esp32s31_dirs --examples --target esp32s31
for dir in "${esp32s31_dirs[@]}"; do
    if ! grep -qE 'arc_target\(esp32s31\)' "$dir/CMakeLists.txt"; then
        die "$dir/CMakeLists.txt must declare arc_target(esp32s31)"
    fi
done

if ! grep -qE 'set\(ARC_TARGET "esp32s3".*CACHE STRING' cmake/arc-idf.cmake; then
    die "cmake/arc-idf.cmake no longer defaults ARC_TARGET to esp32s3"
fi

if ! grep -qE 'ARC_EXPERIMENTAL_ESP32S31' cmake/arc-idf.cmake; then
    die "cmake/arc-idf.cmake no longer gates ESP32-S31 behind an experimental option"
fi

if ! grep -qE 'arc_target' cmake/arc-idf.cmake; then
    die "cmake/arc-idf.cmake must provide arc_target()"
fi

if ! grep -qF '#if __has_include("esp_tls.h") && __has_include("lwip/netdb.h") && __has_include("lwip/sockets.h")' components/arc/include/arc.hpp; then
    die "arc.hpp must only include arc/tls.hpp when both ESP-TLS and lwIP socket headers are visible"
fi

mapfile -t target_api_files < <(find components/arc/include/arc/arch components/arc/include/arc/soc -type f -name '*.hpp' | sort)
if grep_files '\b[a-z][a-z0-9]*(_[a-z0-9]+){2,}\b' "${target_api_files[@]}" >/dev/null 2>&1; then
    grep_files '\b[a-z][a-z0-9]*(_[a-z0-9]+){2,}\b' "${target_api_files[@]}" || true
    die "target/arch public API names must stay short; use at most two snake-case words"
fi
verbose_api_re='current[_]is[_]|feature[_]ready|target[_]name|arch[_]name|Core[P]lan'
if grep_files "$verbose_api_re" "${target_api_files[@]}" >/dev/null 2>&1; then
    grep_files "$verbose_api_re" "${target_api_files[@]}" || true
    die "target/arch public API regressed to verbose target names"
fi

mapfile -t arc_api_name_files < <(
    {
        find components/arc/include/arc -type f -name '*.hpp'
        printf '%s\n' README.md AGENTS.md docs/api-naming.md cmake/arc-idf.cmake
        find examples \( -path '*/build' -o -path '*/managed_components' \) -prune -o -type f \( -name '*.cpp' -o -name '*.hpp' -o -name 'CMakeLists.txt' -o -name 'README.md' \) -print
        printf '%s\n' tests/host/logic.cpp
    } | sort
)
arc_owned_long_re='to[_]device[_]strict|from[_]device[_](strict|unaligned)|discard[_](strict|unaligned)|acq[_]rel[_]fence|rise[_]delay[_]ticks|fall[_]delay[_]ticks|claim[_]proof[_]mix|secure[_]boot[_](state|revoke|digest)|cert[_]bundle[_]attach|sample[_](sram[_]decay|adc[_]noise)|derive[_]key[_]with|drop[_]espnow[_]33|gcc[_]phat[_]tdoa|range[_]from[_]sync|emit[_]chirp[_]symbol|ee[_]mac[_]s8|yuv[_](to[_])?rgb565|yuv422[_](to[_])?rgb565|\brgb565\(|is[_]power[_]of[_]two|local[_]to[_](remote|grandmaster)|remote[_]to[_]local|grandmaster[_]to[_]local|saturating[_](add|sub)|clock[_]dfs[_]possible|require[_]stable[_]source|locked[_]cycle[_]counter|copy[_]coherent[_]strict|send[_]coherent[_](strict|impl)|finish[_]coherent[_]impl|crypto[_]dma[_]submit|essential[_]from[_]flow|target[_]from[_]flow|half[_]full[_]isr|drain[_]half[_]full[_]isr|trace[_]half[_]full|arc[_]require[_]target|i2c[_]stall[_]us|tight[_]overrun[_]cycles|feed[_]mm[_]min|steps[_]per[_](mm|unit)|ticks[_]per[_]step|fat[_]ro[_]off|raise[_]if[_]ip[_]ready|find[_]or[_]empty|predict[_]if[_]stale|queue[_]coherent[_](strict|impl)|mark[_]ticket[_]done|xfer[_]coherent[_]impl|lcd[_]cam[_](start|stop)|cache[_]sensitive[_]caps|can[_]round[_]up|any[_]status[_]call|find[_]headers[_]end|effective[_]total[_]slots|keep[_]alive[_]ms|broker[_]timeout[_]ms|ticks[_]per[_]second|method[_]not[_]allowed|if[_]none[_]match|read[_]write[_]execute|at[_](most|least)[_]once|device[_]under[_]test|can[_]priority[_]spam|i2c[_]short[_]to[_]ground|spi[_]cs[_]drop'
if grep_files "$arc_owned_long_re" "${arc_api_name_files[@]}" >/dev/null 2>&1; then
    grep_files "$arc_owned_long_re" "${arc_api_name_files[@]}" || true
    die "Arc-owned public names must stay short; use at most two snake-case words"
fi
arc_soc_old_re='Soc::(flash[_]xts|flash[_]xts[_]block|async[_]memcpy|ahb[_]gdma|lcdcam[_]dvp|rtc[_]gpio[_](io|hold|wake)|spi[_](peripherals|max[_]cs|fifo[_]bytes)|ds[_](iv[_]bytes|key[_]us)|uart[_]max[_]bitrate|touch[_]max[_]channel)|static constexpr (bool|std::uint32_t) (flash[_]xts|flash[_]xts[_]block|async[_]memcpy|ahb[_]gdma|lcdcam[_]dvp|rtc[_]gpio[_](io|hold|wake)|spi[_](peripherals|max[_]cs|fifo[_]bytes)|ds[_](iv[_]bytes|key[_]us)|uart[_]max[_]bitrate|touch[_]max[_]channel)\b'
if grep_files "$arc_soc_old_re" components/arc/include/arc/soc.hpp components/arc/include/arc/xts.hpp README.md tests/host/logic.cpp >/dev/null 2>&1; then
    grep_files "$arc_soc_old_re" components/arc/include/arc/soc.hpp components/arc/include/arc/xts.hpp README.md tests/host/logic.cpp || true
    die "Arc-owned SoC wrapper names must stay short even when they mirror SDK capabilities"
fi

if ! grep -qE 'ARC_TARGET:-esp32s3|arc_target.*esp32s3' env.sh env.fish; then
    die "env loaders no longer default IDF_TARGET to esp32s3"
fi

if ! grep -qE '\./tools/ci-build-plan\.py --buildable' .github/workflows/build.yml; then
    die "build workflow must plan changed firmware projects through tools/ci-build-plan.py"
fi

if [[ ! -f SECURITY.md ]]; then
    die "root SECURITY.md must define the GitHub-facing disclosure policy"
fi

if ! grep -qF 'docs/security.md' SECURITY.md || ! grep -qF 'private vulnerability reporting' SECURITY.md; then
    die "SECURITY.md must point to detailed security automation docs and private reporting"
fi

if [[ ! -f CONTRIBUTING.md ]]; then
    die "CONTRIBUTING.md must define the repository contribution path"
fi

if ! grep -qF './tools/check-repo.sh' CONTRIBUTING.md \
    || ! grep -qF 'docs/api-naming.md' CONTRIBUTING.md \
    || ! grep -qF 'SECURITY.md' CONTRIBUTING.md \
    || ! grep -qF 'ARC_EXPERIMENTAL_ESP32S31=ON' CONTRIBUTING.md; then
    die "CONTRIBUTING.md must cover validation, API naming, security, and experimental target policy"
fi

if [[ ! -f RELEASE.md ]]; then
    die "RELEASE.md must define release evidence requirements"
fi

if ! grep -qF './tools/check-repo.sh' RELEASE.md \
    || ! grep -qF './tools/host-tests.sh' RELEASE.md \
    || ! grep -qF './tools/firmware-manifest.py --format json --require-artifacts' RELEASE.md \
    || ! grep -qF './tools/evidence-index.py --format json' RELEASE.md \
    || ! grep -qF './tools/evidence-bundle-check.py .arc-artifacts' RELEASE.md \
    || ! grep -qF './tools/evidence-workflow-check.py --format json' RELEASE.md \
    || ! grep -qF './tools/license-policy-check.py --format json' RELEASE.md \
    || ! grep -qF './tools/sbom.py --format json' RELEASE.md \
    || ! grep -qF './tools/provenance.py --format json' RELEASE.md \
    || ! grep -qF './tools/release-evidence.py --format json --require-clean' RELEASE.md \
    || ! grep -qF './tools/source-manifest.py --format json --require-clean' RELEASE.md \
    || ! grep -qF 'idf.py build' RELEASE.md \
    || ! grep -qF 'docs/security.md' RELEASE.md \
    || ! grep -qF 'docs/licensing.md' RELEASE.md \
    || ! grep -qF 'THIRD_PARTY_NOTICES.md' RELEASE.md; then
    die "RELEASE.md must cover repository gates, firmware evidence, security, and licensing"
fi

if [[ ! -f THIRD_PARTY_NOTICES.md ]]; then
    die "THIRD_PARTY_NOTICES.md must define third-party notice handling"
fi

if ! grep -qF 'ESP-IDF' THIRD_PARTY_NOTICES.md \
    || ! grep -qF 'Arduino-ESP32' THIRD_PARTY_NOTICES.md \
    || ! grep -qF 'VitePress' THIRD_PARTY_NOTICES.md \
    || ! grep -qF 'Product Release Rule' THIRD_PARTY_NOTICES.md; then
    die "THIRD_PARTY_NOTICES.md must cover firmware, docs, and product notice rules"
fi

if [[ ! -f THIRD_PARTY_MANIFEST.json ]]; then
    die "THIRD_PARTY_MANIFEST.json must define machine-checkable dependency notice boundaries"
fi

if ! grep -qF 'ESP-IDF' THIRD_PARTY_MANIFEST.json \
    || ! grep -qF 'Arduino-ESP32' THIRD_PARTY_MANIFEST.json \
    || ! grep -qF 'VitePress and npm dependencies' THIRD_PARTY_MANIFEST.json \
    || ! grep -qF 'notice_required_when' THIRD_PARTY_MANIFEST.json; then
    die "THIRD_PARTY_MANIFEST.json must cover firmware, docs, and notice trigger metadata"
fi

if [[ ! -f docs/governance.md ]]; then
    die "docs/governance.md must expose repository governance controls"
fi

if ! grep -qF 'CONTRIBUTING.md' docs/governance.md \
    || ! grep -qF 'RELEASE.md' docs/governance.md \
    || ! grep -qF 'SECURITY.md' docs/governance.md \
    || ! grep -qF 'THIRD_PARTY_NOTICES.md' docs/governance.md \
    || ! grep -qF 'THIRD_PARTY_MANIFEST.json' docs/governance.md \
    || ! grep -qF '.github/CODEOWNERS' docs/governance.md \
    || ! grep -qF 'tools/source-manifest.py --format json --require-clean' docs/governance.md \
    || ! grep -qF 'tools/evidence-index.py --format json' docs/governance.md \
    || ! grep -qF 'tools/evidence-bundle-check.py .arc-artifacts' docs/governance.md \
    || ! grep -qF 'tools/evidence-workflow-check.py --format json' docs/governance.md \
    || ! grep -qF 'tools/license-policy-check.py --format json' docs/governance.md \
    || ! grep -qF 'tools/sbom.py --format json' docs/governance.md \
    || ! grep -qF 'tools/provenance.py --format json' docs/governance.md \
    || ! grep -qF 'tools/release-evidence.py --format json --require-clean' docs/governance.md; then
    die "docs/governance.md must link contribution, release, security, notice, ownership, and evidence controls"
fi

if [[ ! -x tools/release-evidence.py ]]; then
    die "release evidence tool must stay executable"
fi

if [[ ! -x tools/firmware-manifest.py ]]; then
    die "firmware manifest tool must stay executable"
fi

if [[ ! -x tools/evidence-index.py ]]; then
    die "evidence index tool must stay executable"
fi

if [[ ! -x tools/evidence-bundle-check.py ]]; then
    die "evidence bundle check tool must stay executable"
fi

if [[ ! -x tools/workflow-pins-check.py ]]; then
    die "workflow action pin tool must stay executable"
fi

if [[ ! -x tools/workflow-policy-check.py ]]; then
    die "workflow policy tool must stay executable"
fi

if [[ ! -x tools/evidence-workflow-check.py ]]; then
    die "evidence workflow contract tool must stay executable"
fi

if [[ ! -x tools/npm-lock-check.py ]]; then
    die "npm lockfile evidence tool must stay executable"
fi

if [[ ! -x tools/license-policy-check.py ]]; then
    die "license policy evidence tool must stay executable"
fi

if [[ ! -x tools/secret-scan-check.py ]]; then
    die "secret scan evidence tool must stay executable"
fi

if [[ ! -x tools/source-manifest.py ]]; then
    die "source manifest tool must stay executable"
fi

if [[ ! -x tools/sbom.py ]]; then
    die "SBOM tool must stay executable"
fi

if [[ ! -x tools/provenance.py ]]; then
    die "provenance tool must stay executable"
fi

if [[ ! -x tools/third-party-manifest-check.py ]]; then
    die "third-party manifest tool must stay executable"
fi

if [[ ! -f .github/pull_request_template.md ]]; then
    die "pull request template must keep review evidence consistent"
fi

if ! grep -qF './tools/check-repo.sh' .github/pull_request_template.md \
    || ! grep -qF 'Target hardware or host-only scope' .github/pull_request_template.md \
    || ! grep -qF 'SDK, CMake, or configuration impact' .github/pull_request_template.md; then
    die "pull request template must ask for validation, hardware scope, and SDK/config impact"
fi

for issue_template in .github/ISSUE_TEMPLATE/bug_report.yml .github/ISSUE_TEMPLATE/firmware_validation.yml .github/ISSUE_TEMPLATE/config.yml; do
    [[ -f "$issue_template" ]] || die "missing GitHub issue template: $issue_template"
done

if ! grep -qF 'Arc commit' .github/ISSUE_TEMPLATE/bug_report.yml \
    || ! grep -qF 'Validation already run' .github/ISSUE_TEMPLATE/bug_report.yml \
    || ! grep -qF 'SECURITY.md' .github/ISSUE_TEMPLATE/bug_report.yml; then
    die "bug issue template must request commit, validation, and private security routing"
fi

if ! grep -qF 'ESP-IDF version and commit' .github/ISSUE_TEMPLATE/firmware_validation.yml \
    || ! grep -qF 'Hardware and configuration' .github/ISSUE_TEMPLATE/firmware_validation.yml \
    || ! grep -qF 'Logs or evidence' .github/ISSUE_TEMPLATE/firmware_validation.yml; then
    die "firmware issue template must request SDK version, hardware config, and evidence"
fi

if [[ ! -f .github/CODEOWNERS ]]; then
    die "CODEOWNERS must route critical Arc changes to the maintainer"
fi

for owned_path in '*' '/components/arc/' '/cmake/' '/examples/' '/tools/' '/.github/' '/SECURITY.md' '/docs/security.md' '/docs/safety-case.md'; do
    if ! grep -qF "$owned_path @dhimasardinata" .github/CODEOWNERS; then
        die "CODEOWNERS must cover $owned_path"
    fi
done

if ! grep -qF 'permissions:' .github/workflows/build.yml || ! grep -qF '  contents: read' .github/workflows/build.yml; then
    die "build workflow must run with least-privilege contents:read permissions"
fi

if ! grep -qF 'group: build-${{ github.ref }}' .github/workflows/build.yml; then
    die "build workflow must serialize runs per ref"
fi

if ! grep -qF "cancel-in-progress: \${{ github.event_name == 'pull_request' }}" .github/workflows/build.yml; then
    die "build workflow must cancel only superseded pull-request runs"
fi

if ! grep -qE '^[[:space:]]+timeout-minutes: 90$' .github/workflows/build.yml; then
    die "build workflow must bound the firmware job with timeout-minutes"
fi

if grep -qE 'find examples -mindepth .*CMakeLists\.txt' .github/workflows/build.yml; then
    die "build workflow must not duplicate raw example project discovery"
fi

if ! grep -qE '\./tools/host-tests\.sh' .github/workflows/build.yml; then
    die "build workflow must run host tests before firmware builds"
fi

if ! grep -qF '.arc-artifacts/' .gitignore; then
    die ".gitignore must ignore generated CI evidence artifacts"
fi

if ! grep -qE 'go run tools/arc-audit\.go -root \. -all' tools/check-repo.sh; then
    die "repo check must run the strict all-entry realtime audit"
fi

if ! grep -qE '\./tools/arc-prove\.sh' tools/check-repo.sh; then
    die "repo check must run the formal spec structural/model gate"
fi
if ! grep -qE '\./tools/topology-check\.py --quiet' tools/check-repo.sh; then
    die "repo check must run the topology source gate"
fi
if ! grep -qE 'python3 tools/compile-fail-check\.py' tools/check-repo.sh; then
    die "repo check must run the negative compile contract gate"
fi
if ! grep -qE '\./tools/use-after-move-check\.sh' tools/check-repo.sh; then
    die "repo check must run the use-after-move lint gate"
fi
if ! grep -qE '\./tools/safety-case-check\.py' tools/check-repo.sh; then
    die "repo check must run the safety-case evidence gate"
fi

if ! grep -qE '\./tools/host-bench\.sh' .github/workflows/build.yml; then
    die "build workflow must run host benchmarks before firmware builds"
fi
if ! grep -qE 'name: Repository evidence bundle' .github/workflows/build.yml; then
    die "build workflow must emit repository evidence artifacts"
fi
for evidence_file in source-manifest third-party-manifest safety-case release-evidence workflow-pins workflow-policy npm-lock license-policy secret-scan; do
    if ! grep -qF ".arc-artifacts/$evidence_file.json" .github/workflows/build.yml; then
        die "build workflow must publish $evidence_file JSON evidence"
    fi
done
if ! grep -qF '.arc-artifacts/evidence-index.json' .github/workflows/build.yml; then
    die "build workflow must publish repository evidence index"
fi
if ! grep -qE '\./tools/evidence-index\.py --format json --output \.arc-artifacts/evidence-index\.json' .github/workflows/build.yml; then
    die "build workflow must hash repository evidence files"
fi
if ! grep -qE '\./tools/evidence-bundle-check\.py \.arc-artifacts' .github/workflows/build.yml; then
    die "build workflow must verify repository evidence bundle coherence"
fi
if ! grep -qE '\./tools/evidence-workflow-check\.py --format json' tools/check-repo.sh; then
    die "repo check must validate the repository evidence workflow contract"
fi
if ! grep -qE '\./tools/workflow-pins-check\.py --format json > \.arc-artifacts/workflow-pins\.json' .github/workflows/build.yml; then
    die "build workflow must emit workflow action pin evidence"
fi
if ! grep -qE '\./tools/workflow-policy-check\.py --format json > \.arc-artifacts/workflow-policy\.json' .github/workflows/build.yml; then
    die "build workflow must emit workflow policy evidence"
fi
if ! grep -qE '\./tools/npm-lock-check\.py --format json > \.arc-artifacts/npm-lock\.json' .github/workflows/build.yml; then
    die "build workflow must emit npm lockfile evidence"
fi
if ! grep -qE '\./tools/license-policy-check\.py --format json > \.arc-artifacts/license-policy\.json' .github/workflows/build.yml; then
    die "build workflow must emit license policy evidence"
fi
if ! grep -qF -- '--require .arc-artifacts/license-policy.json' .github/workflows/build.yml; then
    die "build workflow must index license policy evidence"
fi
if ! grep -qE '\./tools/secret-scan-check\.py --format json > \.arc-artifacts/secret-scan\.json' .github/workflows/build.yml; then
    die "build workflow must emit secret scan evidence"
fi
if ! grep -qE '\./tools/sbom\.py --format json --output \.arc-artifacts/sbom\.spdx\.json' .github/workflows/build.yml; then
    die "build workflow must emit SPDX SBOM evidence"
fi
if ! grep -qF '.arc-artifacts/sbom.spdx.json' .github/workflows/build.yml; then
    die "build workflow must upload SPDX SBOM evidence"
fi
if ! grep -qF -- '--require .arc-artifacts/sbom.spdx.json' .github/workflows/build.yml; then
    die "build workflow must index SPDX SBOM evidence"
fi
if ! grep -qE '\./tools/provenance\.py --format json --output \.arc-artifacts/provenance\.intoto\.json' .github/workflows/build.yml; then
    die "build workflow must emit in-toto provenance evidence"
fi
if ! grep -qF '.arc-artifacts/provenance.intoto.json' .github/workflows/build.yml; then
    die "build workflow must upload in-toto provenance evidence"
fi
if ! grep -qF -- '--require .arc-artifacts/provenance.intoto.json' .github/workflows/build.yml; then
    die "build workflow must index in-toto provenance evidence"
fi
if ! grep -qE 'name: Upload repository evidence' .github/workflows/build.yml \
    || ! grep -qE 'name: arc-evidence' .github/workflows/build.yml \
    || ! grep -qE 'if-no-files-found: error' .github/workflows/build.yml \
    || ! grep -qE 'retention-days: 30' .github/workflows/build.yml; then
    die "build workflow must upload repository evidence with explicit retention"
fi
if ! grep -qE '\./tools/firmware-manifest\.py --format json --require-artifacts --output \.arc-artifacts/firmware-manifest\.json' .github/workflows/build.yml; then
    die "build workflow must emit a firmware artifact hash manifest"
fi
if ! grep -qF '.arc-artifacts/firmware-manifest.json' .github/workflows/build.yml; then
    die "build workflow must upload the firmware artifact manifest with binaries"
fi
if ! grep -qF '.arc-artifacts/firmware-evidence-index.json' .github/workflows/build.yml; then
    die "build workflow must upload the firmware evidence index with binaries"
fi
workflow_step_before "name: Plan firmware builds" "name: Ensure host tools" \
    "build workflow must plan changed firmware projects before host tool setup"
workflow_step_before "name: Plan firmware builds" "name: Restore ESP-IDF and tools cache" \
    "build workflow must plan changed firmware projects before ESP-IDF cache work"
workflow_step_before "name: Host benchmarks" "name: Build firmware" \
    "build workflow must run host benchmarks before firmware builds"
workflow_step_before "name: Plan firmware builds" "name: Build firmware" \
    "build workflow must plan changed firmware projects before firmware builds"
workflow_step_before "name: Repo sanity" "name: Repository evidence bundle" \
    "build workflow must generate repository evidence after repo sanity"
workflow_step_before "name: Repository evidence bundle" "name: Host tests" \
    "build workflow must publish repository evidence before host tests"
workflow_step_before "name: Repository evidence bundle" "name: Upload repository evidence" \
    "build workflow must upload generated repository evidence"
workflow_step_before "name: Build firmware" "name: Firmware artifact manifest" \
    "build workflow must hash firmware artifacts after building them"
workflow_step_before "name: Firmware artifact manifest" "name: Upload binaries" \
    "build workflow must upload firmware hashes beside binaries"
for cache_guard in 'name: Restore firmware build cache' 'name: Save firmware build cache' '\.arc-build-cache' 'cache_ready'; do
    if ! grep -qE "$cache_guard" .github/workflows/build.yml; then
        die "build workflow must cache selected firmware build directories for incremental CI"
    fi
done

if ! grep -qE 'go run tools/clangd-compile-commands\.go --validate-arc-headers' .github/workflows/build.yml; then
    die "build workflow must validate Arc header compile commands"
fi

if [[ -e components/arc_s3 || -e components/arc_s31 ]]; then
    die "target support must stay unified under components/arc"
fi

required_exec=(
    env.sh
    tools/host-tests.sh
    tools/host-bench.sh
    tools/framework-compare.sh
    tools/ensure-frameworks.sh
    tools/sync-idf.sh
    tools/dump-source.py
    tools/arc-pack-bridge.py
    tools/arc-projects.py
    tools/ci-build-plan.py
    tools/clangd-compile-commands.py
    tools/evidence-bundle-check.py
    tools/evidence-index.py
    tools/evidence-workflow-check.py
    tools/firmware-manifest.py
    tools/format.sh
    tools/hil-evidence-check.py
    tools/license-policy-check.py
    tools/npm-lock-check.py
    tools/provenance.py
    tools/secret-scan-check.py
    tools/sbom.py
    tools/source-manifest.py
    tools/third-party-manifest-check.py
    tools/topology-check.py
    tools/tool-tests.sh
    tools/use-after-move-check.sh
    tools/workflow-policy-check.py
    tools/workflow-pins-check.py
    tools/install-git-hooks.sh
)
load_arc_projects example_dirs --examples
for dir in "${example_dirs[@]}"; do
    if [[ -f "$dir/env.sh" ]]; then
        required_exec+=("$dir/env.sh")
    fi
done

for file in "${required_exec[@]}"; do
    [[ -x "$file" ]] || die "$file must stay executable"
done

echo "arc repo check: OK"
