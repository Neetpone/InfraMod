#include "counters.h"
#include "overlay.h"
#include "metadata.h"

namespace mod {
	namespace counters {
		using infra::Engine;
		using infra::functions::GLOBALESTATE;

		static nlohmann::json current_mapdata;

		const char* GetMapStatName(int event_type) {
			const char* result = NULL;

			switch (event_type)
			{
			case 0:
				result = "successful_photos";
				break;
			case 1:
				result = "corruption_uncovered";
				break;
			case 2:
				result = "spots_repaired";
				break;
			case 3:
				result = "mistakes_made";
				break;
			case 4:
				result = "geocaches_found";
				break;
			case 5:
				result = "water_flow_meters_repaired";
				break;
			default:
				break;
			}
			return result;
		}

		std::string get_counter_name(const char* map_name, const char* stat_name) {
			return std::string(map_name) + "_counter_" + std::string(stat_name);
		}

		std::string format_stat(int value, int max_value) {
			return std::to_string(value) + " / " + std::to_string(max_value);
		}

		int get_max_value(int event_type, const char* map_name) {
			const char* v = "";

			switch (event_type)
			{
			case 0:
				v = "camera_targets";
				break;
			case 1:
				v = "corruption_targets";
				break;
			case 2:
				v = "repair_targets";
				break;
			case 3:
				v = "mistake_targets";
				break;
			case 4:
				v = "geocaches";
				break;
			case 5:
				v = "water_flow_meter_targets";
				break;
			default:
				break;
			}

			std::string s = map_name;
			transform(s.begin(), s.end(), s.begin(), ::tolower);
			return current_mapdata[s][v];
		}

		void update_gui_table(const int event_type, const char* map_name, const int value) {
			int max_value = 0;

			try {
				max_value = get_max_value(event_type, map_name);
			}
			catch (std::exception e) {
			}

			const ImVec4 color = (value == max_value) ? overlay::fontColorMax : overlay::fontColor;

			switch (event_type) {
			case 0:
				overlay::lines[0] = overlay::OverlayLine_t(
					"Defects:     ",
					format_stat(value, max_value),
					color,
					color
				);
				break;
			case 1:
				overlay::lines[1] = overlay::OverlayLine_t(
					"Corruption:  ",
					format_stat(value, max_value),
					color,
					color
				);
				break;
			case 2:
				overlay::lines[2] = overlay::OverlayLine_t(
					"Repairs:     ",
					format_stat(value, max_value),
					color,
					color
				);
				break;
			case 3:
				// "mistakes_made";
				break;
			case 4:
				overlay::lines[3] = overlay::OverlayLine_t(
					"Geocaches:   ",
					format_stat(value, max_value),
					color,
					color
				);
				break;
			case 5:
				overlay::lines[4] = overlay::OverlayLine_t(
					"Flow meters: ",
					format_stat(value, max_value),
					color,
					color
				);
				break;
			default:
				break;
			}
		}

		void init_counter(const char* map_name, int event_type) {
			const std::string name = get_counter_name(map_name, GetMapStatName(event_type));
			const int index = Engine()->GlobalEntity_AddEntity(name.c_str(), map_name, GLOBALESTATE::GLOBAL_OFF);
			int value = 0;

			if (Engine()->GlobalEntity_GetState(index) == GLOBALESTATE::GLOBAL_OFF) {
				Engine()->GlobalEntity_SetCounter(index, 0);
				Engine()->GlobalEntity_SetState(index, GLOBALESTATE::GLOBAL_ON);
			}
			else {
				value = Engine()->GlobalEntity_GetCounter(index);
			}

			update_gui_table(event_type, map_name, value);
		}

		void exclude_inactive_photo_spot(const std::string& map_name, const std::string& spot_name, nlohmann::json& mapdata) {
			const int index = Engine()->GlobalEntity_AddEntity(spot_name.c_str(), map_name.c_str(), GLOBALESTATE::GLOBAL_OFF);

			if (Engine()->GlobalEntity_GetState(index) == GLOBALESTATE::GLOBAL_ON) {
				int num = mapdata[map_name]["camera_targets"];

				if (num > 0) {
					--num;
				}

				mapdata[map_name]["camera_targets"] = num;
			}
		}

		auto exclude_inactive_photo_spots(const std::string& map_name, nlohmann::json mapdata) {
			if (map_name == "infra_c2_m2_reserve2") {
				exclude_inactive_photo_spot(map_name, "infra_reserve1_dam_picture_taken", mapdata);
			}
			else if (map_name == "infra_c3_m2_tunnel2") {
				exclude_inactive_photo_spot(map_name, "infra_tunnel_elevator_picture_taken", mapdata);
				exclude_inactive_photo_spot(map_name, "infra_tunnel_cracks_picture_taken", mapdata);
			}
			else if (map_name == "infra_c5_m1_watertreatment") {
				exclude_inactive_photo_spot(map_name, "infra_watertreatment_steam_picture_taken", mapdata);
			}

			return mapdata;
		}

		/*
			This is called once before a map is loaded
		*/
		void InitMapStats() {
			if (Engine()->is_in_main_menu()) {
				return;
			}

			overlay::lines.resize(5);

			for (int i = 0; i < 6; ++i) {
				overlay::lines[i].nameColor = overlay::fontColor;
				overlay::lines[i].valueColor = overlay::fontColor;
				overlay::lines[i].blinksLeft = 0;
			}

			const char* map_name = Engine()->get_map_name();
			std::string& s = overlay::title.value;
			s.assign(map_name);
			transform(s.begin(), s.end(), s.begin(), ::tolower);

			current_mapdata = exclude_inactive_photo_spots(s, g_mapdata);

			for (int i = 0; i < 6; ++i) {
				init_counter(map_name, i);
			}
		}

		void StatSuccess(int event_type, int count, bool is_new) {
			if (!is_new || count == 0) {
				return;
			}

			const char* stat_name = GetMapStatName(event_type);

			if (!stat_name) {
				return;
			}

			const char* map_name = Engine()->get_map_name();
			const std::string name = get_counter_name(map_name, stat_name);

			const int index = Engine()->GlobalEntity_AddEntity(name.c_str(), map_name, GLOBALESTATE::GLOBAL_OFF);
			const int value = Engine()->GlobalEntity_AddToCounter(index, 1);

			update_gui_table(event_type, map_name, value);

			int line_idx = 0;

			switch (event_type)
			{
			case 0: line_idx = 0; break;
			case 1:	line_idx = 1; break;
			case 2:	line_idx = 2; break;
			case 4:	line_idx = 3; break;
			case 5:	line_idx = 4; break;
			default: break;
			}

			overlay::lines[line_idx].blinksLeft = 7;
		}
	}
}