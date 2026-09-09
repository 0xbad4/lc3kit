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

            void start() {
                m_is_running = true;
            }

            void stop() {
                m_is_running = false;
            }

            void report(error_type et, tpos p = {0, 0}) {
                m_errors.push_back(Error(et, p));
            }

            void reset() {
                m_errors.clear();
                stop();
            }

        public:
            BaseObj(bool en_ext = false) : m_ext_enabled(en_ext) {

            }

            bool set_ext_enabled(bool en=true) {
                if (!m_is_running) {
                    m_ext_enabled = en;
                    return true;
                }
                return false;
            }

            bool is_ext_enabled() const {
                return m_ext_enabled;
            }
            
            const errors_t& errors() const {
                return m_errors;
            }

            bool ok() const {
                return m_errors.size() == 0;
            }

            bool is_running() const {
                return m_is_running && ok();
            }
    };
} // namespace lc3kit::lasm
