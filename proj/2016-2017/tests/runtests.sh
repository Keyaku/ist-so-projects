#!/bin/bash

# =========== CONSTANTS ===========
# Return values
readonly RET_success=0
readonly RET_error=1
readonly RET_usage=2
readonly RET_help=2

# Colors
readonly RCol='\033[0m'                 # Text Reset
readonly Red='\033[0;31m'               # Red, for small details
readonly Whi='\033[0;37m'               # White, for small details
readonly Yel='\033[0;33m'               # Yellow, for mid-building
readonly BGre='\033[1;32m'              # Bold Green, for successes
readonly BWhi='\033[1;37m'              # Bold White, when beginning something
readonly BRed='\033[1;31m'              # Bold Red, when an error occurred
readonly BYel='\033[1;33m'              # Bold Yellow, when building stuff
readonly UWhi='\033[4;37m'              # Underline White, for commands
readonly URed='\033[4;31m'              # Underline Red, for warnings
readonly UBlu='\033[4;34m'              # Underline Blue, for links

# Strings
readonly Note="${UWhi}Notice${Whi}:${RCol}"
readonly Warn="${BYel}Warning${Yel}:${RCol}"
readonly Err="${BRed}Error${Red}:${RCol}"

readonly ScriptName="$0"

# String Arrays
readonly usage_content=( "Usage: $(basename $ScriptName)"
"HELP:
	-h : Shows this message"
"DIRECTORIES:
	-d : Set executable directory
	-t : Set tests directory"
"OPTIONS:
	-c : Set C compiler
	-s : Activate speed testing"
)

# Files & Directories

# Various
MAKE_rule="all"

# =========== FUNCTIONS ===========
function usage {
	for i in `seq 0 ${#usage_content[@]}`; do
		echo -e "${usage_content[i]}"
	done
    exit $RET_usage
}

function get_absolute_dir {
	# $1 : directory to parse
	cd "$1" > /dev/null
	temp_dir="$(pwd)"
	cd - > /dev/null
	echo "$temp_dir"
}

function parse_args {
	if [ $# -eq 0 ]; then return 0; fi

	while [ $# -gt 0 ]; do
		case $1 in
			# DIRECTORIES
			-a )
				shift
				DIR_exec="$(get_absolute_dir "$1")"
				;;
			-t )
				shift
				DIR_tests="$(get_absolute_dir "$1")"
				;;
			# OPTIONS
			-c )
			 	shift
				EXEC_cc="CC=$1"
				;;
			-s )
				MAKE_rule="speed"
				;;
			# HELP
			-h|--help )
				usage
				exit $RET_usage
				;;
			* ) printf "Unknown argument. \"$1\"\n"
				;;
		esac
		shift
	done

	return $RET_success
}

function check_env {
	if [ ! -d "$DIR_exec" ]; then
		print_error "App directory \"$DIR_exec\" is not valid"
		return $RET_error
	elif [ ! -d "$DIR_tests" ]; then
		print_error "Tests directory \"$DIR_tests\" is not valid"
		return $RET_error
	fi
}

function set_env {
	# Defining script directories
	cd "$(dirname "$0")"
	DIR_script="$(pwd)"

	DIR_tests="$DIR_script/tests"
	DIR_exec="$DIR_script/.."
	EXEC_ibanco="i-banco"
}

function print_progress {
	# $1 : text to print
	# $2+: formatting args
	printf "\n${BYel}$1\n${RCol}" ${@:2}
}

function print_failure {
	# $1 : text to print
	# $2+: formatting args
	printf "\n${URed}FAILURE${Red}:${RCol} $1\n" ${@:2}
}

function print_error {
	# $1 : text to print
	# $2+: formatting args
	printf "\n${BRed}ERROR${Red}:${RCol} $1\n" ${@:2}
}

# Target functionality
function start_testing {
	# Rebuild project
	make -C "$DIR_exec" $EXEC_cc clean $MAKE_rule

	local retval=$RET_success
	for x in $DIR_tests/*.txt; do
		"$DIR_exec/$EXEC_ibanco" < $x > ${x%.in}.outhyp
		if [ $? -ne 0 ]; then
			retval=$RET_error
		fi

		# TODO: compare with wanted output
	    #diff ${x%.txt}.out ${x%.txt}.outhyp > ${x%.txt}.diff
	    #if [ -s ${x%.txt}.diff ]; then
	    #	echo "FAILURE: $x. See file ${x%.txt}.diff"
		#	retval=$RET_error
	    #else
	    #	rm -f ${x%.txt}.diff ${x%.txt}.outhyp
	    #fi
	done
	return $retval
}

function main {
	parse_args "$@"
	set_env
	check_env
	if [ $? -eq $RET_error ]; then
		usage
		exit $RET_error
	fi

	cd "$DIR_exec"
	start_testing

	exit $?
}

# Script starts HERE
main "$@"
