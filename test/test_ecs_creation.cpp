#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include "ecs.hpp"


void cr_redirect_all_stream(void)
{
    cr_redirect_stdout();
    cr_redirect_stderr();
}

Test(ecs_suite, creation_test) {
    cr_redirect_all_stream();
    rtk::ecs ecs;
    cr_assert(true);
}