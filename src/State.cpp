#include "State.h"
#include "SLLooper.h"

namespace swt {

// Specialization implementations for State<std::monostate>
void State<std::monostate>::setValue(std::monostate &&value) {
    m_value = std::move(value);
    if (m_continuation && m_looper) {
        executeContination(std::move(*m_value));
    } else {
        // std::cout << "m_continuation or m_looper is empty\n";
    }
}

void State<std::monostate>::setException(std::exception_ptr exception) {
    m_exception = exception;
    if (m_errorHandler && m_errorLooper) {
        executeErrorHandler(m_exception);
    } else {
        std::cout << "m_errorHandler or m_errorLooper is empty\n";
    }
}

void State<std::monostate>::executeContination(std::monostate value) {
    if (m_looper && m_continuation) {
        m_looper->post([continuation = m_continuation]() mutable {
            continuation();
        });
    }
}

void State<std::monostate>::executeErrorHandler(std::exception_ptr exception) {
    if (m_errorLooper && m_errorHandler) {
        m_errorLooper->post([errorHandler = m_errorHandler, exception]() mutable {
            errorHandler(exception);
        });
    }
}

} // namespace swt