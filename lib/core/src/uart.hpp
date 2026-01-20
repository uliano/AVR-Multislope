/*
 * uart.hpp
 *
 * Interrupt-driven UART with double-buffered RX and line-based parsing.
 *
 * Created: 11/3/2022 4:58:21 PM
 *  Author: uliano
 * Revised: 01/9/2026 - Bug fixes (parse functions, error counters) and documentation
 */


#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include "ring.hpp"
#include "utils.hpp"
#include "ticker.hpp"

const bool UART_ALTERNATE = true;
const bool UART_STANDARD = false;
const uint8_t MAX_TOKEN_LENGTH = 16;

/**
 * @brief Interrupt-driven UART with double-buffered RX and command parsing
 *
 * @tparam uart_num UART peripheral number (0-5)
 * @tparam alternate_pins Use alternate pin mapping (see datasheet)
 * @tparam rsize RX buffer size (power of 2)
 * @tparam tsize TX buffer size (power of 2)
 * @tparam RSizeT RX index type (uint8_t/uint16_t)
 * @tparam TSizeT TX index type (uint8_t/uint16_t)
 *
 * Key features:
 * - Double-buffered RX: One buffer receives while the other is being parsed
 * - Line-based protocol: Automatic line detection on '\n' (0x0A)
 * - Interrupt-driven TX: Non-blocking transmission with DRE interrupt
 * - Token parsing: Space-separated tokens with long/ulong conversion
 * - Error counters: TX/RX overflow and parse errors (wrap at 255)
 *
 * Architecture:
 *   m_input_buffer[0] ←→ swap ←→ m_input_buffer[1]
 *        ↑ ISR writes              ↑ Main loop parses
 *
 * Usage:
 *   Uart<2, UART_ALTERNATE> serial(115200);
 *
 *   // In ISR (interrupts.cpp):
 *   ISR(USART2_RXC_vect) { serial.rxc(); }
 *   ISR(USART2_DRE_vect) { serial.dre(); }
 *
 *   // In main loop:
 *   serial.receive_line();  // Start receiving
 *   if (serial.can_parse()) {
 *       long value;
 *       if (serial.parse_long(value)) {
 *           // Process value
 *       }
 *   }
 */
template <const int uart_num,  bool alternate_pins=false, const int rsize=256, const int tsize=512, typename RSizeT=uint16_t, typename TSizeT=uint16_t>
class Uart {
	static_assert((uart_num >= 0) & (uart_num <= 5), "uart can be only 0, 1, 2, 3, 4, or 5");
	static_assert(is_powerof2(rsize), "uart read buffer should be a power of 2");
	static_assert(is_powerof2(tsize), "uart write buffer should be a power of 2");
	private:
	volatile USART_t *regs;
	Ring<uint8_t, RSizeT, rsize> m_input_buffer[2];  // Double buffer: [0]=current RX, [1]=completed line
	Ring<uint8_t, TSizeT, tsize> m_output_buffer;    // TX buffer (single)
	uint8_t m_parse_buffer[MAX_TOKEN_LENGTH];        // Temp buffer for parsing tokens

	// Error counters (volatile uint8_t for atomic ISR access, wrap at 255)
	volatile uint8_t m_tx_errors;			// TX overflow: bytes lost due to full buffer
	volatile uint8_t m_rx_errors;			// RX overflow: bytes lost due to full buffer
	uint8_t m_parse_errors;					// Parse errors: token overflow or conversion failure
	volatile uint8_t m_incomplete_lines;	// Lines overwritten before parsing

	uint8_t m_current_input_buffer_index;   // Buffer currently receiving from ISR (0 or 1)
	uint8_t m_current_line_buffer_index;    // Buffer with completed line for parsing (0 or 1)
	bool m_receiving_line; 					// true = accepting line input
	bool m_can_parse;						// true = complete line ready to parse


	inline void input_buffer_swap(void){
		uint8_t temp = m_current_input_buffer_index;
		m_current_input_buffer_index = m_current_line_buffer_index;
		m_current_line_buffer_index = temp;
	}

	public:
	Uart(uint32_t baud=115200)
	{
		m_tx_errors = 0;
		m_rx_errors = 0;
		m_parse_errors = 0;
		m_incomplete_lines = 0;
		m_current_input_buffer_index = 0;
		m_current_line_buffer_index = 1;
		m_receiving_line = false;
		m_can_parse = false;
        if constexpr (uart_num == 0) {
            regs = &USART0;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEA |= PORTMUX_USART0_0_bm;
                PORTA.DIRSET = PIN4_bm; // Tx
                PORTA.DIRCLR = PIN5_bm; // Rx
                PORTA.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTA.DIRSET = PIN0_bm; // Tx
                PORTA.DIRCLR = PIN1_bm; // Rx
                PORTA.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        } else if constexpr (uart_num == 1) {
            regs = &USART1;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEA |= PORTMUX_USART1_0_bm;
                PORTC.DIRSET = PIN4_bm; // Tx
                PORTC.DIRCLR = PIN5_bm; // Rx
                PORTC.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTC.DIRSET = PIN0_bm; // Tx
                PORTC.DIRCLR = PIN1_bm; // Rx
                PORTC.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        }
        #ifdef USART2
        else if constexpr (uart_num == 2) {
            regs = &USART2;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEA |= PORTMUX_USART2_0_bm;
                PORTF.DIRSET = PIN4_bm; // Tx
                PORTF.DIRCLR = PIN5_bm; // Rx
                PORTF.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTF.DIRSET = PIN0_bm; // Tx
                PORTF.DIRCLR = PIN1_bm; // Rx
                PORTF.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        }
        #endif
        #ifdef USART3
        else if constexpr (uart_num == 3) {
            regs = &USART3;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEA |= PORTMUX_USART3_0_bm;
                PORTB.DIRSET = PIN4_bm; // Tx
                PORTB.DIRCLR = PIN5_bm; // Rx
                PORTB.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTB.DIRSET = PIN0_bm; // Tx
                PORTB.DIRCLR = PIN1_bm; // Rx
                PORTB.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        }
        #endif
        #ifdef USART4
        else if constexpr (uart_num == 4) {
            regs = &USART4;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEB |= PORTMUX_USART4_0_bm;
                PORTE.DIRSET = PIN4_bm; // Tx
                PORTE.DIRCLR = PIN5_bm; // Rx
                PORTE.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTE.DIRSET = PIN0_bm; // Tx
                PORTE.DIRCLR = PIN1_bm; // Rx
                PORTE.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        }
        #endif
        #ifdef USART5
        else if constexpr (uart_num == 5) {
            regs = &USART5;
            if constexpr (alternate_pins) {
                PORTMUX.USARTROUTEB |= PORTMUX_USART5_0_bm;
                PORTG.DIRSET = PIN4_bm; // Tx
                PORTG.DIRCLR = PIN5_bm; // Rx
                PORTG.OUTSET = PIN4_bm | PIN5_bm; // initial state HI
            } else {
                PORTG.DIRSET = PIN0_bm; // Tx
                PORTG.DIRCLR = PIN1_bm; // Rx
                PORTG.OUTSET = PIN0_bm | PIN1_bm; // initial state HI
            }
        }
        #endif

		regs->CTRLA |= USART_RXCIE_bm;
		// regs->BAUD = uint16_t(float(F_CPU * 64 / (16 * float(baud)) + 0.5));
		regs->BAUD = uint16_t(F_CPU * 4 / baud);
		regs->CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
		_delay_ms(10); // give a little time with TX high to have a proper start bit
	}

    uint8_t* parse_string(void) {
        uint8_t* string = m_parse_buffer;  // `string` is now properly a pointer to `m_parse_buffer`
        uint8_t len = 1;
        uint8_t c;

        // Skip leading spaces
        while (m_input_buffer[m_current_line_buffer_index].get(c) && c == 32);

        // Read token
        while (len < MAX_TOKEN_LENGTH && c != 32) {
            if (c == 0x0A) {  // End of line
                *string = 0;
                return m_parse_buffer;
            }
            *string++ = c;
            ++len;

            // Attempt to get the next character, break if empty
            if (!m_input_buffer[m_current_line_buffer_index].get(c)) {
                break;
            }
        }

        // If token length exceeded
        if (c != 32) {
            ++m_parse_errors;
            m_can_parse = false;
            return 0;
        }

        *string = 0;
        return m_parse_buffer;
    }

    
	/**
	 * @brief Parse next token as signed long (decimal base 10)
	 *
	 * Extracts next space-separated token and converts to long.
	 *
	 * @param value Reference to store parsed value
	 * @return true if parsing succeeded, false on error
	 *
	 * Error conditions:
	 * - Token too long (>16 chars)
	 * - No digits found (end == str)
	 * - Extra characters after number (*end != '\0')
	 */
	bool parse_long(long & value){
		uint8_t* str = parse_string();
		if (!str) {
			return false;  // parse_string already set error
		}
		char *end;
		value = strtol((char*)str, &end, 10);
		if (end == (char*)str || *end != '\0') {
			++m_parse_errors;
			m_can_parse = false;
			return false;
		}
		return true;
	}

	/**
	 * @brief Parse next token as unsigned long (hexadecimal base 16)
	 *
	 * Extracts next space-separated token and converts to unsigned long.
	 *
	 * @param value Reference to store parsed value
	 * @return true if parsing succeeded, false on error
	 *
	 * Error conditions:
	 * - Token too long (>16 chars)
	 * - No digits found (end == str)
	 * - Extra characters after number (*end != '\0')
	 */
	bool parse_ulong(unsigned long & value){
		uint8_t* str = parse_string();
		if (!str) {
			return false;  // parse_string already set error
		}
		char *end;
		value = strtoul((char*)str, &end, 16);
		if (end == (char*)str || *end != '\0') {
			++m_parse_errors;
			m_can_parse = false;
			return false;
		}
		return true;
	}

    /**
     * @brief TX Data Register Empty interrupt handler
     *
     * Call this from ISR(USARTx_DRE_vect).
     * Sends next byte from TX buffer, disables interrupt when empty.
     */
    inline void dre(void) {
        uint8_t c;
        if (m_output_buffer.get(c)) {  // Attempt to get a byte
            regs->TXDATAL = c;
            if (!m_output_buffer.size()) {  // Disable DRE interrupt if empty
                regs->CTRLA &= ~USART_DREIE_bm;
            }
        } else {
            // Buffer was empty, so disable the DRE interrupt
            regs->CTRLA &= ~USART_DREIE_bm;
        }
    }

	/**
	 * @brief RX Complete interrupt handler
	 *
	 * Call this from ISR(USARTx_RXC_vect).
	 *
	 * Receives byte and adds to current input buffer. When '\n' (0x0A) is received
	 * during line mode, swaps buffers so main loop can parse the completed line
	 * while ISR continues receiving into the other buffer.
	 *
	 * Double-buffer algorithm:
	 * 1. Receive bytes into m_input_buffer[m_current_input_buffer_index]
	 * 2. On '\n': swap buffer indices
	 * 3. Main loop parses m_input_buffer[m_current_line_buffer_index]
	 * 4. ISR continues receiving into the other buffer
	 */
	inline void rxc(void) {
		uint8_t value =regs->RXDATAL;
		if (m_input_buffer[m_current_input_buffer_index].is_full()) {
			++m_rx_errors;  // Atomic - uint8_t on 8-bit AVR
		} else {
			m_input_buffer[m_current_input_buffer_index].put(value);
			if (m_receiving_line && value == 0x0a) {
				if (m_can_parse) ++m_incomplete_lines;  // Previous line not parsed yet
				input_buffer_swap();
				m_receiving_line = false;
				m_can_parse = true;
			}
		}
	}



	inline bool can_parse(void) {
		return m_can_parse;
	}

	inline void receive_line(void) {
		m_receiving_line = true;
	}
	
    inline bool receive_byte(uint8_t *b) {
        return m_input_buffer[m_current_input_buffer_index].get(*b);
    }

	inline uint8_t send_byte(const uint8_t b){
		if (m_output_buffer.is_full()) {
			++m_tx_errors;  // Atomic - uint8_t on 8-bit AVR
			return 0;
		}
		m_output_buffer.put(b);
		regs->CTRLA |= USART_DREIE_bm;
		return 1;
	}
	
	uint8_t send_buffer(const uint8_t *buffer, const uint8_t len){  
		uint8_t remaining = len;
		while(remaining)
		{
			const uint8_t result = send_byte(*buffer);
			if (result) {
				++buffer;
				--remaining;
			}
			else {
				break;
			}
		}
		return len - remaining;
	}

	inline void newline(const bool cr=false) {
		if (cr) print("\r\n");
		else print("\n");
	}

	void print(const char *string) {
		while(*string) {
			send_byte(*string);
			++string;
		}
	}

	void print(const double val, int8_t width, uint8_t precision) {
		uint8_t buffer[16];
		dtostrf(val, width, precision, buffer);
		print(buffer);
	}

	void print(const double val, uint8_t precision=3) {
		uint8_t buffer[16];
		dtostre(val, buffer, precision, DTOSTR_ALWAYS_SIGN);
		print(buffer);
	}

	void print(const uint32_t val, int8_t radix){
		char buffer[12];
		if (radix==16){		
			buffer[0] = '0';
			buffer[1] = 'x';
			ultoa(val, &buffer[2], radix);
		} else {
			ultoa(val, buffer, radix);
		}
		print(buffer);
	}

	void print(const int32_t val){
		char buffer[12];
		ltoa(val, buffer, 0x0a);
		print(buffer);
	}

	void print(const uint16_t val, int8_t radix){
		char buffer[8];
		if (radix==16){		
			buffer[0] = '0';
			buffer[1] = 'x';
			ultoa(val, &buffer[2], radix);
		} else {
			ultoa(val, buffer, radix);
		}
		print(buffer);
	}

	void print(const int16_t val){
		char buffer[8];
		itoa(val, buffer, 0x0a);
		print(buffer);
	}

	inline void print(const uint8_t u, int8_t radix) {
		print(static_cast<uint16_t>(u), radix);
	}

	inline void print(const int8_t u) {
		print(static_cast<int16_t>(u));
	}

	void print(TimeStamp & t){
		print(t.seconds, 10);
		print("s.");
		print(t.ticks, 10);
		print("t");
	}
	
}; //Uart



