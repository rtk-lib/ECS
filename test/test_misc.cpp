#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "Assert.hpp"
#include "ComponentType.hpp"
#include "Logger/Logger.hpp"
#include "chronoDebug.hpp"

using namespace rtk::ecs;

namespace {
    struct CtA {};
    struct CtB {};
    struct CtC {};
    struct CtFresh1 {};
    struct CtFresh2 {};
    struct CtFresh3 {};
    struct CtConst {};
    struct CtPtr {};
    template <int N> struct CtTpl {};

    class CoutCapture {
        public:
            CoutCapture() : _old(std::cout.rdbuf(_buffer.rdbuf())) {}
            ~CoutCapture() { std::cout.rdbuf(_old); }
            std::string str() const { return _buffer.str(); }
        private:
            std::ostringstream _buffer;
            std::streambuf *_old;
    };

    bool contains(std::string const &haystack, std::string const &needle)
    {
        return haystack.find(needle) != std::string::npos;
    }
}

Test(component_type, same_type_same_id)
{
    cr_assert_eq(ComponentType::get_id<CtA>(), ComponentType::get_id<CtA>());
}

Test(component_type, id_is_stable_across_many_calls)
{
    std::size_t first = ComponentType::get_id<CtB>();
    for (int i = 0; i < 1000; ++i)
        cr_assert_eq(ComponentType::get_id<CtB>(), first);
}

Test(component_type, distinct_types_distinct_ids)
{
    cr_assert_neq(ComponentType::get_id<CtA>(), ComponentType::get_id<CtB>());
    cr_assert_neq(ComponentType::get_id<CtB>(), ComponentType::get_id<CtC>());
    cr_assert_neq(ComponentType::get_id<CtA>(), ComponentType::get_id<CtC>());
}

Test(component_type, fresh_types_get_consecutive_ids)
{
    std::size_t a = ComponentType::get_id<CtFresh1>();
    std::size_t b = ComponentType::get_id<CtFresh2>();
    std::size_t c = ComponentType::get_id<CtFresh3>();
    cr_assert_eq(b, a + 1);
    cr_assert_eq(c, b + 1);
}

Test(component_type, const_qualified_type_has_its_own_id)
{
    cr_assert_neq(ComponentType::get_id<CtConst>(), ComponentType::get_id<const CtConst>());
}

Test(component_type, pointer_type_has_its_own_id)
{
    cr_assert_neq(ComponentType::get_id<CtPtr>(), ComponentType::get_id<CtPtr *>());
}

Test(component_type, template_instances_have_distinct_ids)
{
    cr_assert_neq(ComponentType::get_id<CtTpl<1>>(), ComponentType::get_id<CtTpl<2>>());
}

Test(component_type, primitive_types_have_ids)
{
    std::size_t i = ComponentType::get_id<int>();
    std::size_t f = ComponentType::get_id<float>();
    cr_assert_neq(i, f);
    cr_assert_eq(i, ComponentType::get_id<int>());
}

Test(component_type, ids_from_threads_are_identical_for_same_type)
{
    std::size_t main_id = ComponentType::get_id<CtC>();
    std::size_t other = 0;
    std::thread t([&other]() { other = ComponentType::get_id<CtC>(); });
    t.join();
    cr_assert_eq(other, main_id);
}

Test(rtk_assert, true_condition_never_aborts)
{
    RTK_ASSERT(true, "must not fire");
    RTK_ASSERT(1 + 1 == 2, "must not fire");
    cr_assert(true);
}

#ifdef NDEBUG
Test(rtk_assert, false_condition_is_noop_in_release)
{
    RTK_ASSERT(false, "disabled in release");
    cr_assert(true);
}
#else
Test(rtk_assert, false_condition_aborts_in_debug, .signal = SIGABRT, .init = cr_redirect_stderr)
{
    RTK_ASSERT(false, "enabled in debug");
}
#endif

Test(logger, default_level_is_info)
{
    cr_assert_eq(Logger::minLevel(), L_INFO);
}

Test(logger, set_level_changes_min_level)
{
    Logger::setLevel(L_ERROR);
    cr_assert_eq(Logger::minLevel(), L_ERROR);
    Logger::setLevel(L_INFO);
}

Test(logger, levels_are_ordered)
{
    cr_assert_lt(L_DEBUG, L_INFO);
    cr_assert_lt(L_INFO, L_WARN);
    cr_assert_lt(L_WARN, L_ERROR);
    cr_assert_lt(L_ERROR, L_FATAL);
}

Test(logger, level_tags)
{
    cr_assert(contains(Logger::levelTag(L_DEBUG), "DBG"));
    cr_assert(contains(Logger::levelTag(L_INFO), "INF"));
    cr_assert(contains(Logger::levelTag(L_WARN), "WRN"));
    cr_assert(contains(Logger::levelTag(L_ERROR), "ERR"));
    cr_assert(contains(Logger::levelTag(L_FATAL), "FTL"));
}

Test(logger, unknown_level_tag)
{
    cr_assert_str_eq(Logger::levelTag(static_cast<level_t>(42)), "???");
}

Test(logger, timestamp_format)
{
    std::string ts = Logger::timestamp();
    cr_assert_eq(ts.size(), 12u);
    cr_assert_eq(ts[2], ':');
    cr_assert_eq(ts[5], ':');
    cr_assert_eq(ts[8], '.');
    for (std::size_t i : {0u, 1u, 3u, 4u, 6u, 7u, 9u, 10u, 11u})
        cr_assert(ts[i] >= '0' && ts[i] <= '9');
}

Test(logger, print_writes_message)
{
    CoutCapture cap;
    Logger::print(L_INFO, "file.cpp", 1, "hello world");
    cr_assert(contains(cap.str(), "hello world"));
}

Test(logger, print_ends_with_newline)
{
    CoutCapture cap;
    Logger::print(L_WARN, "file.cpp", 1, "x");
    std::string out = cap.str();
    cr_assert_not(out.empty());
    cr_assert_eq(out.back(), '\n');
}

Test(logger, print_strips_directories_from_file)
{
    CoutCapture cap;
    Logger::print(L_INFO, "some/deep/dir/source.cpp", 77, "msg");
    std::string out = cap.str();
    cr_assert(contains(out, "source.cpp:77"));
    cr_assert_not(contains(out, "some/deep"));
}

Test(logger, print_without_directory_keeps_file)
{
    CoutCapture cap;
    Logger::print(L_INFO, "plain.cpp", 3, "msg");
    cr_assert(contains(cap.str(), "plain.cpp:3"));
}

Test(logger, print_string_overload)
{
    CoutCapture cap;
    Logger::print(L_INFO, "f.cpp", 1, std::string("from std::string"));
    cr_assert(contains(cap.str(), "from std::string"));
}

Test(logger, print_contains_level_tag)
{
    CoutCapture cap;
    Logger::print(L_ERROR, "f.cpp", 1, "boom");
    cr_assert(contains(cap.str(), "ERR"));
}

Test(logger, debug_hidden_by_default)
{
    CoutCapture cap;
    Logger::print(L_DEBUG, "f.cpp", 1, "secret");
    cr_assert(cap.str().empty());
}

Test(logger, debug_shown_when_level_lowered)
{
    Logger::setLevel(L_DEBUG);
    CoutCapture cap;
    Logger::print(L_DEBUG, "f.cpp", 1, "visible");
    cr_assert(contains(cap.str(), "visible"));
    Logger::setLevel(L_INFO);
}

Test(logger, messages_below_level_are_filtered)
{
    Logger::setLevel(L_ERROR);
    CoutCapture cap;
    Logger::print(L_INFO, "f.cpp", 1, "info");
    Logger::print(L_WARN, "f.cpp", 1, "warn");
    cr_assert(cap.str().empty());
    Logger::setLevel(L_INFO);
}

Test(logger, fatal_always_passes_error_level)
{
    Logger::setLevel(L_ERROR);
    CoutCapture cap;
    Logger::print(L_FATAL, "f.cpp", 1, "fatal");
    cr_assert(contains(cap.str(), "fatal"));
    Logger::setLevel(L_INFO);
}

Test(logger, log_macros_use_current_file)
{
    CoutCapture cap;
    LOG_INFO("macro");
    cr_assert(contains(cap.str(), "test_misc.cpp:"));
    cr_assert(contains(cap.str(), "macro"));
}

Test(logger, all_macros_compile_and_respect_level)
{
    Logger::setLevel(L_DEBUG);
    CoutCapture cap;
    LOG_DEBUG("d");
    LOG_INFO("i");
    LOG_WARN("w");
    LOG_ERROR("e");
    LOG_FATAL("f");
    std::string out = cap.str();
    cr_assert(contains(out, "DBG"));
    cr_assert(contains(out, "INF"));
    cr_assert(contains(out, "WRN"));
    cr_assert(contains(out, "ERR"));
    cr_assert(contains(out, "FTL"));
    Logger::setLevel(L_INFO);
}

Test(logger, escape_sequences)
{
    cr_assert_str_eq(RESET, "\033[0m");
    cr_assert_str_eq(BOLD, "\033[1m");
    cr_assert_str_eq(RED, "\033[31m");
    cr_assert_str_eq(GREEN, "\033[32m");
    cr_assert_str_eq(CYAN, "\033[36m");
}

Test(chrono_debug, scoped_timer_logs_name_on_destruction)
{
    CoutCapture cap;
    {
        rtk::debug::ScopedTimer t("my_scope");
        cr_assert(cap.str().empty());
    }
    cr_assert(contains(cap.str(), "[my_scope]"));
}

Test(chrono_debug, profile_scope_macro_logs)
{
    CoutCapture cap;
    {
        PROFILE_SCOPE("profiled");
    }
    cr_assert(contains(cap.str(), "[profiled]"));
}

Test(chrono_debug, two_profile_scopes_in_same_block)
{
    CoutCapture cap;
    {
        PROFILE_SCOPE("first");
        PROFILE_SCOPE("second");
    }
    cr_assert(contains(cap.str(), "[first]"));
    cr_assert(contains(cap.str(), "[second]"));
}

Test(chrono_debug, time_start_end_macros_log)
{
    CoutCapture cap;
    TIME_START(block);
    TIME_END(block);
    cr_assert(contains(cap.str(), "[block]"));
    cr_assert(duration_block >= 0);
}

Test(chrono_debug, scoped_timer_reports_microseconds)
{
    CoutCapture cap;
    {
        rtk::debug::ScopedTimer t("unit");
    }
    cr_assert(contains(cap.str(), "μs"));
}
