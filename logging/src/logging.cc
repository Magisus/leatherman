#include <leatherman/logging/logging.hpp>
#include <leatherman/locale/locale.hpp>
#include <vector>

// boost includes are not always warning-clean. Disable warnings that
// cause problems before including the headers, then re-enable the warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"

#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#pragma GCC diagnostic pop

#ifdef USE_POSIX_FUNCTIONS
#include <unistd.h>
#endif

using namespace std;
namespace expr = boost::log::expressions;
namespace src = boost::log::sources;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

namespace leatherman { namespace logging {

    static function<bool(log_level, string const&)> g_callback;
    static log_level g_level = log_level::none;
    static bool g_colorize = false;
    static bool g_error_logged = false;

    void setup_logging(ostream &dst, string locale)
    {
        // Remove existing sinks before adding a new one
        auto core = boost::log::core::get();
        core->remove_all_sinks();

        auto sink = boost::log::add_console_log(dst, keywords::auto_flush = true);

#if !defined(__sun) || !defined(__GNUC__)
        // Imbue the logging sink with the requested locale.
        // Locale in GCC is busted on Solaris, so skip it.
        sink->imbue(leatherman::locale::get_locale(locale));
#endif

        sink->set_formatter(
            expr::stream
                << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                << " " << left << setfill(' ') << setw(5) << leatherman::logging::log_level_attr
                << " " << leatherman::logging::namespace_attr
                << " - " << expr::smessage);

        boost::log::add_common_attributes();

        // Default to the warning level
        set_level(log_level::warning);

        // Set whether or not to use colorization depending if the destination is a tty
#ifdef USE_POSIX_FUNCTIONS
        g_colorize = (&dst == &cout && isatty(fileno(stdout))) || (&dst == &cerr && isatty(fileno(stderr)));
#else
        g_colorize = false;
#endif
    }

    void set_level(log_level level)
    {
        auto core = boost::log::core::get();
        core->set_logging_enabled(level != log_level::none);
        g_level = level;
    }

    log_level get_level()
    {
        return g_level;
    }

    void set_colorization(bool color)
    {
        g_colorize = color;
    }

    bool get_colorization()
    {
        return g_colorize;
    }

    bool is_enabled(log_level level)
    {
        return g_level != log_level::none && static_cast<int>(level) >= static_cast<int>(g_level);
    }

    bool error_has_been_logged() {
        return g_error_logged;
    }

    void clear_error_logged_flag() {
        g_error_logged = false;
    }

    void on_message(function<bool(log_level, string const&)> callback)
    {
        g_callback = callback;
    }

    string const& colorize(log_level level)
    {
        static const string none = "";
        static const string cyan = "\33[0;36m";
        static const string green = "\33[0;32m";
        static const string yellow = "\33[0;33m";
        static const string red = "\33[0;31m";

        if (!g_colorize) {
            return none;
        }

        if (level == log_level::trace || level == log_level::debug) {
            return cyan;
        } else if (level == log_level::info) {
            return green;
        } else if (level == log_level::warning) {
            return yellow;
        } else if (level == log_level::error || level == log_level::fatal) {
            return red;
        }
        return none;
    }

    string const& colorize()
    {
        static const string none = "";
        static const string reset = "\33[0m";
        return g_colorize ? reset : none;
    }

    void log(const string &logger, log_level level, boost::format& message)
    {
        log(logger, level, message.str());
    }

    void log(const string &logger, log_level level, string const& message)
    {
        if (level >= log_level::error) {
            g_error_logged = true;
        }
        if (!is_enabled(level) || (g_callback && !g_callback(level, message))) {
            return;
        }

        src::severity_logger<log_level> slg;
        slg.add_attribute("Namespace", attrs::constant<string>(logger));
        BOOST_LOG_SEV(slg, level) << colorize(level) << message << colorize();
    }

    istream& operator>>(istream& in, log_level& level)
    {
        string value;
        if (in >> value) {
            if (value == "none") {
                level = log_level::none;
                return in;
            }
            if (value == "trace") {
                level = log_level::trace;
                return in;
            }
            if (value == "debug") {
                level = log_level::debug;
                return in;
            }
            if (value == "info") {
                level = log_level::info;
                return in;
            }
            if (value == "warn") {
                level = log_level::warning;
                return in;
            }
            if (value == "error") {
                level = log_level::error;
                return in;
            }
            if (value == "fatal") {
                level = log_level::fatal;
                return in;
            }
        }
        throw runtime_error("invalid log level: expected none, trace, debug, info, warn, error, or fatal.");
    }

    ostream& operator<<(ostream& strm, log_level level)
    {
        static const vector<string> strings = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

        if (level != log_level::none) {
            size_t index = static_cast<size_t>(level) - 1;
            if (index < strings.size()) {
                strm << strings[index];
            }
        }

        return strm;
    }

}}  // namespace leatherman::logging

