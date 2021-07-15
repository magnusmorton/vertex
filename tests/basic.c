#include <glib.h>

#include "runtime.h"

static void
set_up_fixture(GArray **array, gconstpointer data)
{
	init_san();
	*array = NULL;
}

static void
tear_down_fixture(GArray **array, gconstpointer data)
{
	g_array_free(*array, TRUE);
	finish_san();
}

static void
test_no_data(GArray **detected, gconstpointer data)
{
	// do nothing

	*detected = get_detected();
	g_assert_true((*detected)->len == 0);
}

static void
test_1_data(GArray **detected, gconstpointer data)
{
	void *ptr = 0x94;
	mark_root("foo", ptr, 8, NULL, 0);
	*detected = get_detected();
	g_assert_true((*detected)->len == 1);
}

static void
test_2_data(GArray **detected, gconstpointer data)
{
	void *ptr1 = 0x94;
	void *ptr2 = 0xdeadbeef;
	mark_root("foo", ptr1, 8, NULL, 0);
	mark_root("bar", ptr2, 64, NULL, 0);
	*detected = get_detected();
	g_assert_true((*detected)->len == 2);
}

int main(int argc, char *argv[]) {
	g_test_init (&argc, &argv, NULL);
	g_test_add("/basic/no_data", GArray*, NULL,
		   set_up_fixture, test_no_data, tear_down_fixture);
	g_test_add("/basic/one_data", GArray*, NULL,
		   set_up_fixture, test_1_data, tear_down_fixture);
	g_test_add("/basic/two_data", GArray*, NULL,
		   set_up_fixture, test_2_data, tear_down_fixture);
	return g_test_run();
}
