#include <catch.hpp>
#include <leatherman/file_util/file.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace leatherman { namespace file_util {

// TODO(ale): consider making file_utils.cpp and string_utils.cpp
// consistent across cthun-agent and pegasus before writing more tests

TEST_CASE("file_util::tildeExpand", "[utils]") {
#ifdef _WIN32
    boost::nowide::setenv("USERPROFILE", "/testhome", 1)
#else
    setenv("HOME", "/testhome", 1);
#endif

    SECTION("empty path should be empty") {
        REQUIRE(tildeExpand("") == "");
    }

    SECTION("spaces should be preserved") {
        REQUIRE(tildeExpand("i like spaces") == "i like spaces");
    }

    SECTION("should expand using environment variable") {
        CHECK(tildeExpand("~") == "/testhome");
        CHECK(tildeExpand("~/") == "/testhome/");
        CHECK(tildeExpand("~/foo") == "/testhome/foo");
    }

    SECTION("only a ~ at the start") {
        REQUIRE(tildeExpand("/foo/bar~") == "/foo/bar~");
    }

    SECTION("~baz/foo does not expand") {
        REQUIRE(tildeExpand("~baz/foo") == "~baz/foo");
    }

    SECTION("it should expand the home directory path") {
        REQUIRE(tildeExpand("~/foo") != "~/foo");
    }

    SECTION("it should not expand the working directory path") {
        REQUIRE(tildeExpand("./foo") == "./foo");
    }

    std::string home_path { getenv("HOME") };

    SECTION("it should expand ~ to the HOME env var") {
        REQUIRE(tildeExpand("~") == home_path);
    }

    SECTION("it should expand ~ as the base directory") {
        std::string expected_path { home_path + "/spam" };
        std::string expanded_path { tildeExpand("~/spam") };
        REQUIRE(expanded_path == expected_path);
    }
}

TEST_CASE("shellQuote", "[utils]") {
    SECTION("empty string") {
        REQUIRE(shellQuote("") == "\"\"");
    }

    SECTION("single word") {
        REQUIRE(shellQuote("plain") == "\"plain\"");
    }

    SECTION("words separated by space") {
        REQUIRE(shellQuote("a space") == "\"a space\"");
    }

    SECTION("exclamation mark") {
        REQUIRE(shellQuote("!csh") == "\"!csh\"");
    }

    SECTION("single quote before expression") {
        REQUIRE(shellQuote("'open quote") == "\"'open quote\"");
    }

    SECTION("single quote after expression") {
        REQUIRE(shellQuote("close quote'") == "\"close quote'\"");
    }

    SECTION("double quote before expression") {
        REQUIRE(shellQuote("\"open doublequote")
                == "\"\\\"open doublequote\"");
    }

    SECTION("double quote after expression") {
        REQUIRE(shellQuote("close doublequote\"")
                == "\"close doublequote\\\"\"");
    }
}

static const auto home_path = tildeExpand("~");
static const auto file_path =
        tildeExpand("~/test_file_" + boost::filesystem::unique_path().string());
static const auto dir_path =
        tildeExpand("~/test_dir_" + boost::filesystem::unique_path().string());

TEST_CASE("lth_file::file_readable", "[utils]") {
    SECTION("it can check that a file does not exist") {
        REQUIRE_FALSE(file_readable(file_path));
    }

    SECTION("it can check that a directory exists") {
        REQUIRE(file_readable(home_path));
    }
}

TEST_CASE("lth_file::atomic_write_to_file", "[utils]") {
    SECTION("it can write to a regular file, ensure it exists, and delete it") {
        REQUIRE_FALSE(file_readable(file_path));
        atomic_write_to_file("test\n", file_path);
        REQUIRE(file_readable(file_path));
        boost::filesystem::remove(file_path);
        REQUIRE_FALSE(file_readable(file_path));
    }
}

TEST_CASE("lth_file::createDirectory", "[utils]") {
    SECTION("it can create and remove an empty directory") {
        REQUIRE_FALSE(file_readable(dir_path));
        boost::filesystem::create_directory(dir_path);
        REQUIRE(file_readable(dir_path));
        boost::filesystem::remove(dir_path);
        REQUIRE_FALSE(file_readable(dir_path));
    }
}

}}  // namespace leatherman::file_util
