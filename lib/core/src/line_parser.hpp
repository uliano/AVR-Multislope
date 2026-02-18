#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bytestream.hpp"

/**
 * @brief Generic parsed command representation.
 *
 * The command string and argument pointers refer to memory in the line buffer
 * owned by the stream endpoint. Consume them before the next poll cycle.
 */
template <uint8_t max_arguments = 8>
struct ParsedCommand {
    char *command;
    char *arguments[max_arguments];
    uint8_t argument_count;
    bool is_query;

    inline void clear() {
        command = nullptr;
        argument_count = 0;
        is_query = false;
        for (uint8_t i = 0; i < max_arguments; ++i) {
            arguments[i] = nullptr;
        }
    }
};

inline char parser_ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return static_cast<char>(c - ('a' - 'A'));
    }
    return c;
}

inline void parser_upper_inplace(char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        *text = parser_ascii_upper(*text);
        ++text;
    }
}

inline bool parser_parse_long(const char *text, long &value, uint8_t base = 10) {
    if (!text) {
        return false;
    }
    char *end;
    value = strtol(text, &end, base);
    return (end != text) && (*end == '\0');
}

inline bool parser_parse_ulong(const char *text, unsigned long &value, uint8_t base = 16) {
    if (!text) {
        return false;
    }
    char *end;
    value = strtoul(text, &end, base);
    return (end != text) && (*end == '\0');
}

/**
 * @brief Non-owning tokenizer on top of a mutable C string.
 *
 * Tokenization happens in place by replacing separators with '\0'.
 */
class TokenCursor {
private:
    char *m_next;
    bool m_comma_is_separator;

    inline bool is_separator(char c) const {
        if (c == ' ' || c == '\t') {
            return true;
        }
        return m_comma_is_separator && c == ',';
    }

public:
    explicit TokenCursor(char *line, bool comma_is_separator = false)
        : m_next(line), m_comma_is_separator(comma_is_separator) {}

    bool next(char *&token) {
        token = nullptr;
        if (!m_next) {
            return false;
        }

        while (*m_next && is_separator(*m_next)) {
            ++m_next;
        }
        if (!*m_next) {
            return false;
        }

        token = m_next;
        while (*m_next && !is_separator(*m_next)) {
            ++m_next;
        }
        if (*m_next) {
            *m_next = '\0';
            ++m_next;
        }
        return true;
    }
};

/**
 * @brief Collects newline-terminated lines from a ByteStream.
 *
 * - '\r' is ignored
 * - '\n' terminates the current line
 * - on overflow, bytes are dropped until next '\n'
 */
template <uint8_t max_line_length = 96>
class LineReceiver {
    static_assert(max_line_length >= 4, "line buffer should be at least 4 chars");

private:
    ByteStream &m_stream;
    char m_line[max_line_length]{};
    uint8_t m_line_length{0};
    bool m_has_line{false};
    bool m_drop_until_eol{false};
    uint8_t m_overflow_count{0};

public:
    explicit LineReceiver(ByteStream &stream) : m_stream(stream) {}

    bool poll() {
        if (m_has_line) {
            return true;
        }

        uint8_t byte;
        while (m_stream.read_byte(byte)) {
            if (m_drop_until_eol) {
                if (byte == '\n') {
                    m_drop_until_eol = false;
                    m_line_length = 0;
                }
                continue;
            }

            if (byte == '\r') {
                continue;
            }

            if (byte == '\n') {
                m_line[m_line_length] = '\0';
                m_has_line = true;
                return true;
            }

            if (m_line_length + 1 >= max_line_length) {
                ++m_overflow_count;
                m_line_length = 0;
                m_has_line = false;
                m_drop_until_eol = true;
                continue;
            }

            m_line[m_line_length++] = static_cast<char>(byte);
        }

        return false;
    }

    inline bool has_line() const {
        return m_has_line;
    }

    inline char *line() {
        return m_has_line ? m_line : nullptr;
    }

    inline const char *line() const {
        return m_has_line ? m_line : nullptr;
    }

    inline uint8_t line_length() const {
        return m_line_length;
    }

    inline void consume_line() {
        m_has_line = false;
        m_line_length = 0;
    }

    inline uint8_t overflow_count() const {
        return m_overflow_count;
    }

    inline void clear_counters() {
        m_overflow_count = 0;
    }
};

/**
 * @brief Console-style parser.
 *
 * Command syntax: "CMD arg1 arg2"
 * Separator: spaces/tabs.
 */
template <uint8_t max_arguments = 8>
class ConsoleCommandParser {
public:
    using CommandType = ParsedCommand<max_arguments>;

    static bool parse(char *line, CommandType &out_command) {
        out_command.clear();
        if (!line) {
            return false;
        }

        TokenCursor cursor(line, false);
        char *token = nullptr;
        if (!cursor.next(token)) {
            return false;
        }

        parser_upper_inplace(token);
        out_command.command = token;

        while (cursor.next(token)) {
            if (out_command.argument_count >= max_arguments) {
                return false;
            }
            out_command.arguments[out_command.argument_count++] = token;
        }

        return true;
    }
};

/**
 * @brief Minimal SCPI-like parser.
 *
 * Command syntax: ":SUB:SYSTEM:CMD? arg1,arg2"
 * Separators: spaces, tabs, commas.
 * Normalization:
 * - command is uppercased in place
 * - optional leading ':' is removed from command pointer
 * - trailing '?' sets is_query and is removed
 */
template <uint8_t max_arguments = 8>
class ScpiCommandParser {
public:
    using CommandType = ParsedCommand<max_arguments>;

    static bool parse(char *line, CommandType &out_command) {
        out_command.clear();
        if (!line) {
            return false;
        }

        TokenCursor cursor(line, true);
        char *token = nullptr;
        if (!cursor.next(token)) {
            return false;
        }

        parser_upper_inplace(token);
        if (token[0] == ':') {
            ++token;
        }
        if (!*token) {
            return false;
        }

        const uint8_t len = static_cast<uint8_t>(strlen(token));
        if (len && token[len - 1] == '?') {
            token[len - 1] = '\0';
            out_command.is_query = true;
        }
        if (!*token) {
            return false;
        }

        out_command.command = token;

        while (cursor.next(token)) {
            if (out_command.argument_count >= max_arguments) {
                return false;
            }
            out_command.arguments[out_command.argument_count++] = token;
        }

        return true;
    }
};

inline bool parser_command_equals(const char *left, const char *right) {
    if (!left || !right) {
        return false;
    }
    while (*left && *right) {
        if (parser_ascii_upper(*left) != parser_ascii_upper(*right)) {
            return false;
        }
        ++left;
        ++right;
    }
    return (*left == '\0') && (*right == '\0');
}

/**
 * @brief Small static command router for parsed commands.
 *
 * Route names should be provided in canonical uppercase form.
 */
template <uint8_t max_arguments = 8>
class CommandRouter {
public:
    using CommandType = ParsedCommand<max_arguments>;
    using Handler = void (*)(const CommandType &, ByteStream &);

    struct Route {
        const char *command;
        Handler handler;
    };

    static bool dispatch(
        const CommandType &command,
        const Route *routes,
        uint8_t route_count,
        ByteStream &stream
    ) {
        if (!command.command || !routes) {
            return false;
        }

        for (uint8_t i = 0; i < route_count; ++i) {
            if (!routes[i].command || !routes[i].handler) {
                continue;
            }
            if (parser_command_equals(command.command, routes[i].command)) {
                routes[i].handler(command, stream);
                return true;
            }
        }
        return false;
    }
};

/**
 * @brief Endpoint interface for multiplexing parser services.
 */
class IParserEndpoint {
public:
    virtual void service() = 0;
    virtual ~IParserEndpoint() = default;
};

/**
 * @brief Binds one ByteStream to one parser implementation and one callback.
 *
 * ParserT requirements:
 * - typedef CommandType
 * - static bool parse(char *line, CommandType &out_command)
 */
template <typename ParserT, uint8_t max_line_length = 96>
class StreamParserEndpoint : public IParserEndpoint {
public:
    using CommandType = typename ParserT::CommandType;
    using CommandHandler = void (*)(const CommandType &, ByteStream &);

private:
    ByteStream &m_stream;
    LineReceiver<max_line_length> m_receiver;
    CommandHandler m_handler;
    uint8_t m_parse_errors{0};

public:
    StreamParserEndpoint(ByteStream &stream, CommandHandler handler)
        : m_stream(stream), m_receiver(stream), m_handler(handler) {}

    void service() override {
        while (m_receiver.poll()) {
            CommandType command;
            const bool parsed = ParserT::parse(m_receiver.line(), command);
            if (parsed) {
                if (m_handler) {
                    m_handler(command, m_stream);
                }
            } else {
                ++m_parse_errors;
            }
            m_receiver.consume_line();
        }
    }

    inline uint8_t parse_errors() const {
        return m_parse_errors;
    }

    inline uint8_t line_overflows() const {
        return m_receiver.overflow_count();
    }

    inline void clear_counters() {
        m_parse_errors = 0;
        m_receiver.clear_counters();
    }
};

/**
 * @brief Polls a static set of parser endpoints.
 *
 * Typical use is one endpoint for SCPI and one for console diagnostics.
 */
template <uint8_t max_endpoints = 4>
class ParserHub {
private:
    IParserEndpoint *m_endpoints[max_endpoints]{};
    uint8_t m_count{0};

public:
    bool add(IParserEndpoint &endpoint) {
        if (m_count >= max_endpoints) {
            return false;
        }
        m_endpoints[m_count++] = &endpoint;
        return true;
    }

    inline void service_all() {
        for (uint8_t i = 0; i < m_count; ++i) {
            m_endpoints[i]->service();
        }
    }

    inline uint8_t count() const {
        return m_count;
    }
};
