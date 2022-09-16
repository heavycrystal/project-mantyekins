#include    "mbc_rtc.hpp"

mbc_rtc_t::mbc_rtc_t() {
    memset(&this->data.as_raw, 0, sizeof(this->data.as_raw));
}

mbc_rtc_t::mbc_rtc_t(u8* input) {
    memcpy(&this->data.as_raw, input, sizeof(this->data.as_raw));
}

void mbc_rtc_t::tick(void) {
    subsecond_ticks++;
    if(subsecond_ticks == CYCLES_PER_SECOND) {
        subsecond_ticks = 0;
        data.as_name.seconds++;
        if(data.as_name.seconds == SECONDS_PER_MINUTE) {
            data.as_name.seconds = 0;
            data.as_name.minutes++;
            if(data.as_name.minutes == MINUTES_PER_HOUR) {
                data.as_name.minutes = 0;
                data.as_name.hours++;
                if(data.as_name.hours == HOURS_PER_DAY) {
                    data.as_name.hours = 0;
                    data.as_name.days_low++;
                    if(data.as_name.days_low == 0x00) {
                        //  replace bit offsets with named values
                        /*  sets the 9th bit of day counter if not already, otherwise sets the overflow bit */
                        SET_BIT(data.as_name.day_high_status, (BIT_CUT(data.as_name.day_high_status, 0, 1) == 1) ? 7 : 0);
                    }
                }
            }
        }
    }
}