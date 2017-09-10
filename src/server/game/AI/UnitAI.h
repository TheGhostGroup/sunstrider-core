#ifndef TRINITY_UNITAI_H
#define TRINITY_UNITAI_H

#include "Define.h"
#include "Unit.h"
#include "QuestDef.h"
class Unit;
class Quest;
class Player;

//Selection method used by SelectSpellTarget
enum SelectAggroTarget : int
{
    SELECT_TARGET_RANDOM = 0,                               //Just selects a random target
    SELECT_TARGET_TOPAGGRO,                                 //Selects targes from top aggro to bottom
    SELECT_TARGET_BOTTOMAGGRO,                              //Selects targets from bottom aggro to top
    SELECT_TARGET_NEAREST,
    SELECT_TARGET_FARTHEST,
};


// Simple selector for units using mana
struct TC_GAME_API PowerUsersSelector
{
public:
    PowerUsersSelector(Unit const* unit, Powers power, float dist, bool playerOnly) : _me(unit), _power(power), _dist(dist), _playerOnly(playerOnly) { }
    bool operator()(Unit const* target) const;

private:
    Unit const* _me;
    Powers const _power;
    float const _dist;
    bool const _playerOnly;
};

struct TC_GAME_API FarthestTargetSelector
{
public:
    FarthestTargetSelector(Unit const* unit, float dist, bool playerOnly, bool inLos) : _me(unit), _dist(dist), _playerOnly(playerOnly), _inLos(inLos) {}
    bool operator()(Unit const* target) const;

private:
    const Unit* _me;
    float _dist;
    bool _playerOnly;
    bool _inLos;
};


class TC_GAME_API UnitAI
{
    protected:
        Unit *me;
        //combat movement part not yet implemented. Creatures with m_combatDistance and target distance > 5.0f wont show melee weapons.
        float m_combatDistance;         
        bool m_allowCombatMovement;
        bool m_restoreCombatMovementOnOOM;
    public:
        UnitAI(Unit *u) : me(u), m_combatDistance(0.5f), m_allowCombatMovement(true), m_restoreCombatMovementOnOOM(false) {}
        virtual ~UnitAI() = default;

        virtual bool CanAIAttack(Unit const* /*target*/) const { return true; }
        virtual void AttackStart(Unit *);
        void AttackStartCaster(Unit* victim, float dist);
        virtual void UpdateAI(const uint32 diff) { }

        float GetCombatDistance() { return m_combatDistance; };
        void SetCombatDistance(float dist);

        bool IsCombatMovementAllowed() { return m_allowCombatMovement; };
        void SetCombatMovementAllowed(bool allow);
        void SetRestoreCombatMovementOnOOM(bool set);
        bool GetRestoreCombatMovementOnOOM();

        virtual void InitializeAI() { Reset(); }

        virtual void Reset() {}

        // Called when unit is charmed
        virtual void OnCharmed(Unit* charmer, bool apply) = 0;
        virtual void OnPossess(Unit* charmer, bool apply) { }

        // Pass parameters between AI
        virtual void DoAction(const int32 param) {}
        virtual uint32 GetData(uint32 /*id = 0*/) const { return 0; }
        virtual void SetData(uint32 /*id*/, uint32 /*value*/, Unit* setter = nullptr) {}
        virtual void SetGUID(uint64 /*guid*/, int32 /*id*/ = 0) { }
        virtual uint64 GetGUID(int32 /*id*/ = 0) const { return 0; }

        // Called at any Damage from any attacker (before damage apply)
        virtual void DamageTaken(Unit *done_by, uint32 & /*damage*/) {}
        
        // Called at any Damage to any victim (before damage apply)
        virtual void DamageDealt(Unit* /*victim*/, uint32& /*damage*/, DamageEffectType /*damageType*/) { }

        // Called when the creature receives heal
        virtual void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/) {}

        // Called when the unit heals
        virtual void HealDone(Unit* /*done_to*/, uint32& /*addhealth*/) { }

        //Do melee swing of current victim if in rnage and ready and not casting
        virtual void DoMeleeAttackIfReady();
        bool DoSpellAttackIfReady(uint32 spell);

        //Selects a unit from the creature's current aggro list
        bool checkTarget(Unit* target, bool playersOnly, float radius, bool noTank = false);

        //SelectTargetFromPlayerList -> me->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0.0f, 500.0f, true)
        // Select the targets satisfying the predicate.
        // predicate shall extend std::unary_function<Unit*, bool>
        template<class PREDICATE> Unit* SelectTarget(SelectAggroTarget targetType, uint32 position, PREDICATE const& predicate)
        {
            ThreatContainer::StorageType const& threatlist = me->getThreatManager().getThreatList();
            if (position >= threatlist.size())
                return nullptr;

            std::list<Unit*> targetList;
            for (auto itr : threatlist)
                if (predicate(itr->getTarget()))
                    targetList.push_back(itr->getTarget());

            if (position >= targetList.size())
                return nullptr;

            if (targetType == SELECT_TARGET_NEAREST || targetType == SELECT_TARGET_FARTHEST)
                targetList.sort(Trinity::ObjectDistanceOrderPred(me));

            switch (targetType)
            {
            case SELECT_TARGET_NEAREST:
            case SELECT_TARGET_TOPAGGRO:
            {
                auto itr = targetList.begin();
                std::advance(itr, position);
                return *itr;
            }
            case SELECT_TARGET_FARTHEST:
            case SELECT_TARGET_BOTTOMAGGRO:
            {
                auto ritr = targetList.rbegin();
                std::advance(ritr, position);
                return *ritr;
            }
            case SELECT_TARGET_RANDOM:
            {
                auto itr = targetList.begin();
                std::advance(itr, urand(position, targetList.size() - 1));
                return *itr;
            }
            default:
                break;
            }

            return nullptr;
        }

        Unit* SelectTarget(SelectAggroTarget target, uint32 position);
        Unit* SelectTarget(SelectAggroTarget target, uint32 position, float dist, bool playerOnly, bool noTank = false);
        Unit* SelectTarget(SelectAggroTarget target, uint32 position, float distNear, float distFar, bool playerOnly);
        Unit* SelectTarget(uint32 position, float distMin, float distMax, bool playerOnly, bool auraCheck, bool exceptPossesed, uint32 spellId, uint32 effIndex);
        void SelectUnitList(std::list<Unit*> &targetList, uint32 num, SelectAggroTarget target, float dist, bool playerOnly, uint32 notHavingAuraId = 0, uint8 effIndex = 0);

        // Called when a player opens a gossip dialog with the creature.
        virtual bool GossipHello(Player* /*player*/) { return false; }

        // Called when a player selects a gossip item in the creature's gossip menu.
        virtual bool GossipSelect(Player* /*player*/, uint32 /*menuId*/, uint32 /*gossipListId*/) { return false;  }

        // Called when a player selects a gossip with a code in the creature's gossip menu.
        virtual bool GossipSelectCode(Player* /*player*/, uint32 /*menuId*/, uint32 /*gossipListId*/, const char*  /*code*/) { return false;  }

        // Called when a player accepts a quest from the creature.
        virtual void QuestAccept(Player* /*player*/, Quest const* /*quest*/) { }

        // Called when a player completes a quest and is rewarded, opt is the selected item's index or 0
        virtual void QuestReward(Player* /*player*/, Quest const* /*quest*/, uint32 /*opt*/) { }

        // Called when the dialog status between a player and the creature is requested.
        virtual uint32 GetDialogStatus(Player* /*player*/) { return DIALOG_STATUS_SCRIPTED_NO_STATUS; }
};

#endif //TRINITY_UNITAI_H