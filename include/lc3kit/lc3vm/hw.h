#pragma once

#include <functional>
#include "enums.h"

// Hardware

namespace lc3kit::vm {
    /**
     * @brief Callback invoked when a display character is written.
     *
     * @param c Printed character.
     * @param userdata Optional user data pointer passed during registration.
     */
    using display_callback = std::function<void(char c, void* userdata)>;

    /**
     * @brief Keyboard device interface mapped to the LC-3 keyboard memory registers.
     */
    typedef class Keyboard {
        public:
            Keyboard() = default;

            error_type emit_char(char c) {
                if (!is_memory_valid()) {
                    return error_type::INVALID_MEMORY;
                }

                *m_kbdr_ptr = (std_word_t)c;
                *m_kbsr_ptr |= (1 << 15); // Set bit 15

                return error_type::NO_ERROR;
            }

            void set_addr(std_word_t *kbsr_ptr, std_word_t *kbdr_ptr) {
                m_kbsr_ptr = kbsr_ptr;
                m_kbdr_ptr = kbdr_ptr;
            }

            bool is_memory_valid() {
                return (m_kbsr_ptr != nullptr) && (m_kbdr_ptr != nullptr);
            }


        protected:
            std_word_t *m_kbsr_ptr = nullptr;
            std_word_t *m_kbdr_ptr = nullptr;

        } vm_keyboard;

    /**
     * @brief Display device interface mapped to the LC-3 display memory registers.
     */
    typedef class Display {
        public:
            Display() = default;

            void set_addr(std_word_t* dsr_ptr, std_word_t* ddr_ptr) {
                m_dsr_ptr = dsr_ptr;
                m_ddr_ptr = ddr_ptr;
                ready();
            }

            void set_callback(display_callback callback, void* userdata = nullptr) {
                m_callback = callback;
                m_userdata = userdata;
            }

            void on_write() {
                if (m_callback && m_ddr_ptr != nullptr) {
                    m_callback(last_char(), m_userdata);
                }
            }

            bool is_ready() const {
                return m_dsr_ptr && (*m_dsr_ptr & (1 << 15));
            }

            bool is_memory_valid() {
                return (m_dsr_ptr != nullptr) && (m_ddr_ptr != nullptr);
            }

            char last_char() {
                return (char)(*m_ddr_ptr & 0xFF);
            }

            void ready(bool r=true) {
                if (m_dsr_ptr) {
                    if (r) {
                        *m_dsr_ptr |= (1 << 15);   // Set READY
                    } else {
                        *m_dsr_ptr &= ~(1 << 15);  // Clear READY
                    }
                }
            }

        private:
            std_word_t*           m_dsr_ptr  = nullptr;
            std_word_t*           m_ddr_ptr  = nullptr;
            display_callback      m_callback = nullptr;
            void*                 m_userdata = nullptr;

    } vm_display;
}
