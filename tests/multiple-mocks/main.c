#include "glib-mock.h"

#include "foo.h"
#include "bar.h"

const char * (*real_foo) (void);
const char * (*real_bar) (void);

const char *
foo (void)
{
  return "Mocked foo!";
}

const char *
bar (void)
{
  return "Mocked bar!";
}

void
test_multiple_mocks (void)
{
  g_assert_true (g_strcmp0 (real_foo (), "Foo!") == 0);
  g_assert_true (g_strcmp0 (real_bar (), "Bar!") == 0);

  g_assert_true (g_strcmp0 (foo (), "Mocked foo!") == 0);
  g_assert_true (g_strcmp0 (bar (), "Mocked bar!") == 0);
}

int
main (int argc, char **argv)
{
  g_mock_init (&argc, &argv);

  g_mock_add (foo);
  g_mock_get_real ("foo", &real_foo);

  g_mock_add (bar);
  g_mock_get_real ("bar", &real_bar);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/multiple-mocks", test_multiple_mocks);

  return g_test_run ();
}
