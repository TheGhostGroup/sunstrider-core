
#ifndef TRINITYCORE_DYNAMICOBJECT_H
#define TRINITYCORE_DYNAMICOBJECT_H

#include "Object.h"

class Unit;

enum DynamicObjectType
{
    DYNAMIC_OBJECT_PORTAL           = 0x0,      // unused
    DYNAMIC_OBJECT_AREA_SPELL       = 0x1,
    DYNAMIC_OBJECT_FARSIGHT_FOCUS   = 0x2
};

class TC_GAME_API DynamicObject : public WorldObject, public GridObject<DynamicObject>, public MapObject
{
    public:
        typedef std::set<Unit*> AffectedSet;
        explicit DynamicObject(bool isWorldObject);
		~DynamicObject();

        void AddToWorld() override;
        void RemoveFromWorld() override;

        bool Create(uint32 guidlow, Unit *caster, uint32 spellId, uint32 effIndex, Position const& pos, int32 duration, float radius, DynamicObjectType type);
        void Update(uint32 p_time) override;
		void Remove();
		uint32 GetSpellId() const { return GetUInt32Value(DYNAMICOBJECT_SPELLID); }
		uint32 GetEffIndex() const { return m_effIndex; }
		uint32 GetDuration() const { return m_aliveDuration; }
		//void SetDuration(int32 newDuration);
		uint64 GetCasterGUID() const { return GetGuidValue(DYNAMICOBJECT_CASTER); }
		Unit* GetCaster() const;
		float GetRadius() const { return GetFloatValue(DYNAMICOBJECT_RADIUS); }
        bool IsAffecting(Unit *unit) const { return m_affected.find(unit) != m_affected.end(); }
        void AddAffected(Unit *unit);
        void RemoveAffected(Unit *unit) { m_affected.erase(unit); }
        void Delay(int32 delaytime);
		/*void SetAura(Aura* aura);
		void RemoveAura();*/

		/*void BindToCaster();
		void UnbindFromCaster();*/

		void SetCasterViewpoint();
		void RemoveCasterViewpoint();

    protected:
		/*Aura* _aura;
		Aura* _removedAura;*/

		uint32 m_effIndex;
		int32  m_aliveDuration;
		uint32 m_updateTimer;
		int32 _duration; // for non-aura dynobjects
//        float m_radius; //we use this instead of DYNAMICOBJECT_RADIUS because we're altering DYNAMICOBJECT_RADIUS to change visual size
        AffectedSet m_affected;
		//Unit* _caster;
		DynamicObjectType m_type;

		bool _isViewpoint;
    private:
        GridReference<DynamicObject> m_gridRef;
};
#endif
