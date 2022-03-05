function assert_error()
{
    expected_stderr="$1"

    shift 1
    actual_stderr="$("$@" 2>&1 1>/dev/null)"
    retval=$?
    is_stderr_expected="$(echo "$actual_stderr" | grep -i "$expected_stderr")"

    if [ $retval -eq 0 ]; then
        echo "assert_error: Return value is $retval, expected != 0"
        exit 1
    elif [ -z "$is_stderr_expected" ]; then
        echo "assert_error: stderr does not contain expected string"
        echo "      actual: $actual_stderr"
        echo "    expected: $expected_stderr"
        exit 1
    fi
}

function assert_success()
{
    actual_stderr="$("$@" 2>&1 1>/dev/null)"
    retval=$?

    if [ $retval -ne 0 ]; then
        echo "assert_error: Return value is $retval, expected 0"
        exit 1
    elif [ -n "$actual_stderr" ]; then
        echo "assert_error: stderr is not empty"
        echo "      actual: $actual_stderr"
        exit 1
    fi
}
