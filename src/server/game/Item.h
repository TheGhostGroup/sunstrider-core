
#ifndef TRINITYCORE_ITEM_H
#define TRINITYCORE_ITEM_H

#include "Object.h"
#include "LootMgr.h"
#include "ItemPrototype.h"
#include "Transaction.h"

class SpellInfo;
class Bag;

struct ItemSetEffect
{
    uint32 setid;
    uint32 item_count;
    SpellInfo const *spells[8];
};

enum InventoryResult
{
    EQUIP_ERR_OK                                 = 0,
    EQUIP_ERR_CANT_EQUIP_LEVEL_I                 = 1,
    EQUIP_ERR_ERR_CANT_EQUIP_SKILL               = 2,
    EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT             = 3,
    EQUIP_ERR_BAG_FULL                           = 4,
    EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG        = 5,
    EQUIP_ERR_CANT_TRADE_EQUIP_BAGS              = 6,
    EQUIP_ERR_ONLY_AMMO_CAN_GO_HERE              = 7,
    EQUIP_ERR_NO_REQUIRED_PROFICIENCY            = 8,
    EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE        = 9,
    EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM        = 10,
    EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM2       = 11,
    EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE2       = 12,
    EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED          = 13,
    EQUIP_ERR_CANT_DUAL_WIELD                    = 14,
    EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG            = 15,
    EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG2           = 16,
    EQUIP_ERR_CANT_CARRY_MORE_OF_THIS            = 17,
    EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE3       = 18,
    EQUIP_ERR_ITEM_CANT_STACK                    = 19,
    EQUIP_ERR_ITEM_CANT_BE_EQUIPPED              = 20,
    EQUIP_ERR_ITEMS_CANT_BE_SWAPPED              = 21,
    EQUIP_ERR_SLOT_IS_EMPTY                      = 22,
    EQUIP_ERR_ITEM_NOT_FOUND                     = 23,
    EQUIP_ERR_CANT_DROP_SOULBOUND                = 24,
    EQUIP_ERR_OUT_OF_RANGE                       = 25,
    EQUIP_ERR_TRIED_TO_SPLIT_MORE_THAN_COUNT     = 26,
    EQUIP_ERR_COULDNT_SPLIT_ITEMS                = 27,
    EQUIP_ERR_MISSING_REAGENT                    = 28,
    EQUIP_ERR_NOT_ENOUGH_MONEY                   = 29,
    EQUIP_ERR_NOT_A_BAG                          = 30,
    EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS        = 31,
    EQUIP_ERR_DONT_OWN_THAT_ITEM                 = 32,
    EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER             = 33,
    EQUIP_ERR_MUST_PURCHASE_THAT_BAG_SLOT        = 34,
    EQUIP_ERR_TOO_FAR_AWAY_FROM_BANK             = 35,
    EQUIP_ERR_ITEM_LOCKED                        = 36,
    EQUIP_ERR_YOU_ARE_STUNNED                    = 37,
    EQUIP_ERR_YOU_ARE_DEAD                       = 38,
    EQUIP_ERR_CANT_DO_RIGHT_NOW                  = 39,
    EQUIP_ERR_INT_BAG_ERROR                      = 40,
    EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER2            = 41,
    EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH          = 42,
    EQUIP_ERR_STACKABLE_CANT_BE_WRAPPED          = 43,
    EQUIP_ERR_EQUIPPED_CANT_BE_WRAPPED           = 44,
    EQUIP_ERR_WRAPPED_CANT_BE_WRAPPED            = 45,
    EQUIP_ERR_BOUND_CANT_BE_WRAPPED              = 46,
    EQUIP_ERR_UNIQUE_CANT_BE_WRAPPED             = 47,
    EQUIP_ERR_BAGS_CANT_BE_WRAPPED               = 48,
    EQUIP_ERR_ALREADY_LOOTED                     = 49,
    EQUIP_ERR_INVENTORY_FULL                     = 50,
    EQUIP_ERR_BANK_FULL                          = 51,
    EQUIP_ERR_ITEM_IS_CURRENTLY_SOLD_OUT         = 52,
    EQUIP_ERR_BAG_FULL3                          = 53,
    EQUIP_ERR_ITEM_NOT_FOUND2                    = 54,
    EQUIP_ERR_ITEM_CANT_STACK2                   = 55,
    EQUIP_ERR_BAG_FULL4                          = 56,
    EQUIP_ERR_ITEM_SOLD_OUT                      = 57,
    EQUIP_ERR_OBJECT_IS_BUSY                     = 58,
    EQUIP_ERR_NONE                               = 59,
    EQUIP_ERR_NOT_IN_COMBAT                      = 60,
    EQUIP_ERR_NOT_WHILE_DISARMED                 = 61,
    EQUIP_ERR_BAG_FULL6                          = 62,
    EQUIP_ERR_CANT_EQUIP_RANK                    = 63,
    EQUIP_ERR_CANT_EQUIP_REPUTATION              = 64,
    EQUIP_ERR_TOO_MANY_SPECIAL_BAGS              = 65,
    EQUIP_ERR_LOOT_CANT_LOOT_THAT_NOW            = 66,
    EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE              = 67,
    EQUIP_ERR_VENDOR_MISSING_TURNINS             = 68,
    EQUIP_ERR_NOT_ENOUGH_HONOR_POINTS            = 69,
    EQUIP_ERR_NOT_ENOUGH_ARENA_POINTS            = 70,
    EQUIP_ERR_ITEM_MAX_COUNT_SOCKETED            = 71,
    EQUIP_ERR_MAIL_BOUND_ITEM                    = 72,
    EQUIP_ERR_NO_SPLIT_WHILE_PROSPECTING         = 73,
    EQUIP_ERR_ITEM_MAX_COUNT_EQUIPPED_SOCKETED   = 75,
    EQUIP_ERR_ITEM_UNIQUE_EQUIPPABLE_SOCKETED    = 76,
    EQUIP_ERR_TOO_MUCH_GOLD                      = 77,
    EQUIP_ERR_NOT_DURING_ARENA_MATCH             = 78,
    EQUIP_ERR_CANNOT_TRADE_THAT                  = 79,
    EQUIP_ERR_PERSONAL_ARENA_RATING_TOO_LOW      = 80
    // probably exist more
};

enum BuyFailure
{
    BUY_ERR_CANT_FIND_ITEM                      = 0,
    BUY_ERR_ITEM_ALREADY_SOLD                   = 1,
    BUY_ERR_NOT_ENOUGHT_MONEY                   = 2,
    BUY_ERR_SELLER_DONT_LIKE_YOU                = 4,
    BUY_ERR_DISTANCE_TOO_FAR                    = 5,
    BUY_ERR_ITEM_SOLD_OUT                       = 7,
    BUY_ERR_CANT_CARRY_MORE                     = 8,
    BUY_ERR_RANK_REQUIRE                        = 11,
    BUY_ERR_REPUTATION_REQUIRE                  = 12
};

enum SellFailure
{
    SELL_ERR_CANT_FIND_ITEM                      = 1,
    SELL_ERR_CANT_SELL_ITEM                      = 2,       // merchant doesn't like that item
    SELL_ERR_CANT_FIND_VENDOR                    = 3,       // merchant doesn't like you
    SELL_ERR_YOU_DONT_OWN_THAT_ITEM              = 4,       // you don't own that item
    SELL_ERR_UNK                                 = 5,       // nothing appears...
    SELL_ERR_ONLY_EMPTY_BAG                      = 6        // can only do with empty bags
};

// -1 from client enchantment slot number
enum EnchantmentSlot
{
    PERM_ENCHANTMENT_SLOT       = 0,
    TEMP_ENCHANTMENT_SLOT       = 1,
    SOCK_ENCHANTMENT_SLOT       = 2,
    SOCK_ENCHANTMENT_SLOT_2     = 3,
    SOCK_ENCHANTMENT_SLOT_3     = 4,
    BONUS_ENCHANTMENT_SLOT      = 5,
    MAX_INSPECTED_ENCHANTMENT_SLOT = 6,

    PROP_ENCHANTMENT_SLOT_0     = 6,                        // used with RandomSuffix
    PROP_ENCHANTMENT_SLOT_1     = 7,                        // used with RandomSuffix
    PROP_ENCHANTMENT_SLOT_2     = 8,                        // used with RandomSuffix and RandomProperty
    PROP_ENCHANTMENT_SLOT_3     = 9,                        // used with RandomProperty
    PROP_ENCHANTMENT_SLOT_4     = 10,                       // used with RandomProperty
    MAX_ENCHANTMENT_SLOT        = 11
};

#define MAX_VISIBLE_ITEM_OFFSET   16                        // 16 fields per visible item (creator(2) + enchantments(12) + properties(1) + pad(1))

#define MAX_GEM_SOCKETS               MAX_ITEM_PROTO_SOCKETS// (BONUS_ENCHANTMENT_SLOT-SOCK_ENCHANTMENT_SLOT) and item proto size, equal value expected

enum EnchantmentOffset
{
    ENCHANTMENT_ID_OFFSET       = 0,
    ENCHANTMENT_DURATION_OFFSET = 1,
    ENCHANTMENT_CHARGES_OFFSET  = 2
};

#define MAX_ENCHANTMENT_OFFSET    3

enum EnchantmentSlotMask
{
    ENCHANTMENT_CAN_SOULBOUND  = 0x01,
    ENCHANTMENT_UNK1           = 0x02,
    ENCHANTMENT_UNK2           = 0x04,
    ENCHANTMENT_UNK3           = 0x08
};

enum ItemUpdateState
{
    ITEM_UNCHANGED                               = 0,
    ITEM_CHANGED                                 = 1,
    ITEM_NEW                                     = 2,
    ITEM_REMOVED                                 = 3
};

#define MAX_ITEM_SPELLS 5

bool ItemCanGoIntoBag(ItemTemplate const *proto, ItemTemplate const *pBagProto);

class TC_GAME_API Item : public Object
{
    public:
        static Item* CreateItem( uint32 item, uint32 count, Player const* player = nullptr, ItemTemplate const *proto = nullptr );
        Item* CloneItem( uint32 count, Player const* player = nullptr ) const;

        Item ( );

        virtual bool Create( uint32 guidlow, uint32 itemid, Player const* owner, ItemTemplate const *proto);

        ItemTemplate const* GetTemplate() const { return m_itemProto; }

        uint64 GetOwnerGUID() const;
        void SetOwnerGUID(ObjectGuid const& guid);
        Player* GetOwner() const;

        void SetBinding(bool val);
        bool IsSoulBound() const;
        bool IsBindedNotWith(uint64 guid) const { return IsSoulBound() && GetOwnerGUID()!= guid; }
        bool IsBoundByEnchant() const;
        virtual void SaveToDB(SQLTransaction trans);
        virtual bool LoadFromDB(uint32 guid, uint64 owner_guid);
        virtual void DeleteFromDB();
        void DeleteFromInventoryDB(SQLTransaction trans);

        bool IsBag() const;
        bool IsBroken() const;
        bool CanBeTraded() const;
        void SetInTrade(bool b = true) { mb_in_trade = b; }
        bool IsInTrade() const { return mb_in_trade; }

        bool IsFitToSpellRequirements(SpellInfo const* spellInfo) const;
        bool IsLimitedToAnotherMapOrZone( uint32 cur_mapId, uint32 cur_zoneId) const;
        bool GemsFitSockets() const;

        uint32 GetCount() const;
        void SetCount(uint32 value);
        uint32 GetMaxStackCount() const { return GetTemplate()->Stackable; }
        uint8 GetGemCountWithID(uint32 GemID) const;

        uint8 GetSlot() const {return m_slot;}
        Bag *GetContainer() { return m_container; }
        uint8 GetBagSlot() const;
        void SetSlot(uint8 slot) {m_slot = slot;}
        uint16 GetPos() const { return uint16(GetBagSlot()) << 8 | GetSlot(); }
        void SetContainer(Bag *container) { m_container = container; }

        bool IsInBag() const { return m_container != nullptr; }
        bool IsEquipped() const;

        uint32 GetSkill();
        uint32 GetSpell();

        // RandomPropertyId (signed but stored as unsigned)
        int32 GetItemRandomPropertyId() const;
        uint32 GetItemSuffixFactor() const;
        void SetItemRandomProperties(int32 randomPropId);
        bool UpdateItemSuffixFactor();
        static int32 GenerateItemRandomPropertyId(uint32 item_id);
        void SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges);
        void SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration);
        void SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges);
        void ClearEnchantment(EnchantmentSlot slot);
        uint32 GetEnchantmentId(EnchantmentSlot slot)       const;
        uint32 GetEnchantmentDuration(EnchantmentSlot slot) const;
        uint32 GetEnchantmentCharges(EnchantmentSlot slot)  const;

        void SendTimeUpdate(Player* owner);
        void UpdateDuration(Player* owner, uint32 diff);

        // spell charges (signed but stored as unsigned)
        int32 GetSpellCharges(uint8 index/*0..5*/ = 0) const;
        void  SetSpellCharges(uint8 index/*0..5*/, int32 value);

        Loot loot;
        bool m_lootGenerated;

        // Update States
        ItemUpdateState GetState() const { return uState; }
        void SetState(ItemUpdateState state, Player *forplayer = nullptr);
        void AddToUpdateQueueOf(Player *player);
        void RemoveFromUpdateQueueOf(Player *player);
        bool IsInUpdateQueue() const { return uQueuePos != -1; }
        uint16 GetQueuePos() const { return uQueuePos; }
        void FSetState(ItemUpdateState state)               // forced
        {
            uState = state;
        }

        bool HasQuest(uint32 quest_id) const override
        {
            ItemTemplate const *itemProto = GetTemplate();
            return itemProto && itemProto->StartQuest == quest_id;
        }
        bool HasInvolvedQuest(uint32 /*quest_id*/) const override { return false; }

        void BuildUpdate(UpdateDataMapType&, UpdatePlayerSet&) override;

		void AddToObjectUpdate() override;
		void RemoveFromObjectUpdate() override;

        uint32 GetScriptId() const { return GetTemplate()->ScriptId; }
    protected:    
        ItemTemplate const* m_itemProto;
    private:
        uint8 m_slot;
        Bag *m_container;
        ItemUpdateState uState;
        int16 uQueuePos;
        bool mb_in_trade;                                   // true if item is currently in trade-window
};
#endif
