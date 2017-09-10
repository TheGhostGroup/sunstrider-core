
#include "Common.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"

#include "AccountMgr.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "GameTime.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "LogsDatabaseAccessor.h"
#include "Mail.h"
#include "Bag.h"
#include "CharacterCache.h"

AuctionHouseMgr::AuctionHouseMgr()
{
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for(auto & mAitem : mAitems)
        delete mAitem.second;
}

AuctionHouseObject * AuctionHouseMgr::GetAuctionsMap( uint32 factionTemplateId )
{
    if(sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mNeutralAuctions;

    // team have linked auction houses
    FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if(!u_entry)
        return &mNeutralAuctions;
    else if(u_entry->ourMask & FACTION_MASK_ALLIANCE)
        return &mAllianceAuctions;
    else if(u_entry->ourMask & FACTION_MASK_HORDE)
        return &mHordeAuctions;
    else
        return &mNeutralAuctions;
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, Item *pItem)
{
    uint32 MSV = pItem->GetTemplate()->SellPrice;
    double deposit;
    double faction_pct;
    if (MSV > 0)
    {
        if(sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
            faction_pct = (0.15 * (double)sWorld->GetRate(RATE_AUCTION_DEPOSIT));
        else
        {
            FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(entry->houseId);
            if(!u_entry)
                faction_pct = (0.75 * (double)sWorld->GetRate(RATE_AUCTION_DEPOSIT));
            else if(u_entry->ourMask & FACTION_MASK_ALLIANCE)
                faction_pct = (0.15 * (double)sWorld->GetRate(RATE_AUCTION_DEPOSIT));
            else if(u_entry->ourMask & FACTION_MASK_HORDE)
                faction_pct = (0.15 * (double)sWorld->GetRate(RATE_AUCTION_DEPOSIT));
            else
                faction_pct = (0.75 * (double)sWorld->GetRate(RATE_AUCTION_DEPOSIT));
        }
        deposit = ((double)MSV * faction_pct * (double)pItem->GetCount()) * (double)(time / MIN_AUCTION_TIME );
    }
    else
    {
        //faction_pct = 0.0f;
        deposit = 0.0f;
    }

    if (deposit > 0)
        return (uint32)deposit;
    else
        return 0;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(SQLTransaction& trans, AuctionEntry *auction )
{
    Item *pItem = GetAItem(auction->item_guidlow);
    if(!pItem)
        return;

    Player* bidder = sObjectMgr->GetPlayer(auction->bidder);

    uint32 bidder_accId = 0;
    if (!bidder)
        bidder_accId = sCharacterCache->GetCharacterAccountIdByGuid(auction->bidder);
    else
        bidder_accId = bidder->GetSession()->GetAccountId();

    // data for logging
    {
        uint32 owner_accid = sCharacterCache->GetCharacterAccountIdByGuid(auction->owner);

        LogsDatabaseAccessor::WonAuction(bidder_accId, auction->bidder, owner_accid, auction->owner, auction->item_guidlow, auction->item_template, pItem->GetCount());
    }

    
    // receiver exist
    if(bidder || bidder_accId)
    {
        std::ostringstream msgAuctionWonSubject;
        msgAuctionWonSubject << auction->item_template << ":0:" << AUCTION_WON;

        std::ostringstream msgAuctionWonBody;
        msgAuctionWonBody.width(16);
        msgAuctionWonBody << std::right << std::hex << auction->owner;
        msgAuctionWonBody << std::dec << ":" << auction->bid << ":" << auction->buyout;

        //prepare mail data... :
        uint32 itemTextId = sObjectMgr->CreateItemText( trans, msgAuctionWonBody.str() );

        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        trans->PAppend("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'",auction->bidder,pItem->GetGUIDLow());

        MailItemsInfo mi;
        mi.AddItem(auction->item_guidlow, auction->item_template, pItem);

        if (bidder)
            bidder->GetSession()->SendAuctionBidderNotification( auction->GetHouseId(), auction->Id, bidder->GetGUID(), 0, 0, auction->item_template);
        else
            RemoveAItem(pItem->GetGUIDLow()); // we have to remove the item, before we delete it !!

        // will delete item or place to receiver mail list
        WorldSession::SendMailTo(bidder, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->GetHouseId(), auction->bidder, msgAuctionWonSubject.str(), itemTextId, &mi, 0, 0, MAIL_CHECK_MASK_AUCTION);
    }
    // receiver not exist
    else
    {
        trans->PAppend("DELETE FROM item_instance WHERE guid='%u'", pItem->GetGUIDLow());
        RemoveAItem(pItem->GetGUIDLow()); // we have to remove the item, before we delete it !!
        delete pItem;
    }
}

void AuctionHouseMgr::SendAuctionSalePendingMail(SQLTransaction& trans, AuctionEntry * auction )
{
	ObjectGuid owner_guid(HighGuid::Player, auction->owner);
    Player *owner = ObjectAccessor::FindConnectedPlayer(owner_guid);
	uint32 owner_accId = sCharacterCache->GetCharacterAccountIdByGuid(owner_guid);

    // owner exist (online or offline)
    if(owner || owner_accId)
    {
        std::ostringstream msgAuctionSalePendingSubject;
        msgAuctionSalePendingSubject << auction->item_template << ":0:" << AUCTION_SALE_PENDING;

        std::ostringstream msgAuctionSalePendingBody;
        uint32 auctionCut = auction->GetAuctionCut();

        time_t distrTime = time(nullptr) + sWorld->getConfig(CONFIG_MAIL_DELIVERY_DELAY);

        msgAuctionSalePendingBody.width(16);
        msgAuctionSalePendingBody << std::right << std::hex << auction->bidder;
        msgAuctionSalePendingBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        msgAuctionSalePendingBody << ":" << auction->deposit << ":" << auctionCut << ":0:";
        msgAuctionSalePendingBody << secsToTimeBitFields(distrTime);

        uint32 itemTextId = sObjectMgr->CreateItemText( trans, msgAuctionSalePendingBody.str() );

        WorldSession::SendMailTo(trans, owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->GetHouseId(), auction->owner, msgAuctionSalePendingSubject.str(), itemTextId, nullptr, 0, 0, MAIL_CHECK_MASK_AUCTION);
    }
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(SQLTransaction& trans, AuctionEntry * auction )
{
	ObjectGuid owner_guid(HighGuid::Player, auction->owner);
	Player* owner = ObjectAccessor::FindConnectedPlayer(owner_guid);
	uint32 owner_accId = sCharacterCache->GetCharacterAccountIdByGuid(owner_guid);

    // owner exist
    if(owner || owner_accId)
    {
        std::ostringstream msgAuctionSuccessfulSubject;
        msgAuctionSuccessfulSubject << auction->item_template << ":0:" << AUCTION_SUCCESSFUL;

        std::ostringstream auctionSuccessfulBody;
        uint32 auctionCut = auction->GetAuctionCut();

        auctionSuccessfulBody.width(16);
        auctionSuccessfulBody << std::right << std::hex << auction->bidder;
        auctionSuccessfulBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        auctionSuccessfulBody << ":" << auction->deposit << ":" << auctionCut;

        uint32 itemTextId = sObjectMgr->CreateItemText( trans, auctionSuccessfulBody.str() );

        uint32 profit = auction->bid + auction->deposit - auctionCut;

        if (owner)
        {
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification( auction );
        }

        WorldSession::SendMailTo(trans, owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->GetHouseId(), auction->owner, msgAuctionSuccessfulSubject.str(), itemTextId, nullptr, profit, 0, MAIL_CHECK_MASK_AUCTION, sWorld->getConfig(CONFIG_MAIL_DELIVERY_DELAY));
    }
    else
        TC_LOG_ERROR("auctionHouse","SendAuctionSuccessfulMail: Mail not sent for some reason to player %s (GUID %u, account %u).", owner ? owner->GetName().c_str() : "<unknown> (maybe offline)", owner ? owner->GetGUIDLow() : 0, owner_accId);
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(SQLTransaction& trans, AuctionEntry * auction )
{ //return an item in auction to its owner by mail
    Item *pItem = GetAItem(auction->item_guidlow);
    if(!pItem)
    {
        TC_LOG_ERROR("auctionHouse","Auction item (GUID: %u) not found, and lost.",auction->item_guidlow);
        return;
    }

	ObjectGuid owner_guid(HighGuid::Player, auction->owner);
	Player* owner = ObjectAccessor::FindConnectedPlayer(owner_guid);
	uint32 owner_accId = sCharacterCache->GetCharacterAccountIdByGuid(owner_guid);

    // owner exist
    if(owner || owner_accId)
    {
        std::ostringstream subject;
        subject << auction->item_template << ":0:" << AUCTION_EXPIRED;

        if ( owner )
            owner->GetSession()->SendAuctionOwnerNotification( auction );
        else
            RemoveAItem(pItem->GetGUIDLow()); // we have to remove the item, before we delete it !!

        MailItemsInfo mi;
        mi.AddItem(auction->item_guidlow, auction->item_template, pItem);

        // will delete item or place to receiver mail list
        WorldSession::SendMailTo(trans, owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->GetHouseId(), owner->GetGUIDLow(), subject.str(), 0, &mi, 0, 0, MAIL_CHECK_MASK_NONE);
    }
    // owner not found
    else
    {
        trans->PAppend("DELETE FROM item_instance WHERE guid='%u'",pItem->GetGUIDLow());
        RemoveAItem(pItem->GetGUIDLow()); // we have to remove the item, before we delete it !!
        delete pItem;
    }
}

void AuctionHouseMgr::LoadAuctionItems()
{
    // data needs to be at first place for Item::LoadFromDB
    QueryResult result = CharacterDatabase.Query( "SELECT itemguid, item_template FROM auctionhouse" );

    if( !result )
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 auction items");
        return;
    }

    uint32 count = 0;

    Field *fields;
    do
    {
        fields = result->Fetch();
        uint32 item_guid = fields[0].GetUInt32();
        uint32 item_template = fields[1].GetUInt32();

        ItemTemplate const *proto = sObjectMgr->GetItemTemplate(item_template);

        if(!proto)
        {
            TC_LOG_ERROR("sql.sql", "ObjectMgr::LoadAuctionItems: Unknown item template (GUID: %u, id: #%u) in auction, skipped.", item_guid,item_template);
            continue;
        }

        Item *item = NewItemOrBag(proto);

        if(!item->LoadFromDB(item_guid,0))
        {
            TC_LOG_ERROR("sql.sql", "ObjectMgr::LoadAuctionItems: Unknown item (GUID: %u, id: %u) in auction, skipped.", item_guid, item_template);
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u auction items", count );
}

void AuctionHouseMgr::LoadAuctions()
{
    QueryResult result = CharacterDatabase.Query("SELECT COUNT(*) FROM auctionhouse");
    if( !result )
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    Field *fields = result->Fetch();
    uint32 AuctionCount=fields[0].GetUInt64();

    if(!AuctionCount)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    result = CharacterDatabase.Query( "SELECT id,auctioneerguid,itemguid,item_template,itemowner,buyoutprice,time,buyguid,lastbid,startbid,deposit FROM auctionhouse" );
    if( !result )
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    AuctionEntry *aItem;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    do
    {
        fields = result->Fetch();

        aItem = new AuctionEntry;
        aItem->Id = fields[0].GetUInt32();
        aItem->auctioneer = fields[1].GetUInt32();
        aItem->item_guidlow = fields[2].GetUInt32();
        aItem->item_template = fields[3].GetUInt32();
        aItem->owner = fields[4].GetUInt32();
        aItem->buyout = fields[5].GetUInt32();
        aItem->expire_time = fields[6].GetUInt32();
        aItem->bidder = fields[7].GetUInt32();
        aItem->bid = fields[8].GetUInt32();
        aItem->startbid = fields[9].GetUInt32();
        aItem->deposit = fields[10].GetUInt32();
        aItem->deposit_time = 0;    // No need to save this after a restart, as this is used only for a short amount of time

        CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(aItem->auctioneer);
        if(!auctioneerData)
        {
            AuctionHouseMgr::SendAuctionExpiredMail(trans,aItem);
            aItem->DeleteFromDB(trans);
            TC_LOG_ERROR("misc","Auction %u has not a existing auctioneer (GUID : %u)", aItem->Id, aItem->auctioneer);
            delete aItem;
            continue;
        }

        CreatureTemplate const* auctioneerInfo = sObjectMgr->GetCreatureTemplate(auctioneerData->id);
        if(!auctioneerInfo)
        {
            AuctionHouseMgr::SendAuctionExpiredMail(trans,aItem);
            aItem->DeleteFromDB(trans);
            TC_LOG_ERROR("misc","Auction %u has not a existing auctioneer (GUID : %u Entry: %u)", aItem->Id, aItem->auctioneer,auctioneerData->id);
            delete aItem;
            continue;
        }

        aItem->auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(auctioneerInfo->faction);
        if(!aItem->auctionHouseEntry)
        {
            AuctionHouseMgr::SendAuctionExpiredMail(trans,aItem);
            aItem->DeleteFromDB(trans);
            TC_LOG_ERROR("misc","Auction %u has auctioneer (GUID : %u Entry: %u) with wrong faction %u",
                aItem->Id, aItem->auctioneer,auctioneerData->id,auctioneerInfo->faction);
            delete aItem;
            continue;
        }

        // check if sold item exists for guid
        // and item_template in fact (GetAItem will fail if problematic in result check in ObjectMgr::LoadAuctionItems)
        if ( !GetAItem( aItem->item_guidlow ) )
        {
            aItem->DeleteFromDB(trans);
            TC_LOG_ERROR("misc","Auction %u has not a existing item : %u", aItem->Id, aItem->item_guidlow);
            delete aItem;
            continue;
        }

        GetAuctionsMap( auctioneerInfo->faction )->AddAuction(aItem);

    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_INFO( "server.loading",">> Loaded %u auctions", AuctionCount );
}

void AuctionHouseMgr::AddAItem( Item* it )
{
    ASSERT( it );
    ASSERT( mAitems.find(it->GetGUIDLow()) == mAitems.end());
    mAitems[it->GetGUIDLow()] = it;
}

bool AuctionHouseMgr::RemoveAItem( uint32 id )
{
    auto i = mAitems.find(id);
    if (i == mAitems.end())
    {
        return false;
    }
    mAitems.erase(i);
    return true;
}

void AuctionHouseMgr::Update()
{
    mHordeAuctions.Update();
    mAllianceAuctions.Update();
    mNeutralAuctions.Update();
}

void AuctionHouseMgr::RemoveAllAuctionsOf(SQLTransaction& trans, uint32 ownerGUID)
{
    mHordeAuctions.RemoveAllAuctionsOf(trans, ownerGUID);
    mAllianceAuctions.RemoveAllAuctionsOf(trans, ownerGUID);
    mNeutralAuctions.RemoveAllAuctionsOf(trans, ownerGUID);
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntry(uint32 factionTemplateId)
{
    uint32 houseid = 7; // goblin auction house

    if(!sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        //FIXME: found way for proper auctionhouse selection by another way
        // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
        // but no easy way convert creature faction to player race faction for specific city
        switch(factionTemplateId)
        {
            case 12: houseid = 1; break; // human
            case 29: houseid = 6; break; // orc, and generic for horde
            case 55: houseid = 2; break; // dwarf, and generic for alliance
            case 68: houseid = 4; break; // undead
            case 80: houseid = 3; break; // n-elf
            case 104: houseid = 5; break; // trolls
            case 120: houseid = 7; break; // booty bay, neutral
            case 474: houseid = 7; break; // gadgetzan, neutral
            case 855: houseid = 7; break; // everlook, neutral
            case 1604: houseid = 6; break; // b-elfs,
            default: // for unknown case
            {
                FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if(!u_entry)
                    houseid = 7; // goblin auction house
                else if(u_entry->ourMask & FACTION_MASK_ALLIANCE)
                    houseid = 1; // human auction house
                else if(u_entry->ourMask & FACTION_MASK_HORDE)
                    houseid = 6; // orc auction house
                else
                    houseid = 7; // goblin auction house
                break;
            }
        }
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

void AuctionHouseObject::Update()
{
    time_t curTime = GameTime::GetGameTime();
    ///- Handle expired auctions
    AuctionEntryMap::iterator next;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    for (auto itr = AuctionsMap.begin(); itr != AuctionsMap.end();itr = next)
    {
        next = itr;
        ++next;
        if (curTime > (itr->second->expire_time))
        {
            ///- Either cancel the auction if there was no bidder
            if (itr->second->bidder == 0)
            {
                sAuctionMgr->SendAuctionExpiredMail(trans, itr->second );
            }
            ///- Or perform the transaction
            else
            {
                //we should send an "item sold" message if the seller is online
                //we send the item to the winner
                //we send the money to the seller
                sAuctionMgr->SendAuctionSuccessfulMail(trans, itr->second );
                sAuctionMgr->SendAuctionWonMail(trans, itr->second );
            }

            ///- In any case clear the auction
            itr->second->DeleteFromDB(trans);
            sAuctionMgr->RemoveAItem(itr->second->item_guidlow);
            delete itr->second;
            RemoveAuction(itr->first);
        }
    }
    CharacterDatabase.CommitTransaction(trans);
}

// NOT threadsafe!
void AuctionHouseObject::RemoveAllAuctionsOf(SQLTransaction& trans, uint32 ownerGUID)
{
    AuctionEntryMap::iterator next;
    for (auto itr = AuctionsMap.begin(); itr != AuctionsMap.end();itr = next)
    {
        next = itr;
        ++next;
        if (itr->second->owner == ownerGUID)
        {
            ///- Either cancel the auction if there was no bidder
            if (itr->second->bidder == 0)
            {
                sAuctionMgr->SendAuctionExpiredMail(trans, itr->second );
            }
            ///- Or perform the transaction
            else
            {
                //we should send an "item sold" message if the seller is online
                //we send the item to the winner
                //we send the money to the seller
                sAuctionMgr->SendAuctionSuccessfulMail(trans, itr->second );
                sAuctionMgr->SendAuctionWonMail(trans, itr->second );
            }

            ///- In any case clear the auction
            itr->second->DeleteFromDB(trans);
            sAuctionMgr->RemoveAItem(itr->second->item_guidlow);
            delete itr->second;
            RemoveAuction(itr->first);
        }
    }
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if( Aentry && Aentry->bidder == player->GetGUIDLow() )
        {
            if (itr->second->BuildAuctionInfo(data))
                ++count;
            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if( Aentry && Aentry->owner == player->GetGUIDLow() )
        {
            if(Aentry->BuildAuctionInfo(data))
                ++count;
            ++totalcount;

            if(totalcount >= MAX_AUCTIONS) //avoid client crash
                break;
        }
    }
}

uint32 AuctionHouseObject::GetAuctionsCount(Player* player)
{
    uint32 totalcount = 0;
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if( Aentry && Aentry->owner == player->GetGUIDLow() )
            ++totalcount;
    }
    return totalcount;
}

void AuctionHouseObject::BuildListAuctionItems(WorldPacket& data, Player* player,
    std::wstring const& wsearchedname, uint32 listfrom, uint32 levelmin, uint32 levelmax, uint32 usable,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
    uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        Item *item = sAuctionMgr->GetAItem(Aentry->item_guidlow);
        if (!item)
            continue;

        ItemTemplate const *proto = item->GetTemplate();

        if (itemClass != (0xffffffff) && proto->Class != itemClass)
            continue;

        if (itemSubClass != (0xffffffff) && proto->SubClass != itemSubClass)
            continue;

        if (inventoryType != (0xffffffff) && proto->InventoryType != inventoryType)
            continue;

        if (quality != (0xffffffff) && proto->Quality != quality)
            continue;

        if(    ( levelmin && (proto->RequiredLevel < levelmin) )
            || ( levelmax && (proto->RequiredLevel > levelmax) ) 
          )
            continue;

        if( usable != (0x00) && player->CanUseItem( item ) != EQUIP_ERR_OK )
            continue;

        std::string name = player->GetSession()->GetLocalizedItemName(proto);
        if(name.empty())
            continue;

        if( !wsearchedname.empty() && !Utf8FitTo(name, wsearchedname) )
            continue;

        if ((count < 50) && (totalcount >= listfrom))
        {
            ++count;
            Aentry->BuildAuctionInfo(data);
        }

        ++totalcount;
    }
}

//this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket & data) const
{
    Item *pItem = sAuctionMgr->GetAItem(item_guidlow);
    if (!pItem)
    {
        TC_LOG_ERROR("auctionHouse", "Auction to item %u, that doesn't exist !!!!", item_guidlow);
        return false;
    }
    data << (uint32) Id;
    data << (uint32) pItem->GetEntry();

    for (uint8 i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; i++)
    {
        data << (uint32) pItem->GetEnchantmentId(EnchantmentSlot(i));
        data << (uint32) pItem->GetEnchantmentDuration(EnchantmentSlot(i));
        data << (uint32) pItem->GetEnchantmentCharges(EnchantmentSlot(i));
    }

    data << (uint32) pItem->GetItemRandomPropertyId();      //random item property id
    data << (uint32) pItem->GetItemSuffixFactor();          //SuffixFactor
    data << (uint32) pItem->GetCount();                     //item->count
    data << (uint32) pItem->GetSpellCharges();              //item->charge FFFFFFF
    data << (uint32) 0;                                     //Unknown
    data << (uint64) owner;                                 //Auction->owner
    data << (uint32) startbid;                              //Auction->startbid (not sure if useful)
    data << (uint32) (bid ? GetAuctionOutBid() : 0);
    //minimal outbid
    data << (uint32) buyout;                                //auction->buyout
    data << (uint32) (expire_time - time(nullptr))* 1000;      //time left
    data << (uint64) bidder;                                //auction->bidder current
    data << (uint32) bid;                                   //current bid
    return true;
}

uint32 AuctionEntry::GetAuctionCut() const
{
    uint32 cutPercent;
    if (sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        cutPercent = 5; //default value for horde/alliance auction house
    else
        cutPercent = auctionHouseEntry->cutPercent;

    return uint32(cutPercent * bid * sWorld->GetRate(RATE_AUCTION_CUT) / 100.f);
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint32 AuctionEntry::GetAuctionOutBid() const
{
    uint32 outbid = (bid / 100) * 5;
    if (!outbid)
        outbid = 1;
    return outbid;
}

void AuctionEntry::DeleteFromDB(SQLTransaction& trans) const
{
    //No SQL injection (Id is integer)
    trans->PAppend("DELETE FROM auctionhouse WHERE id = '%u'",Id);
}

void AuctionEntry::SaveToDB(SQLTransaction& trans) const
{
    //No SQL injection (no strings)
    trans->PAppend("INSERT INTO auctionhouse (id,auctioneerguid,itemguid,item_template,itemowner,buyoutprice,time,buyguid,lastbid,startbid,deposit) "
        "VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '" UI64FMTD "', '%u', '%u', '%u', '%u')",
        Id, auctioneer, item_guidlow, item_template, owner, buyout, (uint64)expire_time, bidder, bid, startbid, deposit);
}