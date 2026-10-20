#pragma once

/**
 * @file MessageStore.hpp
 * @author Adrian Szczepanski
 * @date 20-10-2026
 */

#include <etl/type_traits.h>

#include <libnpos/ipc/Service.hpp>


namespace npos::ipc
{
    template<typename T>
    class MessageStore
    {
    public:
        static_assert(etl::is_base_of<Message, T>::value, "T must be derived from ipc::Message");

        MessageStore(Service& owner)
            : owner(owner)
            , pointer(nullptr)
        {
        }

        inline T* store(T& message)
        {
            if(hasValue())
                return nullptr;

            if(message.sender.portId == owner.getOid().portId)
            {
                local = message;
                pointer = static_cast<T*>(&local);
            }
            else
                pointer = &message;

            pointer->referenceCount++;
            return pointer;
        }

        inline T* release()
        {
            if(not hasValue())
                return nullptr;

            if(pointer->referenceCount > 0)
                pointer->referenceCount--;

            auto* result = pointer;
            pointer = nullptr;
            return result;
        }

        inline bool hasValue() const
        {
            return (pointer != nullptr);
        }

    private:
        Service& owner;
        T* pointer;
        Message local;
    };
}