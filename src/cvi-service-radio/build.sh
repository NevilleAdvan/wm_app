#! /usr/bin/env bash

set -e

cd "$(dirname "${BASH_SOURCE[0]}")"

BUILD_PROJECT_PATH=${HOME}/build
BUILD_BASE_PATH=${BUILD_PROJECT_PATH}/script/build_base.sh

if [[ ! -f "${BUILD_BASE_PATH}" ]]; then
    echo "build_base.sh not found, will clone from gitlab"
    rm -rf "${BUILD_PROJECT_PATH}"
    git clone git@gitlab.gz.cvte.cn:ae_linux_projects/cvios/app/build.git "${BUILD_PROJECT_PATH}"
fi

# shellcheck source=/dev/null
source "${BUILD_BASE_PATH}"

main "$@"
