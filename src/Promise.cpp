#include "Promise.h"
#include "State.h"

namespace swt {

// void specialization
Promise<void>::Promise() : m_state(std::make_shared<State<std::monostate>>()) {}

void Promise<void>::set_value() {
    m_state->setValue(std::monostate{});
}

void Promise<void>::set_exception(std::exception_ptr exception) {
    m_state->setException(exception);
}

} // namespace swt