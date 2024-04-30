package org.duckdb;

import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDateTime;

public class Compat {
    /**
     * The constant in the Java programming language, sometimes referred to
     * as a type code, that identifies the generic SQL type
     * {@code TIMESTAMP WITH TIMEZONE}.
     *
     * @since 1.8
     */
    public static final int TIMESTAMP_WITH_TIMEZONE = 2014;

    private static final int MILLIS_PER_SECOND = 1000;
    public static Timestamp from(Instant instant) {
        Timestamp stamp = new Timestamp(instant.getEpochSecond() * MILLIS_PER_SECOND);
        stamp.setNanos(instant.getNano());
        return stamp;
    }

    public static Timestamp timestampValueOf(LocalDateTime dateTime) {
        return new Timestamp(dateTime.getYear() - 1900, dateTime.getMonthValue() - 1, dateTime.getDayOfMonth(),
                             dateTime.getHour(), dateTime.getMinute(), dateTime.getSecond(), dateTime.getNano());
    }

    @SuppressWarnings("deprecation")
    public static LocalDateTime toLocalDateTime(Timestamp timestamp) {
        return LocalDateTime.of(timestamp.getYear() + 1900, timestamp.getMonth() + 1, timestamp.getDate(),
                                timestamp.getHours(), timestamp.getMinutes(), timestamp.getSeconds(),
                                timestamp.getNanos());
    }
}
