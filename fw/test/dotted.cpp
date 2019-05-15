#include "dotted.h"

#include <algorithm>
#include <boost/test/included/unit_test.hpp>
namespace btt = boost::test_tools;

BOOST_AUTO_TEST_CASE(remove_dot)
{
    using filesystem::path;
    BOOST_TEST(removeDot("./folder/filename").generic_string() == "folder/filename");

    BOOST_TEST(removeDot("folder/filename").generic_string() == "folder/filename");
}
