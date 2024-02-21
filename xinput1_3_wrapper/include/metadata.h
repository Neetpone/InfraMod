#pragma once

#include "stdafx.h"
#include "json.hpp"

auto g_mapdata = R"(
{
     "infra_c1_m1_office": {
        "camera_targets": 0,
        "corruption_targets": 3,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c2_m1_reserve1": {
        "camera_targets": 16,
        "corruption_targets": 4,
        "repair_targets": 3,
        "mistake_targets": 2,
        "geocaches": 3,
        "water_flow_meter_targets": 1
    },
    "infra_c2_m2_reserve2": {
        "camera_targets": 20,
        "corruption_targets": 7,
        "repair_targets": 2,
        "mistake_targets": 4,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c2_m3_reserve3": {
        "camera_targets": 8,
        "corruption_targets": 3,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c3_m1_tunnel": {
        "camera_targets": 19,
        "corruption_targets": 6,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 1
    },
    "infra_c3_m2_tunnel2": {
        "camera_targets": 11,
        "corruption_targets": 6,
        "repair_targets": 1,
        "mistake_targets": 3,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c3_m3_tunnel3": {
        "camera_targets": 16,
        "corruption_targets": 5,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 1
    },
    "infra_c3_m4_tunnel4": {
        "camera_targets": 33,
        "corruption_targets": 7,
        "repair_targets": 1,
        "mistake_targets": 1,
        "geocaches": 1,
        "water_flow_meter_targets": 1
    },
    "infra_c4_m2_furnace": {
        "camera_targets": 3,
        "corruption_targets": 5,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c4_m3_tower": {
        "camera_targets": 3,
        "corruption_targets": 5,
        "repair_targets": 1,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c5_m1_watertreatment": {
        "camera_targets": 16,
        "corruption_targets": 4,
        "repair_targets": 2,
        "mistake_targets": 1,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c5_m2_sewer": {
        "camera_targets": 22,
        "corruption_targets": 9,
        "repair_targets": 6,
        "mistake_targets": 4,
        "geocaches": 3,
        "water_flow_meter_targets": 0
    },
    "infra_c5_m2b_sewer2": {
        "camera_targets": 10,
        "corruption_targets": 2,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m1_sewer3": {
        "camera_targets": 10,
        "corruption_targets": 1,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m2_metro": {
        "camera_targets": 11,
        "corruption_targets": 6,
        "repair_targets": 0,
        "mistake_targets": 3,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m3_metroride": {
        "camera_targets": 12,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m4_waterplant": {
        "camera_targets": 14,
        "corruption_targets": 2,
        "repair_targets": 2,
        "mistake_targets": 3,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m5_minitrain": {
        "camera_targets": 26,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c6_m6_central": {
        "camera_targets": 3,
        "corruption_targets": 4,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m1_servicetunnel": {
        "camera_targets": 16,
        "corruption_targets": 6,
        "repair_targets": 2,
        "mistake_targets": 4,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m1b_skyscraper": {
        "camera_targets": 0,
        "corruption_targets": 3,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m2_bunker": {
        "camera_targets": 17,
        "corruption_targets": 7,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m3_stormdrain": {
        "camera_targets": 29,
        "corruption_targets": 3,
        "repair_targets": 1,
        "mistake_targets": 1,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m4_cistern": {
        "camera_targets": 19,
        "corruption_targets": 3,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c7_m5_powerstation": {
        "camera_targets": 4,
        "corruption_targets": 1,
        "repair_targets": 1,
        "mistake_targets": 4,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m1_powerstation2": {
        "camera_targets": 18,
        "corruption_targets": 2,
        "repair_targets": 2,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m3_isle1": {
        "camera_targets": 6,
        "corruption_targets": 2,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 3,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m4_isle2": {
        "camera_targets": 9,
        "corruption_targets": 4,
        "repair_targets": 1,
        "mistake_targets": 2,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m5_isle3": {
        "camera_targets": 15,
        "corruption_targets": 6,
        "repair_targets": 1,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m6_business": {
        "camera_targets": 10,
        "corruption_targets": 5,
        "repair_targets": 1,
        "mistake_targets": 0,
        "geocaches": 3,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m7_business2": {
        "camera_targets": 10,
        "corruption_targets": 1,
        "repair_targets": 1,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c8_m8_officeblackout": {
        "camera_targets": 1,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c9_m1_rails": {
        "camera_targets": 5,
        "corruption_targets": 1,
        "repair_targets": 1,
        "mistake_targets": 0,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c9_m2_tenements": {
        "camera_targets": 12,
        "corruption_targets": 10,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c9_m3_river": {
        "camera_targets": 5,
        "corruption_targets": 2,
        "repair_targets": 0,
        "mistake_targets": 1,
        "geocaches": 2,
        "water_flow_meter_targets": 0
    },
    "infra_c9_m4_villa": {
        "camera_targets": 4,
        "corruption_targets": 12,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
    "infra_c9_m5_field": {
        "camera_targets": 11,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 1,
        "water_flow_meter_targets": 0
    },
   "infra_c10_m1_npp": {
        "camera_targets": 12,
        "corruption_targets": 1,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c10_m2_reactor": {
        "camera_targets": 11,
        "corruption_targets": 6,
        "repair_targets": 2,
        "mistake_targets": 17,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c10_m3_roof": {
        "camera_targets": 0,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c11_ending_1": {
        "camera_targets": 0,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c11_ending_2": {
        "camera_targets": 0,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    },
    "infra_c11_ending_3": {
        "camera_targets": 0,
        "corruption_targets": 0,
        "repair_targets": 0,
        "mistake_targets": 0,
        "geocaches": 0,
        "water_flow_meter_targets": 0
    }
}
)"_json;