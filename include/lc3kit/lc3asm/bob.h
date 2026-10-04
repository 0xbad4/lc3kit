#pragma once


// Base Object
#include "error.h"
#include "ds.h"

namespace lc3kit::lasm
{
    #define IS_OK if (!ok()) return;
    #define MV std::move
    

    /**
     * @brief Base state and error-tracking object shared by assembler components.
     */
    class BaseObj {
        protected:
            bool m_is_running = false;
            bool m_ext_enabled = false;

            errors_t    m_errors {};

            /**
             * @brief Mark the object as active while a stage is running.
             */
            void start() {
                m_is_running = true;
            }

            /**
             * @brief Mark the object as inactive after a stage completes or aborts.
             */
            void stop() {
                m_is_running = false;
            }

            /**
             * @brief Record an assembler error at the given source position.
             *
             * @param et Error type to record.
             * @param p Source position associated with the error.
             */
            void report(error_type et, tpos p = {0, 0}) {
                m_errors.push_back(Error(et, p));
            }

            /**
             * @brief Clear recorded diagnostics and stop the object.
             */
            void reset() {
                m_errors.clear();
                stop();
            }

        public:
            /**
             * @brief Construct a base assembler object.
             *
             * @param en_ext `true` to enable extension opcodes by default.
             */
            BaseObj(bool en_ext = false) : m_ext_enabled(en_ext) {

            }

            /**
             * @brief Enable or disable the extension instruction set.
             *
             * @param en `true` to enable extension opcodes, `false` to disable them.
             *
             * @return `true` when the setting changed successfully, otherwise `false`
             * if the object is currently running.
             */
            bool set_ext_enabled(bool en=true) {
                if (!m_is_running) {
                    m_ext_enabled = en;
                    return true;
                }
                return false;
            }

            /**
             * @brief Check whether extension opcodes are enabled.
             *
             * @return `true` when extension instructions are enabled.
             */
            bool is_ext_enabled() const {
                return m_ext_enabled;
            }
            
            /**
             * @brief Access the collected diagnostics.
             *
             * @return Read-only list of recorded errors.
             */
            const errors_t& errors() const {
                return m_errors;
            }

            /**
             * @brief Check whether no assembler errors were recorded.
             *
             * @return `true` when the object is error-free.
             */
            bool ok() const {
                return m_errors.size() == 0;
            }

            /**
             * @brief Check whether the object is currently active.
             *
             * @return `true` when the object is running and has no recorded errors.
             */
            bool is_running() const {
                return m_is_running && ok();
            }
    };
} // namespace lc3kit::lasm
