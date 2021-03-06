#ifndef TZ_H
#define TZ_H

// Howard Hinnant
// This work is licensed under a Creative Commons Attribution 4.0 International License.
// http://creativecommons.org/licenses/by/4.0/

// Get more recent database at http://www.iana.org/time-zones

// Questions:
// 1.  Reload database.
// 4.  Is the utc to sys renaming complete?  Was it done correctly?

#include "date.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <istream>
#include <ostream>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace date
{

using seconds_point = std::chrono::time_point<std::chrono::system_clock,
                                              std::chrono::seconds>;

enum class tz {utc, local, standard};
enum class choose {earliest, latest};

class nonexistent_local_time
    : public std::runtime_error
{
public:
    template <class Rep, class Period>
    nonexistent_local_time(std::chrono::time_point<std::chrono::system_clock,
                               std::chrono::duration<Rep, Period>> tp,
                           seconds_point first, const std::string& first_abbrev,
                           seconds_point last, const std::string& last_abbrev,
                           seconds_point time_sys);

private:
    template <class Rep, class Period>
    static
    std::string
    make_msg(std::chrono::time_point<std::chrono::system_clock,
                 std::chrono::duration<Rep, Period>> tp,
             seconds_point first, const std::string& first_abbrev,
             seconds_point last, const std::string& last_abbrev,
             seconds_point time_sys);
};

template <class Rep, class Period>
inline
nonexistent_local_time::nonexistent_local_time(
    std::chrono::time_point<std::chrono::system_clock,
        std::chrono::duration<Rep, Period>> tp,
    seconds_point first, const std::string& first_abbrev,
    seconds_point last, const std::string& last_abbrev,
    seconds_point time_sys)
    : std::runtime_error(make_msg(tp, first, first_abbrev, last, last_abbrev, time_sys))
    {}

template <class Rep, class Period>
std::string
nonexistent_local_time::make_msg(std::chrono::time_point<std::chrono::system_clock,
                                     std::chrono::duration<Rep, Period>> tp,
                                 seconds_point first, const std::string& first_abbrev,
                                 seconds_point last, const std::string& last_abbrev,
                                 seconds_point time_sys)
{
    using namespace date;
    std::ostringstream os;
    os << tp << " is in a gap between\n"
       << first << ' ' << first_abbrev << " and\n"
       << last  << ' ' << last_abbrev
       << " which are both equivalent to\n"
       << time_sys << " UTC";
    return os.str();
}

class ambiguous_local_time
    : public std::runtime_error
{
public:
    template <class Rep, class Period>
    ambiguous_local_time(std::chrono::time_point<std::chrono::system_clock,
                             std::chrono::duration<Rep, Period>> tp,
                         std::chrono::seconds first_offset,
                         const std::string& first_abbrev,
                         std::chrono::seconds second_offset,
                         const std::string& second_abbrev);

private:
    template <class Rep, class Period>
    static
    std::string
    make_msg(std::chrono::time_point<std::chrono::system_clock,
                 std::chrono::duration<Rep, Period>> tp,
             std::chrono::seconds first_offset, const std::string& first_abbrev,
             std::chrono::seconds second_offset, const std::string& second_abbrev);
};

template <class Rep, class Period>
inline
ambiguous_local_time::ambiguous_local_time(
    std::chrono::time_point<std::chrono::system_clock,
        std::chrono::duration<Rep, Period>> tp,
    std::chrono::seconds first_offset,
    const std::string& first_abbrev,
    std::chrono::seconds second_offset,
    const std::string& second_abbrev)
    : std::runtime_error(make_msg(tp, first_offset, first_abbrev, second_offset,
                                  second_abbrev))
    {}

template <class Rep, class Period>
std::string
ambiguous_local_time::make_msg(std::chrono::time_point<std::chrono::system_clock,
                                   std::chrono::duration<Rep, Period>> tp,
                               std::chrono::seconds first_offset,
                               const std::string& first_abbrev,
                               std::chrono::seconds second_offset,
                               const std::string& second_abbrev)
{
    using namespace date;
    std::ostringstream os;
    os << tp << " is ambiguous.  It could be\n"
       << tp << ' ' << first_abbrev << " == " << tp - first_offset << " UTC or\n" 
       << tp << ' ' << second_abbrev  << " == " << tp - second_offset  << " UTC";
    return os.str();
}

class Rule;

struct Info
{
    seconds_point        begin;
    seconds_point        end;
    std::chrono::seconds offset;
    std::chrono::minutes save;
    std::string          abbrev;
};

std::ostream&
operator<<(std::ostream& os, const Info& r);

class Zone
{
private:
    struct zonelet;

    std::string          name_;
    std::vector<zonelet> zonelets_;

public:
    explicit Zone(const std::string& s);

    const std::string& name() const {return name_;}
    Info get_info(std::chrono::system_clock::time_point tp, tz timezone) const;

    template <class Rep, class Period>
    Info
    get_info(std::chrono::time_point<std::chrono::system_clock,
                                     std::chrono::duration<Rep, Period>> tp,
             tz timezone) const
    {
        using namespace std::chrono;
        return get_info(floor<system_clock::duration>(tp), timezone);
    }

    template <class Rep, class Period>
    std::chrono::time_point<std::chrono::system_clock,
        typename std::common_type<std::chrono::duration<Rep, Period>,
                                  std::chrono::seconds>::type>
    to_sys(std::chrono::time_point<std::chrono::system_clock,
                                   std::chrono::duration<Rep, Period>> tp) const;

    template <class Rep, class Period>
    std::chrono::time_point<std::chrono::system_clock,
        typename std::common_type<std::chrono::duration<Rep, Period>,
                                  std::chrono::seconds>::type>
    to_sys(std::chrono::time_point<std::chrono::system_clock,
                                   std::chrono::duration<Rep, Period>> tp,
           choose z) const;

    template <class Rep, class Period>
    std::pair
    <
        std::chrono::time_point<std::chrono::system_clock,
            typename std::common_type<std::chrono::duration<Rep, Period>,
                                      std::chrono::seconds>::type>,
        std::string
    >
    to_local(std::chrono::time_point<std::chrono::system_clock,
                                   std::chrono::duration<Rep, Period>> tp) const;

    friend bool operator==(const Zone& x, const Zone& y);
    friend bool operator< (const Zone& x, const Zone& y);
    friend std::ostream& operator<<(std::ostream& os, const Zone& z);

    void add(const std::string& s);
    void adjust_infos(const std::vector<Rule>& rules);

private:
    void parse_info(std::istream& in);

    template <class Rep, class Period, bool b>
    std::chrono::time_point<std::chrono::system_clock,
        typename std::common_type<std::chrono::duration<Rep, Period>,
                                  std::chrono::seconds>::type>
    to_sys_impl(std::chrono::time_point<std::chrono::system_clock,
                                        std::chrono::duration<Rep, Period>> tp,
                choose z, std::integral_constant<bool, b> do_throw) const;
};

template <class Rep, class Period>
inline
std::chrono::time_point<std::chrono::system_clock,
    typename std::common_type<std::chrono::duration<Rep, Period>,
                              std::chrono::seconds>::type>
Zone::to_sys(std::chrono::time_point<std::chrono::system_clock,
             std::chrono::duration<Rep, Period>> tp) const
{
    return to_sys_impl(tp, choose{}, std::true_type{});
}

template <class Rep, class Period>
inline
std::chrono::time_point<std::chrono::system_clock,
    typename std::common_type<std::chrono::duration<Rep, Period>,
                              std::chrono::seconds>::type>
Zone::to_sys(std::chrono::time_point<std::chrono::system_clock,
             std::chrono::duration<Rep, Period>> tp, choose z) const
{
    return to_sys_impl(tp, z, std::false_type{});
}

template <class Rep, class Period>
inline
std::pair
<
    std::chrono::time_point<std::chrono::system_clock,
        typename std::common_type<std::chrono::duration<Rep, Period>,
                                  std::chrono::seconds>::type>,
    std::string
>
Zone::to_local(std::chrono::time_point<std::chrono::system_clock,
               std::chrono::duration<Rep, Period>> tp) const
{
    auto const i = get_info(tp, tz::utc);
    return {tp + i.offset, i.abbrev};
}

inline bool operator==(const Zone& x, const Zone& y) {return x.name_ == y.name_;}
inline bool operator< (const Zone& x, const Zone& y) {return x.name_ < y.name_;}

inline bool operator!=(const Zone& x, const Zone& y) {return !(x == y);}
inline bool operator> (const Zone& x, const Zone& y) {return   y < x;}
inline bool operator<=(const Zone& x, const Zone& y) {return !(y < x);}
inline bool operator>=(const Zone& x, const Zone& y) {return !(x < y);}

template <class Rep, class Period, bool b>
std::chrono::time_point<std::chrono::system_clock,
    typename std::common_type<std::chrono::duration<Rep, Period>,
                              std::chrono::seconds>::type>
Zone::to_sys_impl(std::chrono::time_point<std::chrono::system_clock,
                  std::chrono::duration<Rep, Period>> tp,
                  choose z, std::integral_constant<bool, b> do_throw) const
{
    using namespace date;
    using namespace std::chrono;
    auto i = get_info(tp, tz::local);
    auto tp_sys = tp - i.offset;
    if (tp_sys - i.begin <= days{1})
    {
        if (tp < i.begin + i.offset)
        {
            if (do_throw)
            {
                auto prev = get_info(i.begin - seconds{1}, tz::utc);
                throw nonexistent_local_time(tp, i.begin + prev.offset, prev.abbrev,
                                             i.begin + i.offset, i.abbrev, i.begin);
            }
            return i.begin;
        }
        assert(tp >= i.begin + get_info(i.begin - seconds{1}, tz::utc).offset);
    }
    if (i.end - tp_sys <= days{1})
    {
        assert(tp < i.end + i.offset);
        auto next = get_info(i.end, tz::utc);
        if (tp >= i.end + next.offset)
        {
            if (do_throw)
                throw ambiguous_local_time(tp, i.offset, i.abbrev,
                                               next.offset, next.abbrev);
            if (z == choose::earliest)
                return tp_sys;
            return tp - next.offset;
        }
    }
    return tp_sys;
}

class Link
{
private:
    std::string name_;
    std::string target_;
public:
    explicit Link(const std::string& s);

    const std::string& name() const {return name_;}
    const std::string& target() const {return target_;}

    friend bool operator==(const Link& x, const Link& y) {return x.name_ == y.name_;}
    friend bool operator< (const Link& x, const Link& y) {return x.name_ < y.name_;}

    friend std::ostream& operator<<(std::ostream& os, const Link& x);
};

inline bool operator!=(const Link& x, const Link& y) {return !(x == y);}
inline bool operator> (const Link& x, const Link& y) {return   y < x;}
inline bool operator<=(const Link& x, const Link& y) {return !(y < x);}
inline bool operator>=(const Link& x, const Link& y) {return !(x < y);}

class Leap
{
private:
    seconds_point date_;

public:
    explicit Leap(const std::string& s);

    seconds_point date() const {return date_;}

    friend bool operator==(const Leap& x, const Leap& y) {return x.date_ == y.date_;}
    friend bool operator< (const Leap& x, const Leap& y) {return x.date_ < y.date_;}

    template <class Duration>
    friend
    bool
    operator==(const Leap& x,
               const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
    {
        return x.date_ == y;
    }

    template <class Duration>
    friend
    bool
    operator< (const Leap& x,
               const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
    {
        return x.date_ < y;
    }

    template <class Duration>
    friend
    bool
    operator< (const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
               const Leap& y)
    {
        return x < y.date_;
    }

    friend std::ostream& operator<<(std::ostream& os, const Leap& x);
};

inline bool operator!=(const Leap& x, const Leap& y) {return !(x == y);}
inline bool operator> (const Leap& x, const Leap& y) {return   y < x;}
inline bool operator<=(const Leap& x, const Leap& y) {return !(y < x);}
inline bool operator>=(const Leap& x, const Leap& y) {return !(x < y);}

template <class Duration>
inline
bool
operator==(const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
           const Leap& y)
{
    return y == x;
}

template <class Duration>
inline
bool
operator!=(const Leap& x,
           const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
{
    return !(x == y);
}

template <class Duration>
inline
bool
operator!=(const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
           const Leap& y)
{
    return !(x == y);
}

template <class Duration>
inline
bool
operator> (const Leap& x,
           const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
{
    return y < x;
}

template <class Duration>
inline
bool
operator> (const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
           const Leap& y)
{
    return y < x;
}

template <class Duration>
inline
bool
operator<=(const Leap& x,
           const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
{
    return !(y < x);
}

template <class Duration>
inline
bool
operator<=(const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
           const Leap& y)
{
    return !(y < x);
}

template <class Duration>
inline
bool
operator>=(const Leap& x,
           const std::chrono::time_point<std::chrono::system_clock, Duration>& y)
{
    return !(x < y);
}

template <class Duration>
inline
bool
operator>=(const std::chrono::time_point<std::chrono::system_clock, Duration>& x,
           const Leap& y)
{
    return !(x < y);
}

struct TZ_DB
{
    std::vector<Zone> zones;
    std::vector<Link> links;
    std::vector<Leap> leaps;
    std::vector<Rule> rules;
    
    TZ_DB() = default;
    TZ_DB(TZ_DB&&) = default;
    TZ_DB& operator=(TZ_DB&&) = default;
};

std::ostream& operator<<(std::ostream& os, const TZ_DB& db);

const TZ_DB& get_tzdb();
const TZ_DB& reload_tzdb();
const TZ_DB& reload_tzdb(const std::string& new_install);

const Zone* locate_zone(const std::string& tz_name);
const Zone* current_timezone();

class utc_clock
{
public:
    using duration                  = std::chrono::system_clock::duration;
    using rep                       = duration::rep;
    using period                    = duration::period;
    using time_point                = std::chrono::time_point<utc_clock>;
    static CONSTDATA bool is_steady = true;

    static time_point now() noexcept;

    template <class Duration>
        static
        std::chrono::time_point<utc_clock,
            typename std::common_type<Duration, std::chrono::seconds>::type>
        sys_to_utc(std::chrono::time_point<std::chrono::system_clock, Duration> t);

    template <class Duration>
        static
        std::chrono::time_point<std::chrono::system_clock,
            typename std::common_type<Duration, std::chrono::seconds>::type>
        utc_to_sys(std::chrono::time_point<utc_clock, Duration> t);
};

inline
utc_clock::time_point
utc_clock::now() noexcept
{
    using namespace std::chrono;
    return sys_to_utc(system_clock::now());
}

template <class Duration>
std::chrono::time_point<utc_clock,
    typename std::common_type<Duration, std::chrono::seconds>::type>
utc_clock::sys_to_utc(std::chrono::time_point<std::chrono::system_clock, Duration> t)
{
    using namespace std::chrono;
    using duration = typename std::common_type<Duration, seconds>::type;
    using time_point = std::chrono::time_point<utc_clock, duration>;
    auto const& leaps = get_tzdb().leaps;
    auto const lt = std::upper_bound(leaps.begin(), leaps.end(), t);
    return time_point{t.time_since_epoch() + seconds{lt-leaps.begin()}};
}

template <class Duration>
std::chrono::time_point<std::chrono::system_clock,
    typename std::common_type<Duration, std::chrono::seconds>::type>
utc_clock::utc_to_sys(std::chrono::time_point<utc_clock, Duration> t)
{
    using namespace std::chrono;
    using duration = typename std::common_type<Duration, seconds>::type;
    using time_point = std::chrono::time_point<system_clock, duration>;
    auto const& leaps = get_tzdb().leaps;
    auto tp = time_point{t.time_since_epoch()};
    auto const lt = std::upper_bound(leaps.begin(), leaps.end(), tp);
    tp -= seconds{lt-leaps.begin()};
    if (lt != leaps.begin() && tp + seconds{1} < lt[-1])
        tp += seconds{1};
    return tp;
}

}  // namespace date

#endif  // TZ_H
