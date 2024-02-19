#!/bin/bash

environment_directory=""

SCRIPT_DIR=$(dirname $(readlink -f $0))
WAZUH_DIR=$(realpath -s "$SCRIPT_DIR/../../../..")

while getopts "e:" opt; do
    case $opt in
        e)
            environment_directory="$OPTARG/environment"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            exit 1
            ;;
    esac
done

update_conf() {
    local serv_conf_file="$ENGINE_SRC_DIR/test/integration_tests/configuration_files/general.conf"
    cp $serv_conf_file $ENVIRONMENT_DIR/engine
    sed -i "s|github_workspace|$ENVIRONMENT_DIR|g" "$ENVIRONMENT_DIR/engine/general.conf"
}

create_dummy_integration() {
    local wazuh_core_test="$ENVIRONMENT_DIR/engine/wazuh-core-test"
    mkdir -p "$wazuh_core_test/decoders" "$wazuh_core_test/filters"
    cat <<- EOM > "$wazuh_core_test/decoders/test-message.yml"
name: decoder/test-message/0
check: \$wazuh.queue == 49 # "1"
EOM

    echo "name: filter/allow-all/0" > "$wazuh_core_test/filters/allow-all.yml"
    cat <<- EOM > "$wazuh_core_test/manifest.yml"
name: integration/wazuh-core-test/0
decoders:
- decoder/test-message/0
EOM
}

create_other_dummy_integration() {
    local other_wazuh_core_test="$ENVIRONMENT_DIR/engine/other-wazuh-core-test"
    mkdir -p "$other_wazuh_core_test/decoders" "$other_wazuh_core_test/filters"
    cat <<- EOM > "$other_wazuh_core_test/decoders/other-test-message.yml"
name: decoder/other-test-message/0
check: \$wazuh.queue == 50 # "2"
EOM


    echo "name: filter/allow-all/0" > "$other_wazuh_core_test/filters/allow-all.yml"
    cat <<- EOM > "$other_wazuh_core_test/manifest.yml"
name: integration/other-wazuh-core-test/0
decoders:
- decoder/other-test-message/0
EOM
}

create_dummy_integration_with_parents() {
    local parent_wazuh_core_tes="$ENVIRONMENT_DIR/engine/parent-wazuh-core-test"
    mkdir -p "$parent_wazuh_core_tes/decoders" "$parent_wazuh_core_tes/filters"
    cat <<- EOM > "$parent_wazuh_core_tes/decoders/parent-message.yml"
name: decoder/parent-message/0
check: \$wazuh.queue == 49 # "1"
EOM

    cat <<- EOM > "$parent_wazuh_core_tes/decoders/test-message.yml"
name: decoder/test-message/0
parents:
  - decoder/parent-message/0
EOM
}

main() {
    if [ -z "$environment_directory" ]; then
        echo "environment_directory is optional. For default is wazuh directory. Usage: $0 -e <environment_directory>"
    fi

    ENGINE_SRC_DIR="$WAZUH_DIR/src/engine"
    ENVIRONMENT_DIR="${environment_directory:-$WAZUH_DIR/environment}"
    ENVIRONMENT_DIR=$(echo "$ENVIRONMENT_DIR" | sed 's|//|/|g')

    update_conf

    create_dummy_integration
    create_other_dummy_integration
    create_dummy_integration_with_parents
}
main "$@"