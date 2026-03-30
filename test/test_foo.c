#include <assert.h>
#include "foo.h"

int test_foo(int argc, char *argv[])
{
    assert(foo(1) == 2);
    return 0;
}
