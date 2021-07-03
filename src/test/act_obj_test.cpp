#include "ExtraDescription.hpp"
#include "MemFile.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "merc.h"

#include <catch2/catch.hpp>

extern bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj,
                                         GenericList<Object *> &objs_to);
extern bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj, Object *obj_to);

TEST_CASE("unique object enforcement") {

    Char char_from{};
    Char char_to{};
    ObjectIndex obj_idx{};
    obj_idx.item_type = ITEM_LIGHT;

    Object existing_obj{};
    existing_obj.objIndex = &obj_idx;
    existing_obj.item_type = obj_idx.item_type;

    Object moving_obj{};
    moving_obj.objIndex = &obj_idx;
    moving_obj.item_type = obj_idx.item_type;
    // Default case for most tests: both the existing & moving object instances are flagged unique.
    SET_BIT(obj_idx.extra_flags, ITEM_UNIQUE);
    existing_obj.extra_flags = obj_idx.extra_flags;
    moving_obj.extra_flags = obj_idx.extra_flags;

    SECTION("moving to an inventory") {
        SECTION("unique object collides") {
            char_to.carrying.add_back(&existing_obj);

            auto result = obj_move_violates_uniqueness(nullptr, &char_to, &moving_obj, char_to.carrying);

            REQUIRE(result);
        }
        SECTION("unique object doesn't collide") {
            auto result = obj_move_violates_uniqueness(nullptr, &char_to, &moving_obj, char_to.carrying);

            REQUIRE(!result);
        }
        SECTION("non-unique object doesn't collide") {
            REMOVE_BIT(obj_idx.extra_flags, ITEM_UNIQUE);
            existing_obj.extra_flags = obj_idx.extra_flags;
            moving_obj.extra_flags = obj_idx.extra_flags;

            char_to.carrying.add_back(&existing_obj);

            auto result = obj_move_violates_uniqueness(nullptr, &char_to, &moving_obj, char_to.carrying);

            REQUIRE(!result);
        }
        SECTION("unique object to shopkeeper inventory doesn't collide") {
            // Flag the char_to as an NPC shopkeeper and give it the existing unique object.
            // We allow unique items to be handed to shopkeepers. They can resell them and it
            // also means that the Corpse Summoner can accept summoning shards, which are also flagged unique.
            test::MemFile shopkeeper(R"mob(
            #3001
            trader~
            The Trader~
            The Trader
            ~
            ~
            human~
            A 0 900 0
            23 0 1d1+999 1d1+99 1d8+20 beating
            -30 -30 -30 -30
            CDF ABCD 0 0
            stand stand male 500
            0 0 medium 0
            )mob");
            auto opt_char_to_idx = MobIndexData::from_file(shopkeeper.file());
            REQUIRE(opt_char_to_idx);
            SHOP_DATA shop{};
            char_to.pIndexData = &opt_char_to_idx.value();
            char_to.pIndexData->pShop = &shop;
            SET_BIT(char_to.act, ACT_IS_NPC);
            char_to.carrying.add_back(&existing_obj);

            auto result = obj_move_violates_uniqueness(&char_from, &char_to, &moving_obj, char_to.carrying);

            REQUIRE(!result);
        }
        SECTION("unique object staying in owner's inventory doesn't collide") {
            // e.g. player runs: 'get bag_in_my_inventory.someuniqueobj', to move it to their top level inventory
            auto result = obj_move_violates_uniqueness(&char_from, &char_from, &moving_obj, char_to.carrying);

            REQUIRE(!result);
        }
    }
    SECTION("moving to a container") {
        ObjectIndex container_idx;
        container_idx.item_type = ITEM_CONTAINER;
        Object container{};
        container.objIndex = &container_idx;
        container.item_type = container_idx.item_type;

        SECTION("unique object to container collides") {
            container.contains.add_back(&existing_obj);

            auto result = obj_move_violates_uniqueness(&char_from, nullptr, &moving_obj, &container);

            REQUIRE(result);
        }
        SECTION("unique object to container doesn't collide") {
            auto result = obj_move_violates_uniqueness(&char_from, nullptr, &moving_obj, &container);

            REQUIRE(!result);
        }
        SECTION("non-unique object to container doesn't collide") {
            REMOVE_BIT(obj_idx.extra_flags, ITEM_UNIQUE);
            existing_obj.extra_flags = obj_idx.extra_flags;
            moving_obj.extra_flags = obj_idx.extra_flags;

            container.contains.add_back(&existing_obj);

            auto result = obj_move_violates_uniqueness(&char_from, nullptr, &moving_obj, &container);

            REQUIRE(!result);
        }
    }
}
