test_error=0

function print_assert_out_error()
{
    echo "assert: $1 does not contain expected string"
    echo "      actual: $2"
    echo "    expected: $3"
}

function assert_retval()
{
    if [ $1 -ne $2 ]; then
        echo "assert: Return value is $1, expected $2"
        test_error=1
    fi
}

function assert_out()
{
    if [ -z "$2" -a -z "$3" ]; then
        return
    elif [ -z "$2" -a -n "$3" ]; then
        print_assert_out_error $1 "$2" "$3"
    elif [ -n "$2" -a -z "$3" ]; then
        print_assert_out_error $1 "$2" "$3"
    else
        is_stderr_expected="$(echo "$2" | grep -i "$3")"
        if [ -z "$is_stderr_expected" ]; then
            print_assert_out_error $1 "$2" "$3"
        fi
    fi
}

function assert()
{
    expected_stdout="$1"
    expected_stderr="$2"
    expected_retval="$3"
    shift 3

    "$@" 1>actual_stdout 2>actual_stderr
    actual_retval=$?
    actual_stdout="$(cat actual_stdout)"
    actual_stderr="$(cat actual_stderr)"
    rm actual_stdout actual_stderr

    test_error=0
    assert_retval $actual_retval $expected_retval
    assert_out stdout "$actual_stdout" "$expected_stdout"
    assert_out stderr "$actual_stderr" "$expected_stderr"

    if [ $test_error -ne 0 ]; then
        exit 1
    fi
}
