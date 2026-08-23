#!/bin/bash

# The instrumented executable is named by the caller. It used to be derived from this script's location,
# which assumed the binary sat in bin/ beside the checkout; it is now built into the build tree.
usage() {
    echo "Usage: $(basename "$0") -x <executable> -m <model> -d <data> [-p <path>] <output>"
    exit 1
}

# Parse command line arguments
while getopts "x:m:d:p:" opt; do
    case "${opt}" in
        x)
            bpmnos=${OPTARG}
            ;;
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
if [ -z "${bpmnos}" ] || [ -z "${model}" ] || [ -z "${data}" ]; then
    usage
fi

if [ ! -x "${bpmnos}" ]; then
    echo "Not an executable: ${bpmnos}"
    exit 1
fi

# Get the output parameter
if [ -z "$1" ]; then
    usage
fi
output=$1

echo "Profiling (this may take a while)..."
{
  TIMEFORMAT="Instance executed in %Rs"
  if [ -z "${path}" ]; then
    time $bpmnos -m ${model} -d ${data} -p static -e guided
  else
    time $bpmnos -m ${model} -d ${data} -f ${path} -p static -e guided
  fi
}

gprof $bpmnos gmon.out | gprof2dot -w -n 10 > ${output}.dot
echo "Created: ${output}.dot"
dot -Tsvg ${output}.dot > ${output}.svg
echo "Created: ${output}.svg"
# Cleanup
rm -f gmon.out

