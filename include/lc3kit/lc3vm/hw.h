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

            /**
             * @brief Feed a character to the keyboard device.
             *
             * Writes a character into the keyboard data register and sets the
             * ready bit to signal that input is available.
             *
             * @param c Character to feed to the keyboard.
             *
             * @return `error_type::NO_ERROR` on success; otherwise, the memory
             * configuration error that prevented the write.
             *
             * @warning This method has no locking or atomics of its own. If you feed
             *          the keyboard from a different thread than the one calling
             *          `vm.step()` or `vm.run()`, you are responsible for the
             *          synchronization.
             */
            error_type emit_char(char c) {
                if (!is_memory_valid()) {
                    return error_type::INVALID_MEMORY;
                }

                *m_kbdr_ptr = (std_word_t)c;
                *m_kbsr_ptr |= (1 << 15); // Set bit 15

                return error_type::NO_ERROR;
            }

            /**
             * @brief Attach the keyboard device to its memory-mapped registers.
             *
             * @param kbsr_ptr Pointer to the KBSR register in VM memory.
             * @param kbdr_ptr Pointer to the KBDR register in VM memory.
             */
            void set_addr(std_word_t *kbsr_ptr, std_word_t *kbdr_ptr) {
                m_kbsr_ptr = kbsr_ptr;
                m_kbdr_ptr = kbdr_ptr;
            }

            /**
             * @brief Check whether the keyboard device is properly mapped.
             *
             * @return `true` once both register pointers have been configured.
             */
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

            /**
             * @brief Attach the display device to its memory-mapped registers.
             *
             * @param dsr_ptr Pointer to the DSR register in VM memory.
             * @param ddr_ptr Pointer to the DDR register in VM memory.
             */
            void set_addr(std_word_t* dsr_ptr, std_word_t* ddr_ptr) {
                m_dsr_ptr = dsr_ptr;
                m_ddr_ptr = ddr_ptr;
                ready();
            }

            /**
             * @brief Register a callback that fires on each display write.
             *
             * @param callback Function invoked after the DDR register is written.
             * @param userdata Optional user data pointer passed to the callback.
             */
            void set_callback(display_callback callback, void* userdata = nullptr) {
                m_callback = callback;
                m_userdata = userdata;
            }

            /**
             * @brief Trigger the display callback for the most recent DDR write.
             *
             * This is normally invoked by the VM after a write to the display data
             * register.
             */
            void on_write() {
                if (m_callback && m_ddr_ptr != nullptr) {
                    m_callback(last_char(), m_userdata);
                }
            }

            /**
             * @brief Check whether the display is marked ready for output.
             *
             * @return `true` when the DSR ready bit is set, otherwise `false`.
             */
            bool is_ready() const {
                return m_dsr_ptr && (*m_dsr_ptr & (1 << 15));
            }

            /**
             * @brief Check whether the display device is properly mapped.
             *
             * @return `true` once both display register pointers have been set.
             */
            bool is_memory_valid() {
                return (m_dsr_ptr != nullptr) && (m_ddr_ptr != nullptr);
            }

            /**
             * @brief Read the most recent byte written to the display data register.
             *
             * @return Character stored in the low byte of DDR.
             */
            char last_char() {
                return (char)(*m_ddr_ptr & 0xFF);
            }

            /**
             * @brief Set or clear the display ready bit.
             *
             * @param r `true` to mark the display ready; `false` to clear it.
             */
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
