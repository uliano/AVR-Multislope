#include "scpi.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>

#include "globals.hpp"
#include "input.h"
#include "pins.hpp"
#include "line_parser.hpp"

namespace {
using ScpiParser = ScpiCommandParser<4>;
using ScpiCommand = ScpiParser::CommandType;
using ScpiEndpoint = StreamParserEndpoint<ScpiParser, 96>;
using ScpiRouter = CommandRouter<4>;

constexpr uint16_t SCPI_MAX_READ_COUNT = 1022;
constexpr uint16_t SCPI_BUFFER_LIMIT = 1022;

bool g_scpi_initialized = false;
ParserHub<2> g_parser_hub;

InputSource g_selected_input = InputSource::EXTERNAL;
WindowLength g_selected_window = WindowLength::PLC_1;

bool g_has_last_measurement = false;
Measurement g_last_measurement{0u, 0};

// 0 means infinite/free-running acquisition.
uint16_t g_samples_per_trigger = 0;
uint16_t g_samples_remaining = 0;
bool g_trigger_armed = false;

bool g_trigger_input_inverted = false;
bool g_trigger_output_inverted = false;
bool g_trigger_input_pullup = false;

inline void stream_write_byte(ByteStream &stream, char c) {
    stream.write_byte(static_cast<uint8_t>(c));
}

void stream_write_cstr(ByteStream &stream, const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        stream_write_byte(stream, *text);
        ++text;
    }
}

void stream_write_u32(ByteStream &stream, uint32_t value) {
    char buffer[12];
    ultoa(static_cast<unsigned long>(value), buffer, 10);
    stream_write_cstr(stream, buffer);
}

void stream_write_i32(ByteStream &stream, int32_t value) {
    char buffer[12];
    ltoa(static_cast<long>(value), buffer, 10);
    stream_write_cstr(stream, buffer);
}

void scpi_reply_ok(ByteStream &stream) {
    stream_write_cstr(stream, "OK\n");
}

void scpi_reply_error(ByteStream &stream, const char *code) {
    stream_write_cstr(stream, "ERR:");
    stream_write_cstr(stream, code ? code : "GENERIC");
    stream_write_cstr(stream, "\n");
}

void scpi_reply_measurement(ByteStream &stream, const Measurement &measurement) {
    stream_write_u32(stream, measurement.timestamp);
    stream_write_cstr(stream, ",");
    stream_write_i32(stream, measurement.value);
}

bool parse_polarity_token(const char *token, bool &inverted) {
    if (!token) {
        return false;
    }
    if (parser_command_equals(token, "NORM") ||
        parser_command_equals(token, "NORMAL") ||
        parser_command_equals(token, "POS") ||
        parser_command_equals(token, "POSITIVE")) {
        inverted = false;
        return true;
    }
    if (parser_command_equals(token, "INV") ||
        parser_command_equals(token, "INVERTED") ||
        parser_command_equals(token, "NEG") ||
        parser_command_equals(token, "NEGATIVE")) {
        inverted = true;
        return true;
    }
    return false;
}

bool parse_enable_token(const char *token, bool &enabled) {
    if (!token) {
        return false;
    }
    if (parser_command_equals(token, "ON") ||
        parser_command_equals(token, "ENABLE") ||
        parser_command_equals(token, "ENABLED") ||
        strcmp(token, "1") == 0) {
        enabled = true;
        return true;
    }
    if (parser_command_equals(token, "OFF") ||
        parser_command_equals(token, "DISABLE") ||
        parser_command_equals(token, "DISABLED") ||
        strcmp(token, "0") == 0) {
        enabled = false;
        return true;
    }
    return false;
}

void apply_trigger_io_config() {
    TRG_IN::invert(g_trigger_input_inverted);
    TRG_OUT::invert(g_trigger_output_inverted);
    TRG_IN::pullup(g_trigger_input_pullup);
}

bool parse_input_source_token(const char *token, InputSource &source) {
    if (!token) {
        return false;
    }
    if (parser_command_equals(token, "VIN") ||
        parser_command_equals(token, "EXT") ||
        parser_command_equals(token, "EXTERNAL")) {
        source = InputSource::EXTERNAL;
        return true;
    }
    if (parser_command_equals(token, "REF+10") ||
        parser_command_equals(token, "REFP10") ||
        parser_command_equals(token, "REF10")) {
        source = InputSource::REF10;
        return true;
    }
    if (parser_command_equals(token, "REF+5") ||
        parser_command_equals(token, "REFP5") ||
        parser_command_equals(token, "REF5")) {
        source = InputSource::REF5;
        return true;
    }
    if (parser_command_equals(token, "REF+2.5") ||
        parser_command_equals(token, "REFP2.5") ||
        parser_command_equals(token, "REFP2_5") ||
        parser_command_equals(token, "REF2.5") ||
        parser_command_equals(token, "REF2_5")) {
        source = InputSource::REF2_5;
        return true;
    }
    if (parser_command_equals(token, "GND") || parser_command_equals(token, "REF0")) {
        source = InputSource::REF0;
        return true;
    }
    if (parser_command_equals(token, "REF-2.5") ||
        parser_command_equals(token, "REFM2.5") ||
        parser_command_equals(token, "REFM2_5")) {
        source = InputSource::REF_2_5;
        return true;
    }
    if (parser_command_equals(token, "REF-5") || parser_command_equals(token, "REFM5")) {
        source = InputSource::REF_5;
        return true;
    }
    if (parser_command_equals(token, "REF-10") || parser_command_equals(token, "REFM10")) {
        source = InputSource::REF_10;
        return true;
    }
    return false;
}

const char *input_source_to_token(InputSource source) {
    switch (source) {
        case InputSource::EXTERNAL: return "VIN";
        case InputSource::REF10: return "REF+10";
        case InputSource::REF5: return "REF+5";
        case InputSource::REF2_5: return "REF+2.5";
        case InputSource::REF0: return "GND";
        case InputSource::REF_2_5: return "REF-2.5";
        case InputSource::REF_5: return "REF-5";
        case InputSource::REF_10: return "REF-10";
        default: return "VIN";
    }
}

bool parse_window_plc_token(const char *token, WindowLength &window) {
    if (!token) {
        return false;
    }
    if (strcmp(token, "0.02") == 0) { window = WindowLength::PLC_0_02; return true; }
    if (strcmp(token, "0.1") == 0) { window = WindowLength::PLC_0_1; return true; }
    if (strcmp(token, "0.2") == 0) { window = WindowLength::PLC_0_2; return true; }
    if (strcmp(token, "0.5") == 0) { window = WindowLength::PLC_0_5; return true; }
    if (strcmp(token, "1") == 0) { window = WindowLength::PLC_1; return true; }
    if (strcmp(token, "2") == 0) { window = WindowLength::PLC_2; return true; }
    if (strcmp(token, "5") == 0) { window = WindowLength::PLC_5; return true; }
    if (strcmp(token, "10") == 0) { window = WindowLength::PLC_10; return true; }
    if (strcmp(token, "20") == 0) { window = WindowLength::PLC_20; return true; }
    if (strcmp(token, "50") == 0) { window = WindowLength::PLC_50; return true; }
    if (strcmp(token, "100") == 0) { window = WindowLength::PLC_100; return true; }
    if (strcmp(token, "200") == 0) { window = WindowLength::PLC_200; return true; }
    return false;
}

const char *window_plc_to_token(WindowLength window) {
    switch (window) {
        case WindowLength::PLC_0_02: return "0.02";
        case WindowLength::PLC_0_1: return "0.1";
        case WindowLength::PLC_0_2: return "0.2";
        case WindowLength::PLC_0_5: return "0.5";
        case WindowLength::PLC_1: return "1";
        case WindowLength::PLC_2: return "2";
        case WindowLength::PLC_5: return "5";
        case WindowLength::PLC_10: return "10";
        case WindowLength::PLC_20: return "20";
        case WindowLength::PLC_50: return "50";
        case WindowLength::PLC_100: return "100";
        case WindowLength::PLC_200: return "200";
        default: return "1";
    }
}

void clamp_measurement_buffer() {
    while (meas_buffer.size() >= SCPI_BUFFER_LIMIT) {
        Measurement discarded;
        if (!meas_buffer.get(discarded)) {
            break;
        }
    }
}

void capture_measurement_if_ready() {
    if (!g_trigger_armed) {
        return;
    }

    bool has_measurement = false;
    int32_t value = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (globals->status == Status::RESULT_AVAIL) {
            value = globals->negative_counts;
            globals->status = Status::CLEAN;
            has_measurement = true;
        }
    }

    if (!has_measurement) {
        return;
    }

    Measurement measurement;
    measurement.timestamp = Ticker::ptr ? Ticker::ptr->millis() : 0u;
    measurement.value = value;

    clamp_measurement_buffer();
    meas_buffer.put(measurement);
    g_last_measurement = measurement;
    g_has_last_measurement = true;

    if (g_samples_per_trigger > 0) {
        if (g_samples_remaining > 0) {
            --g_samples_remaining;
        }
        if (g_samples_remaining == 0) {
            g_trigger_armed = false;
            negative_counter.stop();
            window_counter.stop();
        }
    }
}

void handle_idn(const ScpiCommand &command, ByteStream &stream) {
    if (!command.is_query || command.argument_count != 0) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    stream_write_cstr(stream, "Uliano,AVR-Multislope,PROTO,0.1\n");
}

void handle_input(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        stream_write_cstr(stream, input_source_to_token(g_selected_input));
        stream_write_cstr(stream, "\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    InputSource input;
    if (!parse_input_source_token(command.arguments[0], input)) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    set_input_source(input);
    g_selected_input = input;
    scpi_reply_ok(stream);
}

void handle_window(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        stream_write_cstr(stream, window_plc_to_token(g_selected_window));
        stream_write_cstr(stream, "\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    WindowLength window;
    if (!parse_window_plc_token(command.arguments[0], window)) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    window_counter.set_window_length(window);
    g_selected_window = window;
    scpi_reply_ok(stream);
}

void handle_sample_count(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        if (g_samples_per_trigger == 0) {
            stream_write_cstr(stream, "INF\n");
            return;
        }
        stream_write_u32(stream, g_samples_per_trigger);
        stream_write_cstr(stream, "\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    const char *arg = command.arguments[0];
    if (!arg) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    if (parser_command_equals(arg, "INF") || strcmp(arg, "0") == 0) {
        g_samples_per_trigger = 0;
        scpi_reply_ok(stream);
        return;
    }

    unsigned long parsed = 0;
    if (!parser_parse_ulong(arg, parsed, 10)) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    if (parsed == 0 || parsed > SCPI_BUFFER_LIMIT) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    g_samples_per_trigger = static_cast<uint16_t>(parsed);
    scpi_reply_ok(stream);
}

void handle_trigger_input_polarity(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        stream_write_cstr(stream, g_trigger_input_inverted ? "INV\n" : "NORM\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    bool inverted = false;
    if (!parse_polarity_token(command.arguments[0], inverted)) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    g_trigger_input_inverted = inverted;
    apply_trigger_io_config();
    scpi_reply_ok(stream);
}

void handle_trigger_output_polarity(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        stream_write_cstr(stream, g_trigger_output_inverted ? "INV\n" : "NORM\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    bool inverted = false;
    if (!parse_polarity_token(command.arguments[0], inverted)) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    g_trigger_output_inverted = inverted;
    apply_trigger_io_config();
    scpi_reply_ok(stream);
}

void handle_trigger_input_pullup(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query) {
        if (command.argument_count != 0) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        stream_write_cstr(stream, g_trigger_input_pullup ? "ON\n" : "OFF\n");
        return;
    }

    if (command.argument_count != 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    bool enabled = false;
    if (!parse_enable_token(command.arguments[0], enabled)) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    g_trigger_input_pullup = enabled;
    apply_trigger_io_config();
    scpi_reply_ok(stream);
}

void handle_trigger(const ScpiCommand &command, ByteStream &stream) {
    if (command.is_query || command.argument_count != 0) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    negative_counter.reset();
    window_counter.reset();
    negative_counter.start();
    window_counter.start();
    g_trigger_armed = true;
    g_samples_remaining = g_samples_per_trigger;
    scpi_reply_ok(stream);
}

void handle_meas_ready(const ScpiCommand &command, ByteStream &stream) {
    if (!command.is_query || command.argument_count != 0) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    stream_write_cstr(stream, meas_buffer.size() ? "1\n" : "0\n");
}

void handle_meas_count(const ScpiCommand &command, ByteStream &stream) {
    if (!command.is_query || command.argument_count != 0) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    stream_write_u32(stream, meas_buffer.size());
    stream_write_cstr(stream, "\n");
}

void handle_meas_last(const ScpiCommand &command, ByteStream &stream) {
    if (!command.is_query || command.argument_count != 0) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    if (!g_has_last_measurement) {
        scpi_reply_error(stream, "NO_DATA");
        return;
    }
    scpi_reply_measurement(stream, g_last_measurement);
    stream_write_cstr(stream, "\n");
}

void handle_meas_read(const ScpiCommand &command, ByteStream &stream) {
    if (!command.is_query) {
        scpi_reply_error(stream, "ARG");
        return;
    }
    if (command.argument_count > 1) {
        scpi_reply_error(stream, "ARG");
        return;
    }

    uint16_t requested = 1;
    if (command.argument_count == 1) {
        unsigned long parsed = 0;
        if (!parser_parse_ulong(command.arguments[0], parsed, 10)) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        if (parsed == 0 || parsed > SCPI_MAX_READ_COUNT) {
            scpi_reply_error(stream, "ARG");
            return;
        }
        requested = static_cast<uint16_t>(parsed);
    }

    const uint16_t available = meas_buffer.size();
    if (available < requested) {
        scpi_reply_error(stream, "UNDERFLOW");
        return;
    }

    for (uint16_t i = 0; i < requested; ++i) {
        Measurement measurement;
        if (!meas_buffer.get(measurement)) {
            scpi_reply_error(stream, "UNDERFLOW");
            return;
        }
        g_last_measurement = measurement;
        g_has_last_measurement = true;

        if (i) {
            stream_write_cstr(stream, ",");
        }
        scpi_reply_measurement(stream, measurement);
    }
    stream_write_cstr(stream, "\n");
}

void handle_unknown(ByteStream &stream) {
    scpi_reply_error(stream, "CMD");
}

void scpi_command_handler(const ScpiCommand &command, ByteStream &stream) {
    static const ScpiRouter::Route routes[] = {
        { "*IDN", handle_idn },

        // Configuration
        { "ROUTE:INPUT", handle_input },
        { "ROUT:INP", handle_input },
        { "SENSE:WINDOW:PLC", handle_window },
        { "SENS:WIND:PLC", handle_window },
        { "SAMPLE:COUNT", handle_sample_count },
        { "SAMP:COUN", handle_sample_count },
        { "SAMP:COUNT", handle_sample_count },
        { "TRIGGER:INPUT:POLARITY", handle_trigger_input_polarity },
        { "TRIG:INP:POL", handle_trigger_input_polarity },
        { "TRIGGER:OUTPUT:POLARITY", handle_trigger_output_polarity },
        { "TRIG:OUTP:POL", handle_trigger_output_polarity },
        { "TRIGGER:INPUT:PULLUP", handle_trigger_input_pullup },
        { "TRIG:INP:PULL", handle_trigger_input_pullup },

        // Acquisition control
        { "INIT", handle_trigger },
        { "TRIGGER", handle_trigger },
        { "TRIGGER:IMMEDIATE", handle_trigger },
        { "TRIG", handle_trigger },
        { "TRIG:IMM", handle_trigger },

        // Data access
        { "DATA:AVAILABLE", handle_meas_ready },
        { "DATA:POINTS", handle_meas_count },
        { "FETCH:LAST", handle_meas_last },
        { "FETC:LAST", handle_meas_last },
        { "FETCH", handle_meas_read },
        { "FETC", handle_meas_read },
        { "READ", handle_meas_read }
    };

    const uint8_t route_count = static_cast<uint8_t>(sizeof(routes) / sizeof(routes[0]));
    if (!ScpiRouter::dispatch(command, routes, route_count, stream)) {
        handle_unknown(stream);
    }
}
}  // namespace

void scpi_init() {
    if (g_scpi_initialized) {
        return;
    }

    static ScpiEndpoint scpi_endpoint(usb, scpi_command_handler);
    g_parser_hub.add(scpi_endpoint);

    set_input_source(g_selected_input);
    window_counter.set_window_length(g_selected_window);
    apply_trigger_io_config();

    g_scpi_initialized = true;
}

void scpi_service() {
    if (!g_scpi_initialized) {
        return;
    }
    capture_measurement_if_ready();
    g_parser_hub.service_all();
}
