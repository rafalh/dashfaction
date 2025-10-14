#pragma once

#include "object.h"
#include "os/timestamp.h"

namespace rf
{
    struct Event : Object
    {
        int event_type;
        float delay_seconds;
        Timestamp delay_timestamp;
        VArray<int> links;
        int triggered_by_handle;
        int trigger_handle;
        int event_flags;
        bool delayed_msg;

        virtual void initialize() = 0;
        virtual void turn_on() = 0;
        virtual void turn_off() = 0;
        virtual void process() = 0;

    };
    static_assert(sizeof(Event) == 0x2B8);

    static auto& event_type_forwards_messages = addr_as_ref<bool(int)>(0x004B8C40);

    static auto& event_lookup_from_uid = addr_as_ref<Event*(int uid)>(0x004B6820);
    static auto& event_names = addr_as_ref<const char*[90]>(0x005A1A3C);

    // There are 90 built-in events.
    enum EventType : int {
        Play_Sound,
        Slay_Object,
        Remove_Object,
        Invert,
        Teleport,
        Goto,
        Goto_Player,
        Look_At,
        Shoot_At,
        Shoot_Once,
        Explode,
        Play_Animation,
        Play_Custom_Animation,
        Heal,
        Armor,
        Message,
        When_Dead,
        Continuous_Damage,
        Shake_Player,
        Give_Item_To_Player,
        Cyclic_Timer,
        Switch_Model,
        Load_Level,
        Spawn_Object,
        Make_Invulnerable,
        Make_Walk,
        Make_Fly,
        Drop_Point_Marker,
        Follow_Waypoints,
        Follow_Player,
        Set_Friendliness,
        Set_Light_State,
        Switch,
        Swap_Textures,
        Set_AI_Mode,
        Goal_Create,
        Goal_Check,
        Goal_Set,
        Attack,
        Particle_State,
        Set_Liquid_Depth,
        Music_Start,
        Music_Stop,
        Bolt_State,
        Set_Gravity,
        Alarm_Siren,
        Alarm,
        Go_Undercover,
        Delay,
        Monitor_State,
        UnHide,
        Push_Region_State,
        When_Hit,
        Headlamp_State,
        Item_Pickup_State,
        Cutscene,
        Strip_Player_Weapons,
        Fog_State,
        Detach,
        Skybox_State,
        Force_Monitor_Update,
        Black_Out_Player,
        Turn_Off_Physics,
        Teleport_Player,
        Holster_Weapon,
        Holster_Player_Weapon,
        Modify_Rotating_Mover,
        Clear_Endgame_If_Killed,
        Win_PS2_Demo,
        Enable_Navpoint,
        Play_Vclip,
        Endgame,
        Mover_Pause,
        Countdown_Begin,
        Countdown_End,
        When_Countdown_Over,
        Activate_Capek_Shield,
        When_Enter_Vehicle,
        When_Try_Exit_Vehicle,
        Fire_Weapon_No_Anim,
        Never_Leave_Vehicle,
        Drop_Weapon,
        Ignite_Entity,
        When_Cutscene_Over,
        When_Countdown_Reaches,
        Display_Fullscreen_Image,
        Defuse_Nuke,
        When_Life_Reaches,
        When_Armor_Reaches,
        Reverse_Mover,
        // Alpine Faction events begin at 100.
        Set_Variable = 100,
        Clone_Entity,
        Set_Player_World_Collide,
        Switch_Random,
        Difficulty_Gate,
        HUD_Message,
        Play_Video,
        Set_Level_Hardness,
		Sequence,
        Clear_Queued,
		Remove_Link,
        Route_Node,
        Add_Link,
        Valid_Gate,
        Goal_Math,
        Goal_Gate,
        Scope_Gate,
        Inside_Gate,
        Anchor_Marker,
        Force_Unhide,
        Set_Difficulty,
        Set_Fog_Far_Clip,
        AF_When_Dead,
        Gametype_Gate,
        When_Picked_Up,
        Set_Skybox,
        Set_Life,
        Set_Debris,
        Set_Fog_Color,
        Set_Entity_Flag,
        AF_Teleport_Player,
        Set_Item_Drop,
        AF_Heal,
        Anchor_Marker_Orient,
        Light_State,
        World_HUD_Sprite,
        Set_Light_Color,
    };
}
