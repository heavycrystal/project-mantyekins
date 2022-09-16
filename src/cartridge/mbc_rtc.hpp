#pragma     once

#include    "../common/common.hpp"

/* RTC constants */
#define     SECONDS_PER_MINUTE          60
#define     MINUTES_PER_HOUR            60
#define     HOURS_PER_DAY               24

class mbc_rtc_t {
    public:
    union {
        struct {
            u8 seconds;
            u8 minutes;
            u8 hours;
            u8 days_low;
            u8 day_high_status;
        } as_name;
        u8 as_raw[5] = { };
    } data;

    /* deleting copy constructor and copy assignment operator */
    mbc_rtc_t& operator = (const mbc_rtc_t&) = delete;
    mbc_rtc_t(const mbc_rtc_t&) = delete;

    private:
    u32 subsecond_ticks = 0;

    public:
    mbc_rtc_t();
    mbc_rtc_t(u8* input);

    void tick(void);
};