#include <catch.hpp>
#include <leatherman/file_util/file.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace leatherman { namespace file_util {

// TODO(ale): consider making file_utils.cpp and string_utils.cpp
// consistent across cthun-agent and pegasus before writing more tests

TEST_CASE("file_util::tilde_expand", "[utils]") {
#ifdef _WIN32
    boost::nowide::setenv("USERPROFILE", "/testhome", 1)
#else
    setenv("HOME", "/testhome", 1);
#endif

    SECTION("empty path should be empty") {
        REQUIRE(tilde_expand("") == "");
    }

    SECTION("spaces should be preserved") {
        REQUIRE(tilde_expand("i like spaces") == "i like spaces");
    }

    SECTION("should expand using environment variable") {
        CHECK(tilde_expand("~") == "/testhome");
        CHECK(tilde_expand("~/") == "/testhome/");
        CHECK(tilde_expand("~/foo") == "/testhome/foo");
    }

    SECTION("only a ~ at the start") {
        REQUIRE(tilde_expand("/foo/bar~") == "/foo/bar~");
    }

    SECTION("~baz/foo does not expand") {
        REQUIRE(tilde_expand("~baz/foo") == "~baz/foo");
    }

    SECTION("it should expand the home directory path") {
        REQUIRE(tilde_expand("~/foo") != "~/foo");
    }

    SECTION("it should not expand the working directory path") {
        REQUIRE(tilde_expand("./foo") == "./foo");
    }

    std::string home_path { getenv("HOME") };

    SECTION("it should expand ~ to the HOME env var") {
        REQUIRE(tilde_expand("~") == home_path);
    }

    SECTION("it should expand ~ as the base directory") {
        std::string expected_path { home_path + "/spam" };
        std::string expanded_path {tilde_expand("~/spam") };
        REQUIRE(expanded_path == expected_path);
    }
}

TEST_CASE("shell_quote", "[utils]") {
    SECTION("empty string") {
        REQUIRE(shell_quote("") == "\"\"");
    }

    SECTION("single word") {
        REQUIRE(shell_quote("plain") == "\"plain\"");
    }

    SECTION("words separated by space") {
        REQUIRE(shell_quote("a space") == "\"a space\"");
    }

    SECTION("exclamation mark") {
        REQUIRE(shell_quote("!csh") == "\"!csh\"");
    }

    SECTION("single quote before expression") {
        REQUIRE(shell_quote("'open quote") == "\"'open quote\"");
    }

    SECTION("single quote after expression") {
        REQUIRE(shell_quote("close quote'") == "\"close quote'\"");
    }

    SECTION("double quote before expression") {
        REQUIRE(shell_quote("\"open doublequote")
                == "\"\\\"open doublequote\"");
    }

    SECTION("double quote after expression") {
        REQUIRE(shell_quote("close doublequote\"")
                == "\"close doublequote\\\"\"");
    }
}

static const auto home_path = tilde_expand("~");
static const auto file_path =
        tilde_expand("~/test_file_" + boost::filesystem::unique_path().string());
static const auto dir_path =
        tilde_expand("~/test_dir_" + boost::filesystem::unique_path().string());

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
