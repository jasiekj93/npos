#pragma once

/**
 * @file MessagePacket.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <etl/type_traits.h>
#include <etl/type_list.h>
#include <etl/utility.h>
#include <etl/largest.h>

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc
{
    template <typename... TMessageTypes>
    class MessagePacket
    {
    protected:
        template <typename T>
        static constexpr bool isMessagePacket = etl::is_same_v<etl::remove_const_t<etl::remove_reference_t<T>>, npos::ipc::MessagePacket<TMessageTypes...>>;

        template <typename T>
        static constexpr bool isInMessageList = etl::is_one_of_v<etl::remove_const_t<etl::remove_reference_t<T>>, TMessageTypes...>;

        template <typename T>
        static constexpr bool isMessage = etl::is_same_v<etl::remove_const_t<etl::remove_reference_t<T>>, npos::ipc::Message>;

    public:
        using MessageTypes = etl::type_list<TMessageTypes...>;

        constexpr MessagePacket() noexcept
            : valid(false)
        {
        }

        template <typename T, typename = typename etl::enable_if< isMessage<T> || isInMessageList<T>, int>::type>
        explicit MessagePacket(T&& msg)
            : valid(true)
        {
            if constexpr (isMessage<T>)
            {
                if (accepts(msg))
                {
                    addNewMessage(etl::forward<T>(msg));
                valid = true;
                }
                else
                {
                valid = false;
                }

                ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));
            }
            else if constexpr (isInMessageList<T>)
                add_new_message_type<T>(etl::forward<T>(msg));
            else
            {
                static_assert(isInMessageList<T>, "Message not in packet type list");
            }
        }

        MessagePacket(const MessagePacket& other)
        {
            valid = other.isValid();

            if(valid)
                addNewMessage(other.get());
        }

        MessagePacket(const MessagePacket&& other)
        {
            valid = other.isValid();

            if(valid)
                addNewMessage(etl::move(other.get()));
        }

        void copy(const MessagePacket& other)
        {
            valid = other.isValid();

            if (valid)
                addNewMessage(other.get());
        }

        void copy(MessagePacket&& other)
        {
            valid = other.isValid();

            if (valid)
                addNewMessage(etl::move(other.get()));
        }

        MessagePacket& operator=(const MessagePacket& other)
        {
            deleteCurrentMessage();
            copy(other);

            return *this;
        }

        MessagePacket& operator=(MessagePacket&& other)
        {
            deleteCurrentMessage();
            copy(etl::move(other));

            return *this;
        }

        ~MessagePacket()
        {
            deleteCurrentMessage();
        }

        Message& get()
        {
            return *static_cast<npos::ipc::Message*>(data);
        }

        const Message& get() const
        {
            return *static_cast<const npos::ipc::Message*>(data);
        }

        bool isValid() const
        {
            return valid;
        }

        static constexpr bool accepts(npos::ipc::Message::Id id) 
        {
            return (acceptsMessage<TMessageTypes::Id>(id) || ...);
        }

        static constexpr bool accepts(const npos::ipc::Message& msg)
        {
            return accepts(msg.getId());
        }

        template <npos::ipc::Message::Id Id>
        static constexpr bool accepts()
        {
            return (acceptsMessage<TMessageTypes::Id>(id) || ...);
        }
        
        template <typename TMessage>
        static constexpr typename etl::enable_if<etl::is_base_of<npos::ipc::Message, TMessage>::value, bool>::type accepts()
        {
            return accepts<TMessage::Id>();
        }

        enum
        {
            SIZE = etl::largest<TMessageTypes...>::size,
            ALIGNMENT = etl::largest<TMessageTypes...>::alignment
        };
                
    protected:
        template <npos::ipc::Message::Id Id1, npos::ipc::Message::Id Id2>
        static bool acceptsMessage()
        {
            return Id1 == Id2;
        }

        template <npos::ipc::Message::Id Id1>
        static bool acceptsMessage(npos::ipc::Message::Id id2)
        {
            return Id1 == id2;
        }

        void deleteCurrentMessage()
        {
            if (valid)
            {
                npos::ipc::Message* pointer = static_cast<npos::ipc::Message*>(data);
                pointer->~Message();
            }
        }

        void addNewMessage(const npos::ipc::Message& msg)
        {
            (addNewMessageType<TMessageTypes>(msg) || ...);
        }

        void addNewMessage(etl::imessage&& msg)
        {
            (addNewMessageType<TMessageTypes>(etl::move(msg)) || ...);
        }

        template <typename TMessage>
        etl::enable_if_t< etl::is_one_of_v<etl::remove_const_t<etl::remove_reference_t<TMessage>>, TMessageTypes...>, void>
        addNewMessageType(TMessage&& msg)
        {
            void* placement = data;
            new (placement) etl::remove_reference_t<TMessage>((etl::forward<TMessage>(msg)));
        }

        template <typename TType>
        bool addNewMessageType(const npos::ipc::Message& msg)
        {
            if (TType::ID == msg.getId())
            {
                void* placement = data;
                new (placement) TType(static_cast<const TType&>(msg));
                return true;
            }
            else
                return false;
        }

        template <typename TType>
        bool addNewMessageType(npos::ipc::Message&& msg)
        {
            if (TType::ID == msg.getId())
            {
                void* placement = data;
                new (placement) TType(static_cast<TType&&>(msg));
                return true;
            }
            else
                return false;
        }


    private:
        typename etl::aligned_storage<SIZE, ALIGNMENT>::type data;
        bool valid;
    };
}
