#!/bin/bash

script_dir=$(dirname "$(realpath "$0")")
bpmnos=$(dirname "$script_dir")/bin/bpmnos

# Function to display usage information
usage() {
    echo "Usage: $(basename "$0") -m <model> -d <data> [-p <path>] <output>"
    exit 1
}

# Parse command line arguments
while getopts "m:d:p:" opt; do
    case "${opt}" in
        m)
            model=${OPTARG}
            ;;
        d)
            data=${OPTARG}
            ;;
        p)
            path=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done

# Shift off the options and optional -- if present
shift $((OPTIND-1))

# Check if required arguments are provided
if [ -z "${model}" ] || [ -z "${data}" ]; then
    usage
fi

# Get the output parameter
if [ -z "$1" ]; then
    usage
fi
output=$1

echo "Profiling (this may take a while)..."
if [ -z "${path}" ]; then
  $bpmnos -m ${model} -d ${data}
else
  $bpmnos -m ${model} -d ${data} -p ${path}
fi

gprof $bpmnos gmon.out | gprof2dot -w -n 10 > ${output}.dot
echo "Created: ${output}.dot"
dot -Tsvg ${output}.dot > ${output}.svg
echo "Created: ${output}.svg"
# Cleanup
rm gmon.out

