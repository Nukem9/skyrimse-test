#pragma once

#pragma pack(push, 8)
template<typename KeyType, typename ValueType>
struct BSTScatterTable
{
    struct LinkEntry
    {
        KeyType Key;
        ValueType Value;
        LinkEntry *Next; // Can be nullptr
    };

    char _gap0[4];      //
    uint32_t Size;      // Total number of entries (including lists)
    char _gap8[4];      //
    uint32_t HashBase;  // Entries[CRC32(key) & (HashBase - 1)]
    char _gap10[8];     //
    LinkEntry *ListEnd; // Final LinkEntry::Next value points to this
    BYTE gap20[8];      //
    LinkEntry *Buckets; // Resizable array of linked lists

    bool Get(KeyType Key, ValueType &Out)
    {
        if (this->Buckets)
        {
            int keyHash;
            CRC32_Lazy(&keyHash, Key);

            auto entry = &this->Buckets[keyHash & (this->HashBase - 1)];

            for (bool i = entry->Next == nullptr; !i; i = entry == this->ListEnd)
            {
                if (entry->Key == Key)
                {
                    Out = entry->Value;
                    return true;
                }

                entry = entry->Next;
            }
        }

        return false;
    }
};
#pragma pack(pop)

using __BSTSC = BSTScatterTable<uint32_t, void *>;

static_assert(offsetof(__BSTSC, Size) == 0x4, "");
static_assert(offsetof(__BSTSC, HashBase) == 0xC, "");
static_assert(offsetof(__BSTSC, ListEnd) == 0x18, "");
static_assert(offsetof(__BSTSC, Buckets) == 0x28, "");

static_assert(sizeof(__BSTSC::LinkEntry) == 0x18, "");
static_assert(offsetof(__BSTSC::LinkEntry, Key) == 0x0, "");
static_assert(offsetof(__BSTSC::LinkEntry, Value) == 0x8, "");
static_assert(offsetof(__BSTSC::LinkEntry, Next) == 0x10, "");
