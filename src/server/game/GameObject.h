
#ifndef TRINITYCORE_GAMEOBJECT_H
#define TRINITYCORE_GAMEOBJECT_H

#include "Common.h"
#include "SharedDefines.h"
#include "Object.h"
#include "LootMgr.h"
#include "Database/DatabaseEnv.h"
#include "GameObjectAI.h"
#include <G3D/Quat.h>

class StaticTransport;
class MotionTransport;

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

// from `gameobject_template`
struct GameObjectTemplate
{
    uint32  entry;
    uint32  type;
    uint32  displayId;
    std::string name;
    std::string IconName;
    std::string castBarCaption;
    uint32  faction;
    uint32  flags;
    float   size;
    union                                                   // different GO types have different data field
    {
        //0 GAMEOBJECT_TYPE_DOOR
        struct
        {
            uint32 startOpen;                               //0 used client side to determine GO_ACTIVATED means open/closed
            uint32 lockId;                                  //1 -> Lock.dbc
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
            uint32 noDamageImmune;                          //3 break opening whenever you recieve damage?
            uint32 openTextID;                              //4 can be used to replace castBarCaption?
            uint32 closeTextID;                             //5
        } door;
        //1 GAMEOBJECT_TYPE_BUTTON
        struct
        {
            uint32 startOpen;                               //0
            uint32 lockId;                                  //1 -> Lock.dbc
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
            uint32 linkedTrap;                              //3
            uint32 noDamageImmune;                          //4 isBattlegroundObject
            uint32 large;                                   //5
            uint32 openTextID;                              //6 can be used to replace castBarCaption?
            uint32 closeTextID;                             //7
            uint32 losOK;                                   //8
        } button;
        //2 GAMEOBJECT_TYPE_QUESTGIVER
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 questList;                               //1
            uint32 pageMaterial;                            //2
            uint32 gossipID;                                //3
            uint32 customAnim;                              //4
            uint32 noDamageImmune;                          //5
            uint32 openTextID;                              //6 can be used to replace castBarCaption?
            uint32 losOK;                                   //7
            uint32 allowMounted;                            //8
            uint32 large;                                   //9
        } questgiver;
        //3 GAMEOBJECT_TYPE_CHEST
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 lootId;                                  //1
            uint32 chestRestockTime;                        //2
            uint32 consumable;                              //3
            uint32 minSuccessOpens;                         //4
            uint32 maxSuccessOpens;                         //5
            uint32 eventId;                                 //6 lootedEvent
            uint32 linkedTrapId;                            //7
            uint32 questId;                                 //8 store quest required for GO activation for player
            uint32 level;                                   //9
            uint32 losOK;                                   //10
            uint32 leaveLoot;                               //11
            uint32 notInCombat;                             //12
            uint32 logLoot;                                 //13
            uint32 openTextID;                              //14 can be used to replace castBarCaption?
            uint32 groupLootRules;                          //15
        } chest;
        //5 GAMEOBJECT_TYPE_GENERIC
        struct
        {
            uint32 floatingTooltip;                         //0
            uint32 highlight;                               //1
            uint32 serverOnly;                              //2
            uint32 large;                                   //3
            uint32 floatOnWater;                            //4
            uint32 questID;                                 //5
        } _generic;
        //6 GAMEOBJECT_TYPE_TRAP
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 level;                                   //1
            uint32 radius;                                  //2 radius for trap activation
            uint32 spellId;                                 //3
            uint32 charges;                                 //4 need respawn (if > 0)
            uint32 cooldown;                                //5 time in secs
            uint32 autoCloseTime;                           //6
            uint32 startDelay;                              //7
            uint32 serverOnly;                              //8
            uint32 stealthed;                               //9
            uint32 large;                                   //10
            uint32 invisible;                               //11
            uint32 openTextID;                              //12 can be used to replace castBarCaption?
            uint32 closeTextID;                             //13
        } trap;
        //7 GAMEOBJECT_TYPE_CHAIR
        struct
        {
            uint32 slots;                                   //0
            uint32 height;                                  //1
            uint32 onlyCreatorUse;                          //2
        } chair;
        //8 GAMEOBJECT_TYPE_SPELL_FOCUS
        struct
        {
            uint32 focusId;                                 //0
            uint32 dist;                                    //1
            uint32 linkedTrapId;                            //2
            uint32 serverOnly;                              //3
            uint32 questID;                                 //4
            uint32 large;                                   //5
        } spellFocus;
        //9 GAMEOBJECT_TYPE_TEXT
        struct
        {
            uint32 pageID;                                  //0
            uint32 language;                                //1
            uint32 pageMaterial;                            //2
            uint32 allowMounted;                            //3
        } text;
        //10 GAMEOBJECT_TYPE_GOOBER
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 questId;                                 //1
            uint32 eventId;                                 //2
            uint32 autoCloseTime;                           //3
            uint32 customAnim;                              //4
            uint32 consumable;                              //5
            uint32 cooldown;                                //6
            uint32 pageId;                                  //7
            uint32 language;                                //8
            uint32 pageMaterial;                            //9
            uint32 spellId;                                 //10
            uint32 noDamageImmune;                          //11
            uint32 linkedTrapId;                            //12
            uint32 large;                                   //13
            uint32 openTextID;                              //14 can be used to replace castBarCaption?
            uint32 closeTextID;                             //15
            uint32 losOK;                                   //16 isBattlegroundObject
            uint32 allowMounted;                            //17
            uint32 floatingTooltip;                         //18 NYI
            uint32 gossipID;                                //19
        } goober;
        //11 GAMEOBJECT_TYPE_TRANSPORT
        struct
        {
            uint32 pauseAtTime;                             //0
            uint32 startOpen;                               //1
            uint32 autoCloseTime;                           //2 secs till autoclose = autoCloseTime / 0x10000
        } transport;
        //12 GAMEOBJECT_TYPE_AREADAMAGE
        struct
        {
            uint32 lockId;                                  //0
            uint32 radius;                                  //1
            uint32 damageMin;                               //2
            uint32 damageMax;                               //3
            uint32 damageSchool;                            //4
            uint32 autoCloseTime;                           //5 secs till autoclose = autoCloseTime / 0x10000
            uint32 openTextID;                              //6
            uint32 closeTextID;                             //7
        } areadamage;
        //13 GAMEOBJECT_TYPE_CAMERA
        struct
        {
            uint32 lockId;                                  //0 -> Lock.dbc
            uint32 cinematicId;                             //1
            uint32 eventID;                                 //2
            uint32 openTextID;                              //3 can be used to replace castBarCaption?
        } camera;
        //15 GAMEOBJECT_TYPE_MO_TRANSPORT
        struct
        {
            uint32 taxiPathId;                              //0
            uint32 moveSpeed;                               //1
            uint32 accelRate;                               //2
            uint32 startEventID;                            //3
            uint32 stopEventID;                             //4
            uint32 transportPhysics;                        //5
            uint32 mapID;                                   //6
            uint32 worldState1;                             //7
            uint32 canBeStopped;                            //8 used on LK only
        } moTransport;
        //17 GAMEOBJECT_TYPE_FISHINGNODE
        struct
        {
            uint32 _data0;                                  //0
            uint32 lootId;                                  //1
        } fishnode;
        //18 GAMEOBJECT_TYPE_SUMMONING_RITUAL
        struct
        {
            uint32 reqParticipants;                         //0
            uint32 spellId;                                 //1
            uint32 animSpell;                               //2
            uint32 ritualPersistent;                        //3
            uint32 casterTargetSpell;                       //4
            uint32 casterTargetSpellTargets;                //5
            uint32 castersGrouped;                          //6
            uint32 ritualNoTargetCheck;                     //7
        } summoningRitual;
        //20 GAMEOBJECT_TYPE_AUCTIONHOUSE
        struct
        {
            uint32 actionHouseID;                           //0
        } auctionhouse;
        //21 GAMEOBJECT_TYPE_GUARDPOST
        struct
        {
            uint32 creatureID;                              //0
            uint32 charges;                                 //1
        } guardpost;
        //22 GAMEOBJECT_TYPE_SPELLCASTER
        struct
        {
            uint32 spellId;                                 //0
            uint32 charges;                                 //1
            uint32 partyOnly;                               //2
            uint32 allowMounted;                            //3 Is usable while on mount/vehicle. (0/1)
            uint32 large;                                   //4 NYI
            uint32 conditionID1;                            //5 NYI
        } spellcaster;
        //23 GAMEOBJECT_TYPE_MEETINGSTONE
        struct
        {
            uint32 minLevel;                                //0
            uint32 MaxLevel;                                //1
            uint32 areaID;                                  //2
        } meetingstone;
        //24 GAMEOBJECT_TYPE_FLAGSTAND
        struct
        {
            uint32 lockId;                                  //0
            uint32 pickupSpell;                             //1
            uint32 radius;                                  //2
            uint32 returnAura;                              //3
            uint32 returnSpell;                             //4
            uint32 noDamageImmune;                          //5
            uint32 openTextID;                              //6
            uint32 losOK;                                   //7
            uint32 conditionID1;                            //8 NYI
        } flagstand;
        //25 GAMEOBJECT_TYPE_FISHINGHOLE                    // not implemented yet
        struct
        {
            uint32 radius;                                  //0 how close bobber must land for sending loot
            uint32 lootId;                                  //1
            uint32 minSuccessOpens;                         //2
            uint32 maxSuccessOpens;                         //3
            uint32 lockId;                                  //4 -> Lock.dbc; possibly 1628 for all?
        } fishinghole;
        //26 GAMEOBJECT_TYPE_FLAGDROP
        struct
        {
            uint32 lockId;                                  //0
            uint32 eventID;                                 //1
            uint32 pickupSpell;                             //2
            uint32 noDamageImmune;                          //3
            uint32 openTextID;                              //4
        } flagdrop;
        //27 GAMEOBJECT_TYPE_MINI_GAME
        struct
        {
            uint32 gameType;                                //0
        } miniGame;
        //29 GAMEOBJECT_TYPE_CAPTURE_POINT
        struct
        {
            uint32 radius;                                  //0
            uint32 spell;                                   //1
            uint32 worldState1;                             //2
            uint32 worldstate2;                             //3
            uint32 winEventID1;                             //4
            uint32 winEventID2;                             //5
            uint32 contestedEventID1;                       //6
            uint32 contestedEventID2;                       //7
            uint32 progressEventID1;                        //8
            uint32 progressEventID2;                        //9
            uint32 neutralEventID1;                         //10
            uint32 neutralEventID2;                         //11
            uint32 neutralPercent;                          //12
            uint32 worldstate3;                             //13
            uint32 minSuperiority;                          //14
            uint32 maxSuperiority;                          //15
            uint32 minTime;                                 //16
            uint32 maxTime;                                 //17
            uint32 large;                                   //18
            uint32 highlight;                               //19
        } capturePoint;
        //30 GAMEOBJECT_TYPE_AURA_GENERATOR
        struct
        {
            uint32 startOpen;                               //0
            uint32 radius;                                  //1
            uint32 auraID1;                                 //2
            uint32 conditionID1;                            //3
            uint32 auraID2;                                 //4
            uint32 conditionID2;                            //5
            uint32 serverOnly;                              //6
        } auraGenerator;
        //31 GAMEOBJECT_TYPE_DUNGEON_DIFFICULTY
        struct
        {
            uint32 mapID;                                   //0
            uint32 difficulty;                              //1
        } dungeonDifficulty;
        //32 GAMEOBJECT_TYPE_DO_NOT_USE_YET
        struct
        {
            uint32 mapID;                                   //0
            uint32 difficulty;                              //1
        } doNotUseYet;
        //33 GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING
        struct
        {
            uint32 dmgPctState1;                            //0
            uint32 dmgPctState2;                            //1
            uint32 state1Name;                              //2
            uint32 state2Name;                              //3
        } destructibleBuilding;
        //34 GAMEOBJECT_TYPE_GUILDBANK
        struct
        {
            uint32 conditionID1;                            //0
        } guildbank;
#ifdef LICH_KING
        //35 GAMEOBJECT_TYPE_TRAPDOOR
        struct
        {
            uint32 whenToPause;                             // 0
            uint32 startOpen;                               // 1
            uint32 autoClose;                               // 2
        } trapDoor;
#endif

        // not use for specific field access (only for output with loop by all filed), also this determinate max union size
        struct                                              // GAMEOBJECT_TYPE_SPELLCASTER
        {
            uint32 data[MAX_GAMEOBJECT_DATA];
        } raw;
    };

    //helpers 
    
    uint32 GetAutoCloseTime() const
    {
        uint32 autoCloseTime = 0;
        switch (type)
        {
        case GAMEOBJECT_TYPE_DOOR:          autoCloseTime = door.autoCloseTime; break;
        case GAMEOBJECT_TYPE_BUTTON:        autoCloseTime = button.autoCloseTime; break;
        case GAMEOBJECT_TYPE_TRAP:          autoCloseTime = trap.autoCloseTime; break;
        case GAMEOBJECT_TYPE_GOOBER:        autoCloseTime = goober.autoCloseTime; break;
        case GAMEOBJECT_TYPE_TRANSPORT:     autoCloseTime = transport.autoCloseTime; break;
        case GAMEOBJECT_TYPE_AREADAMAGE:    autoCloseTime = areadamage.autoCloseTime; break;
        default: break;
        }
        return autoCloseTime /* xinef: changed to milliseconds/ IN_MILLISECONDS*/;              // prior to 3.0.3, conversion was / 0x10000;
    }

    bool IsDespawnAtAction() const
    {
        switch (type)
        {
        case GAMEOBJECT_TYPE_CHEST:  return chest.consumable != 0;
        case GAMEOBJECT_TYPE_GOOBER: return goober.consumable != 0;
        default: return false;
        }
    }

    bool IsUsableMounted() const
    {
        switch (type)
        {
            case GAMEOBJECT_TYPE_QUESTGIVER: return questgiver.allowMounted != 0;
            case GAMEOBJECT_TYPE_TEXT: return text.allowMounted != 0;
            case GAMEOBJECT_TYPE_GOOBER: return goober.allowMounted != 0;
            case GAMEOBJECT_TYPE_SPELLCASTER: return spellcaster.allowMounted != 0;
            default: return false;
        }
    }

    uint32 GetLootId() const
    {
        switch (type)
        {
            case GAMEOBJECT_TYPE_CHEST:       return chest.lootId;
            case GAMEOBJECT_TYPE_FISHINGHOLE: return fishinghole.lootId;
            case GAMEOBJECT_TYPE_FISHINGNODE: return fishnode.lootId;
            default: return 0;
        }
    }

    uint32 GetCharges() const                               // despawn at uses amount
    {
        switch (type)
        {
            //case GAMEOBJECT_TYPE_TRAP:        return trap.charges;
            case GAMEOBJECT_TYPE_GUARDPOST:   return guardpost.charges;
            case GAMEOBJECT_TYPE_SPELLCASTER: return spellcaster.charges;
            default: return 0;
        }
    }

    uint32 GetGossipMenuId() const
    {
        switch (type)
        {
            case GAMEOBJECT_TYPE_QUESTGIVER:    return questgiver.gossipID;
            case GAMEOBJECT_TYPE_GOOBER:        return goober.gossipID;
            default: return 0;
        }
    }

    uint32 GetCooldown() const                              // Cooldown preventing goober and traps to cast spell
    {
        switch (type)
        {
            case GAMEOBJECT_TYPE_TRAP:        return trap.cooldown;
            case GAMEOBJECT_TYPE_GOOBER:      return goober.cooldown;
            default: return 0;
        }
    }

    bool GetDespawnPossibility() const                      // despawn at targeting of cast?
    {
        switch (type)
        {
        case GAMEOBJECT_TYPE_DOOR:       return door.noDamageImmune != 0;
        case GAMEOBJECT_TYPE_BUTTON:     return button.noDamageImmune != 0;
        case GAMEOBJECT_TYPE_QUESTGIVER: return questgiver.noDamageImmune != 0;
        case GAMEOBJECT_TYPE_GOOBER:     return goober.noDamageImmune != 0;
        case GAMEOBJECT_TYPE_FLAGSTAND:  return flagstand.noDamageImmune != 0;
        case GAMEOBJECT_TYPE_FLAGDROP:   return flagdrop.noDamageImmune != 0;
        default: return true;
        }
    }

    uint32 GetLinkedGameObjectEntry() const
    {
        switch (type)
        {
        case GAMEOBJECT_TYPE_CHEST:       return chest.linkedTrapId;
        case GAMEOBJECT_TYPE_SPELL_FOCUS: return spellFocus.linkedTrapId;
        case GAMEOBJECT_TYPE_GOOBER:      return goober.linkedTrapId;
        default: return 0;
        }
    }

    uint32 GetLockId() const
    {
        switch (type)
        {
            case GAMEOBJECT_TYPE_DOOR:       return door.lockId;
            case GAMEOBJECT_TYPE_BUTTON:     return button.lockId;
            case GAMEOBJECT_TYPE_QUESTGIVER: return questgiver.lockId;
            case GAMEOBJECT_TYPE_CHEST:      return chest.lockId;
            case GAMEOBJECT_TYPE_TRAP:       return trap.lockId;
            case GAMEOBJECT_TYPE_GOOBER:     return goober.lockId;
            case GAMEOBJECT_TYPE_AREADAMAGE: return areadamage.lockId;
            case GAMEOBJECT_TYPE_CAMERA:     return camera.lockId;
            case GAMEOBJECT_TYPE_FLAGSTAND:  return flagstand.lockId;
            case GAMEOBJECT_TYPE_FISHINGHOLE:return fishinghole.lockId;
            case GAMEOBJECT_TYPE_FLAGDROP:   return flagdrop.lockId;
            default: return 0;
        }
    }

    uint32 GetEventScriptId() const
    {
        switch (type)
        {
        case GAMEOBJECT_TYPE_GOOBER:        return goober.eventId;
        case GAMEOBJECT_TYPE_CHEST:         return chest.eventId;
        case GAMEOBJECT_TYPE_CAMERA:        return camera.eventID;
        default: return 0;
        }
    }

    std::string AIName;
    uint32 ScriptId;
};

struct TransportAnimation;

typedef std::unordered_map<uint32, GameObjectTemplate> GameObjectTemplateContainer;

union GameObjectValue
{
    //11 GAMEOBJECT_TYPE_TRANSPORT
    struct
    {
        //ever incrementing time
        uint32 PathProgress;
        TransportAnimation const* AnimationInfo;
        uint32 CurrentSeg;
    } Transport;
};

struct GameObjectLocale
{
    std::vector<std::string> Name;
    std::vector<std::string> CastBarCaption;
};

// client side GO show states
enum GOState: uint32
{
    GO_STATE_ACTIVE             = 0,                        // show in world as used and not reset (closed door open)
    GO_STATE_READY              = 1,                        // show in world as ready (closed door close)
    GO_STATE_ACTIVE_ALTERNATIVE = 2,                        // show in world as used in alt way and not reset (closed door open by cannon fire)

    MAX_GO_STATE,
};

// from `gameobject`
struct GameObjectData
{
    uint32 id;                                              // entry in gamobject_template
    uint32 mapid;
    float posX;
    float posY;
    float posZ;
    float orientation;
    G3D::Quat rotation;
    int32  spawntimesecs;
    uint32 animprogress;
    uint32 go_state;
    uint8 spawnMask;
    uint32 ArtKit;
    uint32 ScriptId;
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

// For containers:  [GO_NOT_READY]->GO_READY (close)->GO_ACTIVATED (open) ->GO_JUST_DEACTIVATED->GO_READY        -> ...
// For bobber:      GO_NOT_READY  ->GO_READY (close)->GO_ACTIVATED (open) ->GO_JUST_DEACTIVATED-><deleted>
// For door(closed):[GO_NOT_READY]->GO_READY (close)->GO_ACTIVATED (open) ->GO_JUST_DEACTIVATED->GO_READY(close) -> ...
// For door(open):  [GO_NOT_READY]->GO_READY (open) ->GO_ACTIVATED (close)->GO_JUST_DEACTIVATED->GO_READY(open)  -> ...
enum LootState: uint32
{
    GO_NOT_READY = 0,
    GO_READY,                                               // can be ready but despawned, and then not possible activate until spawn
    GO_ACTIVATED,
    GO_JUST_DEACTIVATED
};

class Unit;
class GameObjectModel;

// 5 sec for bobber catch
#define FISHING_BOBBER_READY_TIME 5
//time before chest are automatically despawned after first loot
#define CHEST_DESPAWN_TIME 300

class TC_GAME_API GameObject : public WorldObject, public GridObject<GameObject>, public MapObject
{
    public:
        explicit GameObject();
        ~GameObject() override;

        void BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, Player* target) const override;

        void AddToWorld() override;
        void RemoveFromWorld() override;
		void CleanupsBeforeDelete(bool finalCleanup = true) override;

        virtual bool Create(uint32 guidlow, uint32 name_id, Map *map, uint32 phaseMask, Position const& pos, G3D::Quat const& rotation, uint32 animprogress, GOState go_state, uint32 ArtKit = 0);
        void Update(uint32 diff) override;
        static GameObject* GetGameObject(WorldObject& object, uint64 guid);
        GameObjectTemplate const* GetGOInfo() const;
        GameObjectData const* GetGOData() const { return m_goData; }
        GameObjectValue const* GetGOValue() const { return &m_goValue; }

        bool IsTransport() const;

        void SetOwnerGUID(uint64 owner)
        {
            m_spawnedByDefault = false;                     // all object with owner is despawned after delay
            SetUInt64Value(OBJECT_FIELD_CREATED_BY, owner);
        }
        uint64 GetOwnerGUID() const { return GetUInt64Value(OBJECT_FIELD_CREATED_BY); }
        Unit* GetOwner() const;

        uint32 GetSpawnId() const { return m_spawnId; }

        void SetTransportPathRotation(G3D::Quat const& rot);

        // overwrite WorldObject function for proper name localization
        std::string const& GetNameForLocaleIdx(LocaleConstant locale_idx) const override;

        void SaveToDB();
        void SaveToDB(uint32 mapid, uint8 spawnMask);
        bool LoadFromDB(uint32 guid, Map *map);
        void DeleteFromDB();
        void SetLootState(LootState state, Unit* unit = nullptr);
        uint32 GetLockId() const
        {
            if (manual_unlock)
                return 0;

            switch(GetGoType())
            {
                case GAMEOBJECT_TYPE_DOOR:       return GetGOInfo()->door.lockId;
                case GAMEOBJECT_TYPE_BUTTON:     return GetGOInfo()->button.lockId;
                case GAMEOBJECT_TYPE_QUESTGIVER: return GetGOInfo()->questgiver.lockId;
                case GAMEOBJECT_TYPE_CHEST:      return GetGOInfo()->chest.lockId;
                case GAMEOBJECT_TYPE_TRAP:       return GetGOInfo()->trap.lockId;
                case GAMEOBJECT_TYPE_GOOBER:     return GetGOInfo()->goober.lockId;
                case GAMEOBJECT_TYPE_AREADAMAGE: return GetGOInfo()->areadamage.lockId;
                case GAMEOBJECT_TYPE_CAMERA:     return GetGOInfo()->camera.lockId;
                case GAMEOBJECT_TYPE_FLAGSTAND:  return GetGOInfo()->flagstand.lockId;
                case GAMEOBJECT_TYPE_FISHINGHOLE:return GetGOInfo()->fishinghole.lockId;
                case GAMEOBJECT_TYPE_FLAGDROP:   return GetGOInfo()->flagdrop.lockId;
                default: return 0;
            }
        }

        time_t GetRespawnTime() const { return m_respawnTime; }
        time_t GetRespawnTimeEx() const
        {
            time_t now = time(nullptr);
            if(m_respawnTime > now)
                return m_respawnTime;
            else
                return now;
        }
        time_t GetDespawnTime() const { return m_despawnTime; }

        //Force despawn after specified time 
        void SetDespawnTimer(uint32 timer); 
        void SetRespawnTime(int32 respawn);
        void Respawn();
        bool isSpawned() const
        {
            return m_respawnDelayTime == 0 ||
                (m_respawnTime > 0 && !m_spawnedByDefault) ||
                (m_respawnTime == 0 && m_spawnedByDefault);
        }
        bool isSpawnedByDefault() const { return m_spawnedByDefault; }
        void SetSpawnedByDefault(bool b) { m_spawnedByDefault = b; }
        uint32 GetRespawnDelay() const { return m_respawnDelayTime; }
        void Refresh();
        void Delete();
        void SetSpellId(uint32 id) { m_spellId = id;}
        uint32 GetSpellId() const { return m_spellId;}
        void getFishLoot(Loot *loot);
        GameobjectTypes GetGoType() const { return GameobjectTypes(GetUInt32Value(GAMEOBJECT_TYPE_ID)); }
        void SetGoType(GameobjectTypes type) { SetUInt32Value(GAMEOBJECT_TYPE_ID, type); }
        uint32 GetGoState() const { return GetUInt32Value(GAMEOBJECT_STATE); }
        void SetGoState(GOState state);
        uint32 GetGoArtKit() const { return GetUInt32Value(GAMEOBJECT_ARTKIT); }
        void SetGoArtKit(uint32 artkit);
        uint32 GetGoAnimProgress() const { return GetUInt32Value(GAMEOBJECT_ANIMPROGRESS); }
        void SetGoAnimProgress(uint32 animprogress) { SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, animprogress); }
        
		void SetPhaseMask(uint32 newPhaseMask, bool update) override;

        void EnableCollision(bool enable);

        void Use(Unit* user);

        LootState getLootState() const { return m_lootState; }

        void AddToSkillupList(uint32 PlayerGuidLow) { m_SkillupList.push_back(PlayerGuidLow); }
        bool IsInSkillupList(uint32 PlayerGuidLow) const
        {
            for (uint32 i : m_SkillupList)
                if (i == PlayerGuidLow) return true;
            return false;
        }
        void ClearSkillupList() { m_SkillupList.clear(); }

        void AddUniqueUse(Player* player);
        void AddUse();

        uint32 GetUseCount() const { return m_usetimes; }
        uint32 GetUniqueUseCount() const { return m_unique_users.size(); }

        void SaveRespawnTime() override;

        Loot        loot;

        bool HasQuest(uint32 quest_id) const override;
        bool HasInvolvedQuest(uint32 quest_id) const override;
        bool ActivateToQuest(Player *pTarget) const;
        //Open or activate gameobject (GO_STATE_ACTIVE). No effect if gameobject is already opened
        void UseDoorOrButton(uint32 time_to_restore = 0, bool alternative = false, Unit* user = nullptr);
        //Close or reset gameobject (GO_STATE_READY). No effect if gameobject is already closed
        void ResetDoorOrButton();

		bool IsNeverVisible() const override;

		bool IsAlwaysVisibleFor(WorldObject const* seer) const override;
		bool IsInvisibleDueToDespawn() const override;

		uint8 GetLevelForTarget(WorldObject const* target) const override
		{
			if (Unit* owner = GetOwner())
				return owner->GetLevelForTarget(target);

			return 1;
		}

        uint32 GetLinkedGameObjectEntry() const
        {
            switch(GetGoType())
            {
                case GAMEOBJECT_TYPE_CHEST:       return GetGOInfo()->chest.linkedTrapId;
                case GAMEOBJECT_TYPE_SPELL_FOCUS: return GetGOInfo()->spellFocus.linkedTrapId;
                case GAMEOBJECT_TYPE_GOOBER:      return GetGOInfo()->goober.linkedTrapId;
                default: return 0;
            }
        }

        uint32 GetAutoCloseTime() const
        {
            uint32 autoCloseTime = 0;
            switch(GetGoType())
            {
                case GAMEOBJECT_TYPE_DOOR:          autoCloseTime = GetGOInfo()->door.autoCloseTime; break;
                case GAMEOBJECT_TYPE_BUTTON:        autoCloseTime = GetGOInfo()->button.autoCloseTime; break;
                case GAMEOBJECT_TYPE_TRAP:          autoCloseTime = GetGOInfo()->trap.autoCloseTime; break;
                case GAMEOBJECT_TYPE_GOOBER:        autoCloseTime = GetGOInfo()->goober.autoCloseTime; break;
                case GAMEOBJECT_TYPE_TRANSPORT:     autoCloseTime = GetGOInfo()->transport.autoCloseTime; break;
                case GAMEOBJECT_TYPE_AREADAMAGE:    autoCloseTime = GetGOInfo()->areadamage.autoCloseTime; break;
                default: break;
            }
            return autoCloseTime / 0x10000;
        }

        void TriggeringLinkedGameObject( uint32 trapEntry, Unit* target);

        GameObject* LookupFishingHoleAround(float range);

        //returns SpellCastResult
        uint32 CastSpell(Unit *target, uint32 spell, uint64 originalCaster = 0);
        void SendCustomAnim(uint32 anim);
        bool IsInRange(float x, float y, float z, float radius) const;
        
        void SwitchDoorOrButton(bool activate, bool alternative = false);
        
        GameObjectModel * m_model;
        void GetRespawnPosition(float &x, float &y, float &z, float* ori = nullptr) const;
        
        float GetInteractionDistance() const;

        bool IsStaticTransport() const { return GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT; }
        bool IsMotionTransport() const { return GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT; }

        Transport* ToTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT || GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<Transport*>(this); else return nullptr; }
        Transport const* ToTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT || GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<Transport const*>(this); else return nullptr; }

        StaticTransport* ToStaticTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<StaticTransport*>(this); else return nullptr; }
        StaticTransport const* ToStaticTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<StaticTransport const*>(this); else return nullptr; }

        MotionTransport* ToMotionTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT) return reinterpret_cast<MotionTransport*>(this); else return nullptr; }
        MotionTransport const* ToMotionTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT) return reinterpret_cast<MotionTransport const*>(this); else return nullptr; }

        void UpdateModelPosition();

        void EventInform(uint32 eventId);

        uint32 GetScriptId() const;
        GameObjectAI* AI() const { return m_AI; }

        std::string GetAIName() const;
        void SetDisplayId(uint32 displayid);
        uint32 GetDisplayId() const { return GetUInt32Value(GAMEOBJECT_DISPLAYID); }
        
        uint32 GetFaction() const { return GetUInt32Value(GAMEOBJECT_FACTION); }
        void SetFaction(uint32 faction) { SetUInt32Value(GAMEOBJECT_FACTION, faction); }

        void setManualUnlocked() { manual_unlock = true; RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED); }
        void SetInactive(bool inactive) { m_inactive = inactive; }
        bool IsInactive() { return m_inactive; }

        float GetStationaryX() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionX(); return GetPositionX(); }
        float GetStationaryY() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionY(); return GetPositionY(); }
        float GetStationaryZ() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionZ(); return GetPositionZ(); }
        float GetStationaryO() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetOrientation(); return GetOrientation(); }

        bool AIM_Initialize();
        void AIM_Destroy();
    protected:
        uint32      m_charges;                              // Spell charges for GAMEOBJECT_TYPE_SPELLCASTER (22)
        uint32      m_spellId;
        time_t      m_respawnTime;                          // (secs) time of next respawn (or despawn if GO have owner()),
        uint32      m_respawnDelayTime;                     // (secs) if 0 then current GO state no dependent from timer
        time_t      m_despawnTime;                          // (secs) programmed despawn time
        LootState   m_lootState;
        bool        m_spawnedByDefault;
        time_t      m_cooldownTime;                         // used as internal reaction delay time store (not state change reaction).
                                                            // For traps this: spell casting cooldown, for doors/buttons: reset time.
        GOState     m_prevGoState;                          // What state to set whenever resetting
        bool        m_inactive;
        std::list<uint32> m_SkillupList;

        std::set<uint32> m_unique_users;
        uint32 m_usetimes;

        uint32 m_spawnId;                               ///< For new or temporary gameobjects is 0 for saved it is lowguid
        GameObjectTemplate const* m_goInfo;
        GameObjectData const* m_goData;
        GameObjectValue m_goValue;
        bool manual_unlock;
        
        GameObjectModel* CreateModel();
        void UpdateModel();                                 // updates model in case displayId were changed

        Position m_stationaryPosition;
    private:
		void RemoveFromOwner();

        GameObjectAI* m_AI;
        GridReference<GameObject> m_gridRef;
};
#endif
