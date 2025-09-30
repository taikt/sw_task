#include "Message.h"
#include "Handler.h"

Message::Message() :
    what(0),
    arg1(0),
    arg2(0),
    arg3(0),
    obj(NULL),
    obj_size(0),
    whenUs(0)
{
    mNextMessage = NULL;
    mHandler = NULL;
    //post = NULL;
    spRef = NULL;
}

Message::Message(Message& other) :
    what(0),
    arg1(0),
    arg2(0),
    arg3(0),
    obj(NULL),
    obj_size(0),
    whenUs(0),
    spRef(NULL)
{
    setTo(other);
}

Message::~Message()
{
}

std::shared_ptr<Message> Message::obtain()
{
	auto msg = std::make_shared<Message>();
	return msg;
}

std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    return message;
}

std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    return message;
}

std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , int32_t obtain_arg1)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->arg1 = obtain_arg1;
    return message;
}

std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , void* obtain_obj)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->obj = obtain_obj;
    return message;
}


std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , int32_t obtain_arg1, int32_t obtain_arg2)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->arg1 = obtain_arg1;
    message->arg2 = obtain_arg2;
    return message;
}


std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , int32_t obtain_arg1, int32_t obtain_arg2, void* obtain_obj)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->arg1 = obtain_arg1;
    message->arg2 = obtain_arg2;
    message->obj = obtain_obj;
    return message;
}


std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , int32_t obtain_arg1, int32_t obtain_arg2, int32_t obtain_arg3)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->arg1 = obtain_arg1;
    message->arg2 = obtain_arg2;
    message->arg3 = obtain_arg3;
    return message;
}

std::shared_ptr<Message> Message::obtain(const Message* message)
{
    std::shared_ptr<Message> dupMessage = obtain();
    dupMessage->what = message->what;
    dupMessage->arg1 = message->arg1;
    dupMessage->arg2 = message->arg2;
    dupMessage->arg3 = message->arg3;
    dupMessage->obj = message->obj;
    dupMessage->spRef = message->spRef;
    dupMessage->whenUs = 0;
    dupMessage->mHandler = message->mHandler;
    dupMessage->mNextMessage = NULL;
    return dupMessage;
}


std::shared_ptr<Message> Message::obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what
                               , std::shared_ptr<RefBase> obtain_spRef)
{
    std::shared_ptr<Message> message = obtain();
    message->mHandler = handler;
    message->what = obtain_what;
    message->spRef= obtain_spRef;
    return message;
}




bool Message::sendToTarget()
{
    if (mHandler != NULL) {
        try {
            // ✅ SAFE: Sử dụng shared_from_this()
            return mHandler->sendMessage(shared_from_this());
        } catch (const std::bad_weak_ptr& e) {
            // Message không được tạo từ shared_ptr
            std::cerr << "Error: Message not managed by shared_ptr" << std::endl;
            return false;
        }
    } else {
        //cout<<"sendToTarget handler is NULL\n";
        return false;
    }
}

std::shared_ptr<Message> Message::dup() const
{
    return Message::obtain(this);
}

void Message::clear()
{
    what = 0;
    arg1 = 0;
    arg2 = 0;
    arg3 = 0;
    obj = NULL;
    obj_size = 0;
    whenUs = 0;
    mNextMessage = NULL;
    mHandler = NULL;
    //post = NULL;
    spRef = NULL;
}


Message& Message::operator=(const Message& other)
{
    setTo(other);
    return *this;
}

void Message::setTo(const Message& other)
{
    if(this != &other)
    {
        what = other.what;
        arg1 = other.arg1;
        arg2 = other.arg2;
        arg3 = other.arg3;
        obj = other.obj;
        obj_size = other.obj_size;
        whenUs = other.whenUs;
        //post = other.post;
        spRef = other.spRef;
    }
}


