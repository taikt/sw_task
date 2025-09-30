#ifndef MESSAGE_H
#define MESSAGE_H

#include <memory>
#include <cstdint>
#include "Refbase.h"
class Handler;

class Message : public std::enable_shared_from_this<Message>
{
public:
    Message();
    Message(Message& other);
    virtual ~Message();

    static std::shared_ptr<Message> obtain();
    static std::shared_ptr<Message> obtain(const Message* message);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 void* obtain_obj);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2, void* obtain_obj);
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2, int32_t obtain_arg3);
	
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 std::shared_ptr<RefBase> obtain_spRef);
	
	/*
	template<typename T>
	static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 std::shared_ptr<T> obtain_spRef);
	*/
    bool sendToTarget();

    std::shared_ptr<Message> dup() const;

    template<typename T>
    void getObject(std::shared_ptr<T>& obj)
    {
        obj = static_cast<T*>( spRef.get() );
    }


protected:
    Message& operator=(const Message& other);
    void clear();

public:
    void setTo(const Message& other);
    std::shared_ptr<Handler> mHandler;
    std::shared_ptr<Message> mNextMessage;

public:
    int32_t what;
    int32_t arg1;
    int32_t arg2;
    int32_t arg3;

    void* obj;
    ssize_t obj_size;
    std::shared_ptr<RefBase> spRef;

private:
    int64_t whenUs;

    friend class Handler;
    friend class EventQueue;
    friend class SLLooper;
};

#endif // MESSAGE_H